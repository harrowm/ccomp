#include "tac.h"
#include "lexer.h" // Include token definitions like TOK_PLUS
#include "debug.h" // Include debug.h for logging macros
#include "parser.h" // For node_type_to_string
#include <string.h>

// Helper function to create a new TAC instruction
static TAC *create_tac(TACType type, const char *result, const char *arg1, const char *arg2, const char *op, const char *label) {
    TAC *tac = malloc(sizeof(TAC));
    if (!tac) {
        fprintf(stderr, "Error: Unable to allocate memory for TAC instruction\n");
        exit(EXIT_FAILURE);
    }
    tac->type = type;
    tac->result = result ? strdup(result) : NULL;
    tac->arg1 = arg1 ? strdup(arg1) : NULL;
    tac->arg2 = arg2 ? strdup(arg2) : NULL;
    tac->op = op ? strdup(op) : NULL;
    tac->label = label ? strdup(label) : NULL;
    tac->int_value = 0; // Default to 0
    tac->ptr_value = NULL; // Default to NULL
    tac->next = NULL;
    return tac;
}

// Updated helper function to convert token types to string
static const char *operator_to_string(int op) {
    switch (op) {
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_AMP_AMP: return "&&";
        case TOK_PIPE_PIPE: return "||";
        case TOK_PLUS_EQ: return "+=";
        case TOK_MINUS_EQ: return "-=";
        case TOK_STAR_EQ: return "*=";
        case TOK_SLASH_EQ: return "/=";
        case TOK_PLUS_PLUS: return "++";
        case TOK_MINUS_MINUS: return "--";
        case TOK_GT: return ">";
        case TOK_LT: return "<";
        case TOK_LT_EQ: return "<=";
        case TOK_GT_EQ: return ">=";
        case TOK_EQ_EQ: return "==";
        case TOK_BANG_EQ: return "!=";
        case TOK_AMP: return "&";
        case TOK_PIPE: return "|";
        case TOK_CARET: return "^";
        case TOK_AMP_EQ: return "&=";
        case TOK_PIPE_EQ: return "|=";
        case TOK_CARET_EQ: return "^=";
        case TOK_UNKNOWN: 
        default: return "<unknown>";
    }
}

// Helper function to convert NodeType to string
static const char *cfg_node_type_to_string(NodeType type) {
    return node_type_to_string(type);
}

// Global variable to track the last temporary variable
static char last_temp_var[32] = "";

// Update the last_temp_var whenever a new temporary variable is created
#define UPDATE_LAST_TEMP_VAR(temp) snprintf(last_temp_var, sizeof(last_temp_var), "%s", temp)

