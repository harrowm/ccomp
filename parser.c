/*
 * File: parser.c
 * Description: Implements the parser for constructing an Abstract Syntax Tree (AST).
 * Purpose: Analyzes token streams and builds a structured representation of the source code.
 */

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Lexer *lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR, // ||
    PREC_AND, // &&
    PREC_EQUALITY, // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM, // + -
    PREC_FACTOR, // * / %
    PREC_UNARY, // ! - +
    PREC_CALL, // . () []
    PREC_PRIMARY
} Precedence;

typedef ASTNode* (*PrefixParseFn)(Parser *parser);
typedef ASTNode (*InfixParseFn)(Parser *parser, ASTNode *left);

typedef struct {
    PrefixParseFn prefix;
    InfixParseFn infix;
    Precedence precedence;
} ParseRule;

// Forward declarations
static ASTNode* parse_expression(Parser *parser);
static ASTNode* parse_statement(Parser *parser);
static ASTNode* parse_block(Parser *parser);
static ASTNode* parse_var_declaration(Parser *parser);
static Type* parse_type(Parser *parser);
static Precedence get_precedence(TokenType type);

static void advance(Parser *parser) {
    parser->previous = parser->current;
    for (;;) {
        parser->current = next_token(parser->lexer);
        if (parser->current.type != TOK_COMMENT && parser->current.type != TOK_WHITESPACE) break;
    }
}

static void synchronize(Parser *parser) {
    LOG_INFO("Synchronize: Current token type: %s", token_type_to_string(parser->current.type));
    LOG_INFO("Synchronize: Previous token type: %s", token_type_to_string(parser->previous.type));
    LOG_INFO("Entering synchronize: current token=%s, previous token=%s", 
             token_type_to_string(parser->current.type), 
             token_type_to_string(parser->previous.type));

    LOG_INFO("Starting synchronization loop");
    LOG_INFO("Initial token: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
    LOG_INFO("Initial previous token: %s, line: %u, column: %u", token_type_to_string(parser->previous.type), parser->previous.line, parser->previous.column);

    int iteration_count = 0;
    const int max_iterations = 1000; // Safeguard to prevent infinite loops

    while (parser->current.type == TOK_SEMICOLON) {
        LOG_INFO("Skipping redundant semicolon during synchronization, iteration: %d", iteration_count);
        LOG_INFO("Current token before advance: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
        advance(parser);
        LOG_INFO("Current token after advance: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
        if (++iteration_count > max_iterations) {
            LOG_ERROR("Exceeded maximum iterations in synchronize while skipping semicolons");
            return;
        }
    }

    if (parser->current.type == TOK_EOF) {
        LOG_INFO("Reached EOF during synchronization");
        return;
    }

    while (parser->current.type == TOK_SEMICOLON || parser->current.type == TOK_UNKNOWN) {
        LOG_INFO("Skipping invalid or redundant token: %s, iteration: %d", token_type_to_string(parser->current.type), iteration_count);
        LOG_INFO("Current token before advance: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
        advance(parser);
        LOG_INFO("Current token after advance: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
        if (++iteration_count > max_iterations) {
            LOG_ERROR("Exceeded maximum iterations in synchronize while skipping invalid tokens");
            return;
        }
    }

    while (parser->current.type != TOK_EOF) {
        LOG_INFO("Checking synchronization point: current token=%s, previous token=%s, iteration: %d", 
                 token_type_to_string(parser->current.type), 
                 token_type_to_string(parser->previous.type), 
                 iteration_count);

        if (parser->previous.type == TOK_SEMICOLON ||
            parser->current.type == TOK_KW_RETURN ||
            parser->current.type == TOK_KW_IF ||
            parser->current.type == TOK_KW_WHILE ||
            parser->current.type == TOK_KW_FOR ||
            parser->current.type == TOK_KW_INT ||
            parser->current.type == TOK_KW_CHAR ||
            parser->current.type == TOK_KW_VOID) {
            LOG_INFO("Recovered at valid synchronization point: %s", token_type_to_string(parser->current.type));
            LOG_INFO("Parser state before resuming: current token=%s, previous token=%s", 
                     token_type_to_string(parser->current.type), 
                     token_type_to_string(parser->previous.type));
            LOG_INFO("Parser panic mode: %s", parser->panic_mode ? "true" : "false");
            return;
        }

        LOG_INFO("Advancing past token: %s, iteration: %d", token_type_to_string(parser->current.type), iteration_count);
        LOG_INFO("Current token before advance: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
        advance(parser);
        LOG_INFO("Current token after advance: %s, line: %u, column: %u", token_type_to_string(parser->current.type), parser->current.line, parser->current.column);
        if (++iteration_count > max_iterations) {
            LOG_ERROR("Exceeded maximum iterations in synchronize while advancing past tokens");
            return;
        }
    }

    LOG_INFO("Exiting synchronize: current token=%s, iteration: %d", token_type_to_string(parser->current.type), iteration_count);
}

static void error_at_current(Parser *parser, const char *message) {
    fprintf(stderr, "Error at line %u, column %u: %s\n",
    parser->current.line, parser->current.column, message);
    parser->had_error = true;
    parser->panic_mode = true;
    synchronize(parser);
}
    
static void consume(Parser *parser, TokenType type, const char *message) {
    LOG_INFO("current token: %s", token_type_to_string(parser->current.type));
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    error_at_current(parser, message);
}

static bool match(Parser *parser, TokenType type) {
    if (parser->current.type == type) {
        advance(parser);
        return true;
    }
    return false;
}
    
static bool check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}
    
static Type* create_type(TypeKind kind) {
    LOG_INFO("creating type %d", kind);
    Type *type = malloc(sizeof(Type));
    if (!type) return NULL;
    type->kind = kind;
    type->base = NULL;
    type->array_size = 0;
    return type;
}

static Type* create_pointer_type(Type *base) {
    Type *type = create_type(TYPE_POINTER);
    if (!type) return NULL;
    type->base = base;
    return type;
}

static Type* create_array_type(Type *base, size_t size) {
    Type *type = create_type(TYPE_ARRAY);
    if (!type) return NULL;
    type->base = base;
    type->array_size = size;
    return type;
}

static ASTNode* create_literal_node(int value, Type *type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_LITERAL;
    node->data.literal.value.int_value = value;
    node->data.literal.type = type;
    return node;
}

static ASTNode *create_literal_node_with_ptr(void *ptr, Type *type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_LITERAL;
    node->data.literal.value.ptr_value = ptr;
    node->data.literal.type = type;
    return node;
}

static ASTNode* create_binary_op_node(int op, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_BINARY_OP;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

static ASTNode* create_unary_op_node(int op, ASTNode *operand, bool is_prefix) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    node->data.unary_op.is_prefix = is_prefix;
    return node;
}

static ASTNode* create_var_ref_node(const char *name, Type *type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_VAR_REF;
    node->data.var_ref.name = strdup(name);
    node->data.var_ref.type = type;
    return node;
}

static ASTNode* create_var_decl_node(const char *name, Type *type, ASTNode *init_value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_VAR_DECL;
    node->data.var_decl.name = strdup(name);
    node->data.var_decl.type = type;
    node->data.var_decl.init_value = init_value;
    return node;
}

static ASTNode* create_assignment_node(const char *name, ASTNode *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_ASSIGNMENT;
    node->data.assignment.name = strdup(name);
    node->data.assignment.value = value;
    return node;
}

static ASTNode* create_function_decl_node(const char *name, Type *return_type, ASTNode *params, ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_FUNCTION_DECL;
    node->data.function_decl.name = strdup(name);
    node->data.function_decl.return_type = return_type;
    node->data.function_decl.params = params;
    node->data.function_decl.body = body;
    return node;
}

static ASTNode* create_return_node(ASTNode *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_RETURN;
    node->data.return_stmt.value = value;
    return node;
}

static ASTNode* create_param_list_node(ASTNode **params, size_t count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_PARAM_LIST;
    node->data.param_list.params = params;
    node->data.param_list.count = count;
    return node;
}

static ASTNode* create_stmt_list_node(ASTNode **stmts, size_t count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_STMT_LIST;
    node->data.stmt_list.stmts = stmts;
    node->data.stmt_list.count = count;
    return node;
}

static ASTNode* create_type_spec_node(Type *type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = NODE_TYPE_SPECIFIER;
    node->data.type_spec.type = type;
    return node;
}

static ASTNode* parse_primary(Parser *parser) {
    LOG_INFO("current token: %s", token_type_to_string(parser->current.type));
    if (match(parser, TOK_INTEGER)) {
        int value = atoi(parser->previous.text);
        return create_literal_node(value, create_type(TYPE_INT));
    }

    if (match(parser, TOK_STRING)) {
        char *value = strndup(parser->previous.text, parser->previous.length);
        return create_literal_node_with_ptr(value, create_type(TYPE_POINTER));
    }

    if (match(parser, TOK_IDENTIFIER)) {
        char *name = strndup(parser->previous.text, parser->previous.length);

        // Check for function call
        if (match(parser, TOK_LPAREN)) {
            // Parse arguments
            ASTNode **args = NULL;
            size_t arg_count = 0;
            size_t arg_capacity = 0;

            if (!check(parser, TOK_RPAREN)) {
                do {
                    if (arg_count >= arg_capacity) {
                        arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                        args = realloc(args, sizeof(ASTNode *) * arg_capacity);
                        if (!args) {
                            LOG_ERROR("Memory allocation failed");
                            return NULL;
                        }
                    }

                    ASTNode *arg = parse_expression(parser);
                    if (!arg) break;

                    args[arg_count++] = arg;
                } while (match(parser, TOK_COMMA));
            }

            consume(parser, TOK_RPAREN, "Expect ')' after function arguments.");

            // Create function call node
            ASTNode *call_node = malloc(sizeof(ASTNode));
            if (!call_node) return NULL;
            call_node->type = NODE_FUNCTION_CALL;
            call_node->data.function_call.name = name;
            call_node->data.function_call.args = args;
            call_node->data.function_call.arg_count = arg_count;
            return call_node;
        }

        // Otherwise, it's a variable reference
        return create_var_ref_node(name, NULL); // Type will be resolved later
    }
    
    if (match(parser, TOK_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        consume(parser, TOK_RPAREN, "Expect ')' after expression.");
        return expr;
    }
    
    error_at_current(parser, "Expect expression.");
    return NULL;
}

static ASTNode* parse_unary(Parser *parser) {
    LOG_INFO("current token: %s", token_type_to_string(parser->current.type));
    if (match(parser, TOK_MINUS) || match(parser, TOK_BANG)) {
        Token op = parser->previous;
        ASTNode *right = parse_unary(parser);
        return create_unary_op_node(op.type, right, true); // true for prefix
    }

    if (match(parser, TOK_MINUS_MINUS)) {
        // Handle prefix decrement
        Token op = parser->previous;
        ASTNode *operand = parse_primary(parser);
        return create_unary_op_node(op.type, operand, true); // true for prefix
    }

    if (match(parser, TOK_PLUS_PLUS)) {
        // Handle prefix increment
        Token op = parser->previous;
        ASTNode *operand = parse_primary(parser);
        return create_unary_op_node(op.type, operand, true); // true for prefix
    }

    return parse_primary(parser);
}

static ASTNode* parse_binary(Parser *parser, ASTNode *left, Precedence precedence) {
    while (1) {
        Precedence current_prec = get_precedence(parser->current.type);
        if (current_prec < precedence) break;
        Token op = parser->current;
        advance(parser);

        // Special case for assignment operator '='
        if (op.type == TOK_EQ && precedence == PREC_ASSIGNMENT) {
            LOG_INFO("Detected assignment operator in parse_binary");
            ASTNode *right = parse_expression(parser);
            if (!right) {
                synchronize(parser);
                return NULL;
            }
            return create_assignment_node(left->data.var_ref.name, right);
        }

        ASTNode *right = parse_unary(parser);
        if (!right) {
            synchronize(parser);
            return NULL;
        }

        current_prec = get_precedence(parser->current.type);
        while (current_prec > get_precedence(op.type)) {
            right = parse_binary(parser, right, current_prec);
            current_prec = get_precedence(parser->current.type);
        }

        LOG_INFO("Creating binary operation node with operator: %s", token_type_to_string(op.type));
        left = create_binary_op_node(op.type, left, right);
    }

    return left;
}

static Precedence get_precedence(TokenType type) {
    switch (type) {
        case TOK_EQ: 
        case TOK_PLUS_EQ:
        case TOK_MINUS_EQ:
        case TOK_STAR_EQ:
        case TOK_SLASH_EQ:
        case TOK_PERCENT_EQ:
            return PREC_ASSIGNMENT; // Same precedence as '='
        case TOK_PIPE_PIPE: 
            return PREC_OR;
        case TOK_AMP_AMP: 
            return PREC_AND;
        case TOK_EQ_EQ:
        case TOK_BANG_EQ: 
            return PREC_EQUALITY;
        case TOK_LT:
        case TOK_LT_EQ:
        case TOK_GT:
        case TOK_GT_EQ: 
            return PREC_COMPARISON;
        case TOK_PLUS:
        case TOK_MINUS: 
            return PREC_TERM;
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT: 
            return PREC_FACTOR;
        default: 
            return PREC_NONE;
    }
}

static ASTNode* parse_expression(Parser *parser) {
    LOG_INFO("Parsing expression: current token='%s'", token_type_to_string(parser->current.type));
    
    ASTNode *left = parse_unary(parser);
    if (!left) {
        synchronize(parser);
        return NULL;
    }

    // Check for postfix decrement
    if (match(parser, TOK_MINUS_MINUS)) {
        return create_unary_op_node(TOK_MINUS_MINUS, left, false); // false for postfix
    }

    // Check for postfix increment
    if (match(parser, TOK_PLUS_PLUS)) {
        return create_unary_op_node(TOK_PLUS_PLUS, left, false); // false for postfix
    }

    if (match(parser, TOK_PLUS_EQ) || match(parser, TOK_MINUS_EQ) || match(parser, TOK_STAR_EQ) ||
        match(parser, TOK_SLASH_EQ) || match(parser, TOK_PERCENT_EQ)) {
        Token op = parser->previous;
        ASTNode *value = parse_expression(parser);
        return create_binary_op_node(op.type, left, value);
    }

    return parse_binary(parser, left, PREC_ASSIGNMENT);
}
    
static ASTNode* parse_var_declaration(Parser *parser) {
    LOG_INFO("current token: %s", token_type_to_string(parser->current.type));
    Type *type = parse_type(parser);
    if (!type) return NULL;

    if (type->kind == TYPE_ARRAY && type->array_size == 0) {
        LOG_ERROR("Array with unspecified size detected in parse_var_declaration");
        error_at_current(parser, "Array with unspecified size is not allowed outside of function parameters.");
        parser->had_error = true; // Mark the parser as having encountered an error
        return NULL; // Abort parsing this declaration immediately
    }

    LOG_INFO("parse_var_declaration: Starting variable declaration parsing");
    LOG_INFO("parse_var_declaration: Type kind = %d, Array size = %zu", type->kind, type->array_size);

    consume(parser, TOK_IDENTIFIER, "Expect variable name.");
    char *name = strndup(parser->previous.text, parser->previous.length);

    // Check for array syntax
    if (match(parser, TOK_LBRACKET)) {
        if (match(parser, TOK_RBRACKET)) {
            // Array with unspecified size
            LOG_INFO("Detected array with unspecified size in parse_var_declaration");
            type->kind = TYPE_ARRAY;
            type->array_size = 0;
        } else {
            // Array with specified size
            ASTNode *size_expr = parse_expression(parser);
            if (size_expr && size_expr->type == NODE_LITERAL) {
                LOG_INFO("Detected array with specified size in parse_var_declaration: size=%d", size_expr->data.literal.value.int_value);
                type->kind = TYPE_ARRAY;
                type->array_size = size_expr->data.literal.value.int_value;
                consume(parser, TOK_RBRACKET, "Expect ']' after array size.");
            } else {
                error_at_current(parser, "Array size must be a constant expression.");
                return NULL;
            }
        }
    }

    ASTNode *init = NULL;
    if (match(parser, TOK_EQ)) {
        init = parse_expression(parser);
    }

    consume(parser, TOK_SEMICOLON, "Expect ';' after variable declaration.");
    LOG_INFO("parsed variable declaration: %s", name);
    return create_var_decl_node(name, type, init);
}

static ASTNode* parse_if_statement(Parser *parser) {
    LOG_INFO("current token: %s", token_type_to_string(parser->current.type));

    // Parse the condition
    consume(parser, TOK_LPAREN, "Expect '(' after 'if'.");
    ASTNode *condition = parse_expression(parser);
    consume(parser, TOK_RPAREN, "Expect ')' after condition.");

    // Parse the then branch
    ASTNode *then_branch = parse_statement(parser);

    // Parse the else branch if it exists
    ASTNode *else_branch = NULL;
    if (match(parser, TOK_KW_ELSE)) {
        else_branch = parse_statement(parser);
    }

    // Create and return the if statement node
    ASTNode *if_node = malloc(sizeof(ASTNode));
    if (!if_node) return NULL;
    if_node->type = NODE_IF_STMT;
    if_node->data.if_stmt.condition = condition;
    if_node->data.if_stmt.then_branch = then_branch;
    if_node->data.if_stmt.else_branch = else_branch;
    return if_node;
}

static ASTNode* parse_while_statement(Parser *parser) {
    // Parse the condition
    consume(parser, TOK_LPAREN, "Expect '(' after 'while'.");
    ASTNode *condition = parse_expression(parser);
    consume(parser, TOK_RPAREN, "Expect ')' after condition.");

    // Parse the body
    ASTNode *body = parse_statement(parser);

    // Create and return the while statement node
    ASTNode *while_node = malloc(sizeof(ASTNode));
    if (!while_node) return NULL;
    while_node->type = NODE_WHILE_STMT;
    while_node->data.while_stmt.condition = condition;
    while_node->data.while_stmt.body = body;
    return while_node;
}

static ASTNode* parse_for_statement(Parser *parser) {
    // Parse the initializer
    consume(parser, TOK_LPAREN, "Expect '(' after 'for'.");
    ASTNode *init = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        init = parse_var_declaration(parser);
    } else {
        consume(parser, TOK_SEMICOLON, "Expect ';' after initializer.");
    }

    // Parse the condition
    ASTNode *condition = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        condition = parse_expression(parser);
    }
    consume(parser, TOK_SEMICOLON, "Expect ';' after condition.");

    // Parse the update
    ASTNode *update = NULL;
    if (!check(parser, TOK_RPAREN)) {
        update = parse_expression(parser);
    }
    consume(parser, TOK_RPAREN, "Expect ')' after for clauses.");

    // Parse the body
    ASTNode *body = parse_statement(parser);

    // Create and return the for statement node
    ASTNode *for_node = malloc(sizeof(ASTNode));
    if (!for_node) return NULL;
    for_node->type = NODE_FOR_STMT;
    for_node->data.for_stmt.init = init;
    for_node->data.for_stmt.condition = condition;
    for_node->data.for_stmt.update = update;
    for_node->data.for_stmt.body = body;
    return for_node;
}

static ASTNode* parse_statement(Parser *parser) {
    LOG_INFO("current token: %s", token_type_to_string(parser->current.type));
    if (parser->had_error) {
        synchronize(parser);
        return NULL;
    }

    // Check for variable declarations first
    if (check(parser, TOK_KW_INT) || check(parser, TOK_KW_CHAR) || check(parser, TOK_KW_VOID)) {
        return parse_var_declaration(parser);
    }

    if (match(parser, TOK_KW_RETURN)) {
        ASTNode *value = parse_expression(parser);
        consume(parser, TOK_SEMICOLON, "Expect ';' after return statement.");
        return create_return_node(value);
    }

    if (match(parser, TOK_KW_IF)) {
        return parse_if_statement(parser);
    }

    if (match(parser, TOK_KW_WHILE)) {
        return parse_while_statement(parser);
    }

    if (match(parser, TOK_KW_FOR)) {
        return parse_for_statement(parser);
    }

    if (match(parser, TOK_LBRACE)) {
        LOG_INFO("parsing block");
        return parse_block(parser);
    }

    // Try to parse as expression statement
    ASTNode *expr = parse_expression(parser);
    if (expr && expr->type == NODE_VAR_REF && match(parser, TOK_EQ)) {
        // It's an assignment
        LOG_INFO("Detected assignment: variable=%s", expr->data.var_ref.name);
        ASTNode *value = parse_expression(parser);
        consume(parser, TOK_SEMICOLON, "Expect ';' after assignment.");
        return create_assignment_node(expr->data.var_ref.name, value);
    }

    // Otherwise it's an expression statement
    LOG_INFO("Parsing as expression statement");
    consume(parser, TOK_SEMICOLON, "Expect ';' after expression.");
    return expr;
}

static ASTNode* parse_block(Parser *parser) {
    LOG_INFO("Entering parse_block: current token=%s", token_type_to_string(parser->current.type));

    ASTNode **stmts = NULL;
    size_t count = 0;
    size_t capacity = 0;

    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        LOG_INFO("About to parse a statement: current token=%s", token_type_to_string(parser->current.type));
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) {
            LOG_INFO("parse_statement returned NULL, advancing to avoid infinite loop");
            advance(parser); // Ensure the parser moves forward
            continue;
        }

        if (count >= capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            stmts = realloc(stmts, sizeof(ASTNode*) * capacity);
            if (!stmts) {
                LOG_ERROR("Memory allocation failed in parse_block");
                return NULL;
            }
        }

        stmts[count++] = stmt;
    }

    consume(parser, TOK_RBRACE, "Expect '}' after block.");
    LOG_INFO("Exiting parse_block: current token=%s", token_type_to_string(parser->current.type));
    return create_stmt_list_node(stmts, count);
}

static ASTNode* parse_parameter(Parser *parser) {
    Type *type = parse_type(parser);
    if (!type) return NULL;
    consume(parser, TOK_IDENTIFIER, "Expect parameter name.");
    char *name = strndup(parser->previous.text, parser->previous.length);

    // Check for array syntax
    if (match(parser, TOK_LBRACKET)) {
        if (match(parser, TOK_RBRACKET)) {
            // Array with unspecified size
            Type *array_type = create_array_type(type, 0);
            return create_var_decl_node(name, array_type, NULL);
        } else {
            // Array with specified size
            ASTNode *size_expr = parse_expression(parser);
            if (size_expr && size_expr->type == NODE_LITERAL) {
                Type *array_type = create_array_type(type, size_expr->data.literal.value.int_value);
                consume(parser, TOK_RBRACKET, "Expect ']' after array size.");
                return create_var_decl_node(name, array_type, NULL);
            } else {
                error_at_current(parser, "Array size must be a constant expression.");
                return NULL;
            }
        }
    }

    return create_var_decl_node(name, type, NULL);
}

static ASTNode* parse_parameter_list(Parser *parser) {
    ASTNode **params = NULL;
    size_t count = 0;
    size_t capacity = 0;
    if (!check(parser, TOK_RPAREN)) {
        do {
            if (count >= capacity) {
                capacity = capacity == 0 ? 4 : capacity * 2;
                params = realloc(params, sizeof(ASTNode*) * capacity);
                if (!params) {
                    fprintf(stderr, "Memory allocation failed\n");
                    return NULL;
                }
            }
            
            ASTNode *param = parse_parameter(parser);
            if (!param) break;
            
            params[count++] = param;
        } while (match(parser, TOK_COMMA));
    }
    
    consume(parser, TOK_RPAREN, "Expect ')' after parameters.");
    return create_param_list_node(params, count);
}

static Type* parse_type(Parser *parser) {
    Type *type = NULL;
    if (match(parser, TOK_KW_INT)) {
        type = create_type(TYPE_INT);
    } else if (match(parser, TOK_KW_CHAR)) {
        type = create_type(TYPE_CHAR);
    } else if (match(parser, TOK_KW_VOID)) {
        type = create_type(TYPE_VOID);
    } else {
        error_at_current(parser, "Expect type specifier.");
        return NULL;
    }
    
    // Handle pointers
    while (match(parser, TOK_STAR)) {
        type = create_pointer_type(type);
    }
    
    return type;
}

static ASTNode* parse_function(Parser *parser) {
    LOG_INFO("about to parse a type");
    Type *return_type = parse_type(parser);
    if (!return_type) return NULL;
    consume(parser, TOK_IDENTIFIER, "Expect function name.");
    char *name = strndup(parser->previous.text, parser->previous.length);

    consume(parser, TOK_LPAREN, "Expect '(' after function name.");
    ASTNode *params = parse_parameter_list(parser);

    consume(parser, TOK_LBRACE, "Expect '{' before function body.");
    ASTNode *body = parse_block(parser);

    return create_function_decl_node(name, return_type, params, body);
}

static ASTNode* parse_program(Parser *parser) {
    ASTNode **functions = NULL;
    size_t count = 0;
    size_t capacity = 0;
    while (!check(parser, TOK_EOF)) {
        LOG_INFO("about to parse a function");
        ASTNode *function = parse_function(parser);
        if (!function) continue;
        
        if (count >= capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            functions = realloc(functions, sizeof(ASTNode*) * capacity);
            if (!functions) {
                fprintf(stderr, "Memory allocation failed\n");
                return NULL;
            }
        }
        
        functions[count++] = function;
    }
    
    ASTNode *program = malloc(sizeof(ASTNode));
    if (!program) return NULL;
    
    program->type = NODE_PROGRAM;
    program->data.stmt_list.stmts = functions;
    program->data.stmt_list.count = count;
    return program;
}

ASTNode* parse(Lexer *lexer) {
    Parser parser;
    parser.lexer = lexer;
    parser.had_error = false;
    parser.panic_mode = false;
    advance(&parser);
    ASTNode *program = parse_program(&parser);

    if (parser.had_error) {
        free_ast(program);
        return NULL;
    }

    return program;
}

void free_ast(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_LITERAL:
            if (node->data.literal.type) free(node->data.literal.type);
            break;
            
        case NODE_BINARY_OP:
            free_ast(node->data.binary_op.left);
            free_ast(node->data.binary_op.right);
            break;
            
        case NODE_UNARY_OP:
            free_ast(node->data.unary_op.operand);
            break;

        case NODE_VAR_REF:
            free(node->data.var_ref.name);
            if (node->data.var_ref.type) free(node->data.var_ref.type);
            break;
            
        case NODE_VAR_DECL:
            free(node->data.var_decl.name);
            if (node->data.var_decl.type) free(node->data.var_decl.type);
            free_ast(node->data.var_decl.init_value);
            break;
            
        case NODE_ASSIGNMENT:
            free(node->data.assignment.name);
            free_ast(node->data.assignment.value);
            break;
        
        case NODE_IF_STMT:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_branch);
            free_ast(node->data.if_stmt.else_branch); // This can be NULL
            break;

        case NODE_WHILE_STMT:
            free_ast(node->data.while_stmt.condition);
            free_ast(node->data.while_stmt.body);
            break;

        case NODE_FOR_STMT:
            free_ast(node->data.for_stmt.init);      // Can be NULL
            free_ast(node->data.for_stmt.condition); // Can be NULL
            free_ast(node->data.for_stmt.update);    // Can be NULL
            free_ast(node->data.for_stmt.body);
            break;

        case NODE_FUNCTION_DECL:
            free(node->data.function_decl.name);
            if (node->data.function_decl.return_type) free(node->data.function_decl.return_type);
            free_ast(node->data.function_decl.params);
            free_ast(node->data.function_decl.body);
            break;
            
        case NODE_RETURN:
            free_ast(node->data.return_stmt.value);
            break;
            
        case NODE_PARAM_LIST:
            for (size_t i = 0; i < node->data.param_list.count; i++) {
                free_ast(node->data.param_list.params[i]);
            }
            free(node->data.param_list.params);
            break;
            
        case NODE_STMT_LIST:
            for (size_t i = 0; i < node->data.stmt_list.count; i++) {
                free_ast(node->data.stmt_list.stmts[i]);
            }
            free(node->data.stmt_list.stmts);
            break;
            
        case NODE_TYPE_SPECIFIER:
            if (node->data.type_spec.type) free(node->data.type_spec.type);
            break;
            
        case NODE_PROGRAM:
            for (size_t i = 0; i < node->data.stmt_list.count; i++) {
                free_ast(node->data.stmt_list.stmts[i]);
            }
            free(node->data.stmt_list.stmts);
            break;

        case NODE_FUNCTION_CALL:
            free(node->data.function_call.name);
            for (size_t i = 0; i < node->data.function_call.arg_count; i++) {
                free_ast(node->data.function_call.args[i]);
            }
            free(node->data.function_call.args);
            break;

        case INVALID_NODE_TYPE:
        case UNKNOWN_NODE_TYPE:
            LOG_ERROR("Invalid or unknown node type encountered during AST cleanup\n");
            break;
    }
    
    free(node);
}