// Refactor the processing of statements into a separate function
static void process_statement(ASTNode *stmt, BasicBlock *block, size_t stmt_index, TAC **tail) {
    LOG_INFO("Processing statement %zu in block %zu of type %s", stmt_index, block->id, cfg_node_type_to_string(stmt->type));
    TAC *new_tac = NULL;

    switch (stmt->type) {
        case NODE_VAR_DECL:
            LOG_INFO("Variable declaration: %s", stmt->data.var_decl.name);
            if (stmt->data.var_decl.init_value == NULL) {
                LOG_INFO("Detected φ-function for variable: %s", stmt->data.var_decl.name);
                new_tac = create_tac(TAC_PHI, stmt->data.var_decl.name, NULL, NULL, NULL, NULL);
            } else {
                if (stmt->data.var_decl.init_value->type == NODE_LITERAL) {
                    new_tac = create_tac(TAC_ASSIGN, stmt->data.var_decl.name, NULL, NULL, NULL, NULL);
                    new_tac->int_value = stmt->data.var_decl.init_value->data.literal.value.int_value;
                } else if (stmt->data.var_decl.init_value->type == NODE_VAR_REF) {
                    new_tac = create_tac(TAC_ASSIGN, stmt->data.var_decl.name, stmt->data.var_decl.init_value->data.var_ref.name, NULL, NULL, NULL);
                } else {
                    LOG_ERROR("Unsupported initializer type for variable declaration");
                }
            }
            break;

        case NODE_ASSIGNMENT:
            LOG_INFO("Assignment: %s", stmt->data.assignment.name);
            if (stmt->data.assignment.value) {
                if (stmt->data.assignment.value->type == NODE_LITERAL) {
                    new_tac = create_tac(TAC_ASSIGN, stmt->data.assignment.name, NULL, NULL, NULL, NULL);
                    new_tac->int_value = stmt->data.assignment.value->data.literal.value.int_value;
                } else if (stmt->data.assignment.value->type == NODE_VAR_REF) {
                    new_tac = create_tac(TAC_ASSIGN, stmt->data.assignment.name, stmt->data.assignment.value->data.var_ref.name, NULL, NULL, NULL);
                } else if (stmt->data.assignment.value->type == NODE_BINARY_OP) {
                    char temp_var[32];
                    snprintf(temp_var, sizeof(temp_var), "t%zu", block->id * 100 + stmt_index);
                    const char *left_operand = stmt->data.assignment.value->data.binary_op.left->data.var_ref.name;
                    const char *right_operand;
                    if (stmt->data.assignment.value->data.binary_op.right->type == NODE_LITERAL) {
                        char literal_value[32];
                        snprintf(literal_value, sizeof(literal_value), "%d", stmt->data.assignment.value->data.binary_op.right->data.literal.value.int_value);
                        right_operand = strdup(literal_value);
                    } else {
                        right_operand = stmt->data.assignment.value->data.binary_op.right->data.var_ref.name;
                    }
                    new_tac = create_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, operator_to_string(stmt->data.assignment.value->data.binary_op.op), NULL);
                    UPDATE_LAST_TEMP_VAR(temp_var);
                    TAC *assign_tac = create_tac(TAC_ASSIGN, stmt->data.assignment.name, temp_var, NULL, NULL, NULL);
                    (*tail)->next = new_tac;
                    *tail = new_tac;
                    new_tac = assign_tac;
                } else {
                    LOG_ERROR("Unsupported value type for assignment");
                }
            }
            break;

        case NODE_BINARY_OP:
            LOG_INFO("Binary operation");
            if (stmt->data.binary_op.left && stmt->data.binary_op.right) {
                char temp_var[32];
                snprintf(temp_var, sizeof(temp_var), "t%zu", block->id * 100 + stmt_index);
                const char *left_operand = stmt->data.binary_op.left->data.var_ref.name;
                const char *right_operand;
                if (stmt->data.binary_op.right->type == NODE_LITERAL) {
                    char literal_value[32];
                    snprintf(literal_value, sizeof(literal_value), "%d", stmt->data.binary_op.right->data.literal.value.int_value);
                    right_operand = strdup(literal_value);
                } else {
                    right_operand = stmt->data.binary_op.right->data.var_ref.name;
                }
                const char *op = operator_to_string(stmt->data.binary_op.op);
                new_tac = create_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);
                LOG_INFO("Generated TAC for binary operation: %s = %s %s %s", temp_var, left_operand, op, right_operand);
                (*tail)->next = new_tac;
                *tail = new_tac;
                UPDATE_LAST_TEMP_VAR(temp_var);
            } else {
                LOG_ERROR("Binary operation has NULL operands");
            }
            break;

        case NODE_RETURN:
            LOG_INFO("Return statement");
            if (stmt->data.return_stmt.value) {
                if (stmt->data.return_stmt.value->type == NODE_LITERAL) {
                    new_tac = create_tac(TAC_RETURN, NULL, NULL, NULL, NULL, NULL);
                    new_tac->int_value = stmt->data.return_stmt.value->data.literal.value.int_value;
                } else if (stmt->data.return_stmt.value->type == NODE_VAR_REF) {
                    new_tac = create_tac(TAC_RETURN, stmt->data.return_stmt.value->data.var_ref.name, NULL, NULL, NULL, NULL);
                } else {
                    LOG_ERROR("Unsupported return value type");
                }
            }
            break;

        default:
            LOG_INFO("Unsupported AST node type in CFG to TAC conversion: %d", stmt->type);
            break;
    }

    if (new_tac) {
        LOG_INFO("Generated TAC: type=%d, result=%s, arg1=%s, arg2=%s, op=%s, label=%s", 
                 new_tac->type, new_tac->result, new_tac->arg1, new_tac->arg2, new_tac->op, new_tac->label);
        (*tail)->next = new_tac;
        *tail = new_tac;
    }
}

// Updated handling for CFG block types in TAC generation
static void traverse_cfg(BasicBlock *block, bool *visited, TAC **head, TAC **tail) {
    if (visited[block->id]) {
        return; // Block already visited
    }
    visited[block->id] = true;

    // Add a label for the block after processing its statements
    char label[32];
    snprintf(label, sizeof(label), "L%zu", block->id);
    LOG_INFO("Adding label: %s", label);
    TAC *label_tac = create_tac(TAC_LABEL, NULL, NULL, NULL, NULL, label);
    if (!*head) {
        *head = *tail = label_tac;
    } else {
        (*tail)->next = label_tac;
        *tail = label_tac;
    }

    // Handle block types for control flow
    if (block->type == BLOCK_IF_THEN) {
        // Use the last temporary variable for the condition
        char temp_var[32];
        snprintf(temp_var, sizeof(temp_var), "%s", last_temp_var);

        // // Generate a jump in case the IF fails
        if (block->preds[0]->succ_count == 2) { // There is an else block
            char else_label[32];
            snprintf(else_label, sizeof(else_label), "L%zu", block->preds[0]->succs[1]->id);
            TAC *if_goto_tac = create_tac(TAC_IF_GOTO, NULL, temp_var, NULL, NULL, else_label);
            LOG_INFO("Adding if-goto TAC for condition: %s", else_label);
            (*tail)->next = if_goto_tac;
            *tail = if_goto_tac;
        } else {
            // Add a goto to the successor block if no else block exists
            char successor_label[32];
            snprintf(successor_label, sizeof(successor_label), "L%zu", block->succs[0]->id);
            TAC *goto_tac = create_tac(TAC_GOTO, NULL, NULL, NULL, NULL, successor_label);
            LOG_INFO("Adding goto TAC for successor block: %s", successor_label);
            (*tail)->next = goto_tac;
            *tail = goto_tac;
        }
    }

    // Process statements in the block
    for (size_t j = 0; j < block->stmt_count; j++) {
        process_statement(block->stmts[j], block, j, tail);
    }

    // Ensure a jump to the successor block rfor non IF / ELSE blocks, belt and braces ..
    if (block->succ_count == 1) {
        char successor_label[32];
        snprintf(successor_label, sizeof(successor_label), "L%zu", block->succs[0]->id);
        TAC *goto_tac = create_tac(TAC_GOTO, NULL, NULL, NULL, NULL, successor_label);
        LOG_INFO("Adding unconditional goto TAC for successor block: %s", successor_label);
        (*tail)->next = goto_tac;
        *tail = goto_tac;
    }

    // Recursively process successor blocks
    for (size_t i = 0; i < block->succ_count; i++) {
        traverse_cfg(block->succs[i], visited, head, tail);
    }
}

// Convert a CFG to TAC
TAC *cfg_to_tac(CFG *cfg) {
    LOG_INFO("CFG to TAC conversion started");

    TAC *head = NULL, *tail = NULL;

    bool *visited = calloc(cfg->block_count, sizeof(bool));
    if (!visited) {
        fprintf(stderr, "Error: Unable to allocate memory for visited array\n");
        exit(EXIT_FAILURE);
    }

    traverse_cfg(cfg->blocks[0], visited, &head, &tail);

    free(visited);

    LOG_INFO("CFG to TAC conversion completed");
    return head;
}

// Print TAC instructions for debugging
void print_tac(TAC *tac, FILE *stream) {
    while (tac) {
        switch (tac->type) {
            case TAC_ASSIGN:
                if (tac->int_value != 0) {
                    fprintf(stream, "%s = %d\n", tac->result, tac->int_value);
                } else if (tac->ptr_value != NULL) {
                    fprintf(stream, "%s = %p\n", tac->result, tac->ptr_value);
                } else {
                    fprintf(stream, "%s = %s\n", tac->result, tac->arg1);
                }
                break;
            case TAC_BINARY_OP:
                fprintf(stream, "%s = %s %s %s\n", tac->result, tac->arg1, tac->op, tac->arg2);
                break;
            case TAC_UNARY_OP:
                fprintf(stream, "%s = %s %s\n", tac->result, tac->op, tac->arg1);
                break;
            case TAC_LABEL:
                fprintf(stream, "%s:\n", tac->label);
                break;
            case TAC_GOTO:
                fprintf(stream, "goto %s\n", tac->label);
                break;
            case TAC_IF_GOTO:
                fprintf(stream, "if not %s goto %s\n", tac->arg1, tac->label);
                break;
            case TAC_RETURN:
                if (tac->int_value != 0) {
                    fprintf(stream, "return %d\n", tac->int_value);
                } else if (tac->ptr_value != NULL) {
                    fprintf(stream, "return %p\n", tac->ptr_value);
                } else {
                    fprintf(stream, "return %s\n", tac->result);
                }
                break;
            case TAC_PHI:
                fprintf(stream, "%s = phi(...)\n", tac->result);
                break;
            default:
                fprintf(stream, "Unknown TAC type\n");
                break;
        }
        tac = tac->next;
    }
}

// Free TAC instructions
void free_tac(TAC *tac) {
    while (tac) {
        TAC *next = tac->next;
        free(tac->result);
        free(tac->arg1);
        free(tac->arg2);
        free(tac->op);
        free(tac->label);
        free(tac);
        tac = next;
    }
}