const char *node_type_to_string(NodeType type) {
    static const char *names[] = {
        [NODE_PROGRAM] = "PROGRAM",
        [NODE_FUNCTION_DECL] = "FUNCTION_DECL",
        [NODE_VAR_DECL] = "VAR_DECL",
        [NODE_VAR_REF] = "VAR_REF",
        [NODE_ASSIGNMENT] = "ASSIGNMENT",
        [NODE_BINARY_OP] = "BINARY_OP",
        [NODE_UNARY_OP] = "UNARY_OP",
        [NODE_RETURN] = "RETURN",
        [NODE_LITERAL] = "LITERAL",
        [NODE_PARAM_LIST] = "PARAM_LIST",
        [NODE_STMT_LIST] = "STMT_LIST",
        [NODE_TYPE_SPECIFIER] = "TYPE_SPECIFIER",
        [NODE_IF_STMT] = "IF_STMT",
        [NODE_WHILE_STMT] = "WHILE_STMT",
        [NODE_FOR_STMT] = "FOR_STMT",
        [NODE_FUNCTION_CALL] = "FUNCTION_CALL"
    };

    // Handle invalid type values
    if (type < 0 || type >= sizeof(names)/sizeof(names[0])) {
        return "INVALID_NODE_TYPE";
    }

    // Handle undefined entries (shouldn't happen if array is complete)
    return names[type] ? names[type] : "UNKNOWN_NODE_TYPE";
}

void print_ast(ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_PROGRAM:
            printf("Program\n");
            for (size_t i = 0; i < node->data.stmt_list.count; i++) {
                print_ast(node->data.stmt_list.stmts[i], indent + 1);
            }
            break;

        case NODE_FUNCTION_DECL:
            printf("Function: %s\n", node->data.function_decl.name);
            if (node->data.function_decl.params) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Parameters:\n");
                print_ast(node->data.function_decl.params, indent + 2);
            }
            if (node->data.function_decl.body) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Body:\n");
                print_ast(node->data.function_decl.body, indent + 2);
            }
            break;

        case NODE_VAR_DECL:
            printf("VarDecl: %s\n", node->data.var_decl.name);
            if (node->data.var_decl.init_value) {
                print_ast(node->data.var_decl.init_value, indent + 1);
            }
            break;

        case NODE_RETURN:
            printf("Return\n");
            print_ast(node->data.return_stmt.value, indent + 1);
            break;

        case NODE_LITERAL:
            if (node->data.literal.type->kind == TYPE_POINTER) {
                printf("Literal (pointer): %p\n", node->data.literal.value.ptr_value);
            } else {
                printf("Literal: %d\n", node->data.literal.value.int_value);
            }
            break;

        case NODE_BINARY_OP: {
            const char *op_str = token_type_to_string(node->data.binary_op.op);
            printf("BinaryOp: %s\n", op_str);
            print_ast(node->data.binary_op.left, indent + 1);
            print_ast(node->data.binary_op.right, indent + 1);
            break;
        }

        case NODE_UNARY_OP: {
            const char *op_str = token_type_to_string(node->data.unary_op.op);
            printf("UnaryOp: %s (%s)\n", op_str, node->data.unary_op.is_prefix ? "prefix" : "postfix");
            print_ast(node->data.unary_op.operand, indent + 1);
            break;
        }

        case NODE_VAR_REF:
            printf("VarRef: %s\n", node->data.var_ref.name);
            break;
            
        case NODE_PARAM_LIST:
            printf("ParamList of size %zu\n", node->data.param_list.count);
            for (size_t i = 0; i < node->data.param_list.count; i++) {
                print_ast(node->data.param_list.params[i], indent);
            }
            break;
            
        case NODE_STMT_LIST:
            printf("StmtList of size %zu\n", node->data.stmt_list.count);
            for (size_t i = 0; i < node->data.stmt_list.count; i++) {
                print_ast(node->data.stmt_list.stmts[i], indent);
            }
            break;

        case NODE_IF_STMT:
            printf("IfStatement\n");
            // Print condition
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Condition:\n");
            print_ast(node->data.if_stmt.condition, indent + 2);
            
            // Print then branch
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Then:\n");
            print_ast(node->data.if_stmt.then_branch, indent + 2);
            
            // Print else branch if exists
            if (node->data.if_stmt.else_branch) {
                for (int i = 0; i < indent+1; i++) printf("  ");
                printf("Else:\n");
                print_ast(node->data.if_stmt.else_branch, indent + 2);
            }
            break;

        case NODE_WHILE_STMT:
            printf("WhileStatement\n");
            // Print condition
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Condition:\n");
            print_ast(node->data.while_stmt.condition, indent + 2);
            
            // Print body
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Body:\n");
            print_ast(node->data.while_stmt.body, indent + 2);
            break;

        case NODE_FOR_STMT:
            printf("ForStatement\n");
            // Print init if exists
            if (node->data.for_stmt.init) {
                for (int i = 0; i < indent+1; i++) printf("  ");
                printf("Initializer:\n");
                print_ast(node->data.for_stmt.init, indent + 2);
            }
            
            // Print condition if exists
            if (node->data.for_stmt.condition) {
                for (int i = 0; i < indent+1; i++) printf("  ");
                printf("Condition:\n");
                print_ast(node->data.for_stmt.condition, indent + 2);
            }
            
            // Print update if exists
            if (node->data.for_stmt.update) {
                for (int i = 0; i < indent+1; i++) printf("  ");
                printf("Update:\n");
                print_ast(node->data.for_stmt.update, indent + 2);
            }

            // Print body
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Body:\n");
            print_ast(node->data.for_stmt.body, indent + 2);
            break;
            
        case NODE_TYPE_SPECIFIER:
            printf("TypeSpecifier: ");
            switch (node->data.type_spec.type->kind) {
                case TYPE_INT: printf("int\n"); break;
                case TYPE_CHAR: printf("char\n"); break;
                case TYPE_VOID: printf("void\n"); break;
                case TYPE_POINTER: 
                    printf("pointer to ");
                    print_ast(create_type_spec_node(node->data.type_spec.type->base), 0);
                    break;
                case TYPE_ARRAY:
                    printf("array[%zu] of ", node->data.type_spec.type->array_size);
                    print_ast(create_type_spec_node(node->data.type_spec.type->base), 0);
                    break;
                default: printf("unknown\n"); break;
            }
            break;

        case NODE_FUNCTION_CALL:
            printf("FunctionCall: %s\n", node->data.function_call.name);
            for (size_t i = 0; i < node->data.function_call.arg_count; i++) {
                print_ast(node->data.function_call.args[i], indent + 1);
            }
            break;
            
        case INVALID_NODE_TYPE:
        case UNKNOWN_NODE_TYPE:
            LOG_ERROR("Invalid or unknown node type encountered during AST print\n");
            break;

        default:
            printf("Unknown node type\n");
            break;
    }
}