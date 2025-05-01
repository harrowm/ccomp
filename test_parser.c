#include "parser.h"
#include "lexer.h"
#include "minunit.h"
#include "debug.h"
#include "ast.h"
#include <string.h>

MU_TEST(test_parser_simple_program) {
    LOG_TRACE("Running test_parser_simple_program");
    const char *input = "int main() { return 1 + 2; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);
    mu_assert_int_eq(1, ast->data.stmt_list.count);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);
    mu_assert_string_eq("main", function->data.function_decl.name);

    ASTNode *body = function->data.function_decl.body;
    mu_assert_int_eq(NODE_STMT_LIST, body->type);
    mu_assert_int_eq(1, body->data.stmt_list.count);

    ASTNode *return_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_RETURN, return_stmt->type);

    ASTNode *binary_op = return_stmt->data.return_stmt.value;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op->type);
    mu_assert_int_eq(TOK_PLUS, binary_op->data.binary_op.op);
    mu_assert_int_eq(NODE_LITERAL, binary_op->data.binary_op.left->type);
    mu_assert_int_eq(1, binary_op->data.binary_op.left->data.literal.value.int_value);
    mu_assert_int_eq(NODE_LITERAL, binary_op->data.binary_op.right->type);
    mu_assert_int_eq(2, binary_op->data.binary_op.right->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_nested_program) {
    LOG_TRACE("Running test_parser_nested_program");
    const char *input = "int main() { return (1 + 2) * 3; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);
    mu_assert_int_eq(1, ast->data.stmt_list.count);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);
    mu_assert_string_eq("main", function->data.function_decl.name);

    ASTNode *body = function->data.function_decl.body;
    mu_assert_int_eq(NODE_STMT_LIST, body->type);
    mu_assert_int_eq(1, body->data.stmt_list.count);

    ASTNode *return_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_RETURN, return_stmt->type);

    ASTNode *binary_op = return_stmt->data.return_stmt.value;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op->type);
    mu_assert_int_eq(TOK_STAR, binary_op->data.binary_op.op);

    ASTNode *left = binary_op->data.binary_op.left;
    mu_assert_int_eq(NODE_BINARY_OP, left->type);
    mu_assert_int_eq(TOK_PLUS, left->data.binary_op.op);
    mu_assert_int_eq(NODE_LITERAL, left->data.binary_op.left->type);
    mu_assert_int_eq(1, left->data.binary_op.left->data.literal.value.int_value);
    mu_assert_int_eq(NODE_LITERAL, left->data.binary_op.right->type);
    mu_assert_int_eq(2, left->data.binary_op.right->data.literal.value.int_value);

    mu_assert_int_eq(NODE_LITERAL, binary_op->data.binary_op.right->type);
    mu_assert_int_eq(3, binary_op->data.binary_op.right->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_unary_operations) {
    LOG_TRACE("Running test_parser_unary_operations");
    const char *input = "int main() { return -1 + !2; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *return_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_RETURN, return_stmt->type);

    ASTNode *binary_op = return_stmt->data.return_stmt.value;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op->type);
    mu_assert_int_eq(TOK_PLUS, binary_op->data.binary_op.op);

    ASTNode *left = binary_op->data.binary_op.left;
    mu_assert_int_eq(NODE_UNARY_OP, left->type);
    mu_assert_int_eq(TOK_MINUS, left->data.unary_op.op);
    mu_assert_int_eq(NODE_LITERAL, left->data.unary_op.operand->type);
    mu_assert_int_eq(1, left->data.unary_op.operand->data.literal.value.int_value);

    ASTNode *right = binary_op->data.binary_op.right;
    mu_assert_int_eq(NODE_UNARY_OP, right->type);
    mu_assert_int_eq(TOK_BANG, right->data.unary_op.op);
    mu_assert_int_eq(NODE_LITERAL, right->data.unary_op.operand->type);
    mu_assert_int_eq(2, right->data.unary_op.operand->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_variable_declaration) {
    LOG_TRACE("Running test_parser_variable_declaration");
    const char *input = "int main() { int x = 42; return x; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    mu_assert_int_eq(NODE_STMT_LIST, body->type);

    ASTNode *var_decl = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl->type);
    mu_assert_string_eq("x", var_decl->data.var_decl.name);
    mu_assert_int_eq(NODE_LITERAL, var_decl->data.var_decl.init_value->type);
    mu_assert_int_eq(42, var_decl->data.var_decl.init_value->data.literal.value.int_value);

    ASTNode *return_stmt = body->data.stmt_list.stmts[1];
    mu_assert_int_eq(NODE_RETURN, return_stmt->type);
    mu_assert_int_eq(NODE_VAR_REF, return_stmt->data.return_stmt.value->type);
    mu_assert_string_eq("x", return_stmt->data.return_stmt.value->data.var_ref.name);

    free_ast(ast);
}

MU_TEST(test_parser_if_statement) {
    LOG_TRACE("Running test_parser_if_statement");
    const char *input = "int main() { if (1) return 42; else return 0; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *if_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_IF_STMT, if_stmt->type);

    ASTNode *condition = if_stmt->data.if_stmt.condition;
    mu_assert_int_eq(NODE_LITERAL, condition->type);
    mu_assert_int_eq(1, condition->data.literal.value.int_value);

    ASTNode *then_branch = if_stmt->data.if_stmt.then_branch;
    mu_assert_int_eq(NODE_RETURN, then_branch->type);
    mu_assert_int_eq(42, then_branch->data.return_stmt.value->data.literal.value.int_value);

    ASTNode *else_branch = if_stmt->data.if_stmt.else_branch;
    mu_assert_int_eq(NODE_RETURN, else_branch->type);
    mu_assert_int_eq(0, else_branch->data.return_stmt.value->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_while_statement) {
    LOG_TRACE("Running test_parser_while_statement");
    const char *input = "int main() { while (1) return 42; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *while_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_WHILE_STMT, while_stmt->type);

    ASTNode *condition = while_stmt->data.while_stmt.condition;
    mu_assert_int_eq(NODE_LITERAL, condition->type);
    mu_assert_int_eq(1, condition->data.literal.value.int_value);

    ASTNode *loop_body = while_stmt->data.while_stmt.body;
    mu_assert_int_eq(NODE_RETURN, loop_body->type);
    mu_assert_int_eq(42, loop_body->data.return_stmt.value->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_for_statement) {
    LOG_TRACE("Running test_parser_for_statement");
    const char *input = "int main() { for (int i = 0; i < 10; i++) return i; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *for_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FOR_STMT, for_stmt->type);

    ASTNode *init = for_stmt->data.for_stmt.init;
    mu_assert_int_eq(NODE_VAR_DECL, init->type);
    mu_assert_string_eq("i", init->data.var_decl.name);
    mu_assert_int_eq(0, init->data.var_decl.init_value->data.literal.value.int_value);

    ASTNode *condition = for_stmt->data.for_stmt.condition;
    mu_assert_int_eq(NODE_BINARY_OP, condition->type);
    mu_assert_int_eq(TOK_LT, condition->data.binary_op.op);

    ASTNode *update = for_stmt->data.for_stmt.update;
    mu_assert_int_eq(NODE_UNARY_OP, update->type);
    mu_assert_int_eq(TOK_PLUS_PLUS, update->data.unary_op.op);

    ASTNode *loop_body = for_stmt->data.for_stmt.body;
    mu_assert_int_eq(NODE_RETURN, loop_body->type);
    mu_assert_int_eq(NODE_VAR_REF, loop_body->data.return_stmt.value->type);
    mu_assert_string_eq("i", loop_body->data.return_stmt.value->data.var_ref.name);

    free_ast(ast);
}

MU_TEST(test_parser_function_call) {
    LOG_TRACE("Running test_parser_function_call");
    const char *input = "int main() { foo(1, 2, 3); }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *call_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_CALL, call_stmt->type);
    mu_assert_string_eq("foo", call_stmt->data.function_call.name);
    mu_assert_int_eq(3, call_stmt->data.function_call.arg_count);

    ASTNode *arg1 = call_stmt->data.function_call.args[0];
    mu_assert_int_eq(NODE_LITERAL, arg1->type);
    mu_assert_int_eq(1, arg1->data.literal.value.int_value);

    ASTNode *arg2 = call_stmt->data.function_call.args[1];
    mu_assert_int_eq(NODE_LITERAL, arg2->type);
    mu_assert_int_eq(2, arg2->data.literal.value.int_value);

    ASTNode *arg3 = call_stmt->data.function_call.args[2];
    mu_assert_int_eq(NODE_LITERAL, arg3->type);
    mu_assert_int_eq(3, arg3->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_assignment) {
    LOG_TRACE("Running test_parser_assignment");
    const char *input = "int main() { x = 42; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *assignment = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_ASSIGNMENT, assignment->type);
    mu_assert_string_eq("x", assignment->data.assignment.name);
    mu_assert_int_eq(NODE_LITERAL, assignment->data.assignment.value->type);
    mu_assert_int_eq(42, assignment->data.assignment.value->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_parser_empty_block) {
    LOG_TRACE("Running test_parser_empty_block");
    const char *input = "int main() { {} }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    mu_assert_int_eq(NODE_STMT_LIST, body->type);
    mu_assert_int_eq(1, body->data.stmt_list.count);

    ASTNode *empty_block = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_STMT_LIST, empty_block->type);
    mu_assert_int_eq(0, empty_block->data.stmt_list.count);

    free_ast(ast);
}

MU_TEST(test_parser_invalid_token) {
    LOG_TRACE("Running test_parser_invalid_token");
    const char *input = "int main() { @ }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast == NULL, "AST should be NULL for invalid input");
}

MU_TEST(test_error_handling) {
    LOG_TRACE("Running test_error_handling");
    const char *source = "int x = ;";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for invalid variable declaration");

    source = "int x = 42";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for missing semicolon");

    source = "unknown_type x;";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for invalid type specifier");
}

MU_TEST(test_error_handling_edge_cases) {
    LOG_TRACE("Running test_error_handling_edge_cases");

    // Test unmatched parentheses
    const char *source = "int x = (42;";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for unmatched parentheses");

    // Test missing semicolon
    source = "int x = 42";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for missing semicolon");

    // Test invalid token
    source = "int x = 42 @;";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for invalid token");

    // Test array declaration with unspecified size
    source = "int arr[];";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for array with unspecified size");
}

MU_TEST(test_type_parsing) {
    LOG_TRACE("Running test_type_parsing");
    const char *input = "int main() { char x; void *y; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *var_decl1 = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl1->type);
    mu_assert_string_eq("x", var_decl1->data.var_decl.name);
    mu_assert_int_eq(TYPE_CHAR, var_decl1->data.var_decl.type->kind);

    ASTNode *var_decl2 = body->data.stmt_list.stmts[1];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl2->type);
    mu_assert_string_eq("y", var_decl2->data.var_decl.name);
    mu_assert_int_eq(TYPE_POINTER, var_decl2->data.var_decl.type->kind);
    mu_assert_int_eq(TYPE_VOID, var_decl2->data.var_decl.type->base->kind);

    free_ast(ast);
}

MU_TEST(test_unary_operations) {
    LOG_TRACE("Running test_unary_operations");
    const char *input = "int main() { int x = 0; ++x; x--; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *var_decl = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl->type);
    mu_assert_string_eq("x", var_decl->data.var_decl.name);
    mu_assert_int_eq(0, var_decl->data.var_decl.init_value->data.literal.value.int_value);

    ASTNode *prefix_inc = body->data.stmt_list.stmts[1];
    mu_assert_int_eq(NODE_UNARY_OP, prefix_inc->type);
    mu_assert_int_eq(TOK_PLUS_PLUS, prefix_inc->data.unary_op.op);
    mu_assert_string_eq("x", prefix_inc->data.unary_op.operand->data.var_ref.name);

    ASTNode *postfix_dec = body->data.stmt_list.stmts[2];
    mu_assert_int_eq(NODE_UNARY_OP, postfix_dec->type);
    mu_assert_int_eq(TOK_MINUS_MINUS, postfix_dec->data.unary_op.op);
    mu_assert_string_eq("x", postfix_dec->data.unary_op.operand->data.var_ref.name);

    free_ast(ast);
}

MU_TEST(test_binary_operations) {
    LOG_TRACE("Running test_binary_operations");
    const char *input = "int main() { int x = 1 + 2 * 3 - 4 / 5; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *var_decl = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl->type);
    mu_assert_string_eq("x", var_decl->data.var_decl.name);

    ASTNode *binary_op1 = var_decl->data.var_decl.init_value;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op1->type);
    mu_assert_int_eq(TOK_MINUS, binary_op1->data.binary_op.op);

    ASTNode *binary_op2 = binary_op1->data.binary_op.left;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op2->type);
    mu_assert_int_eq(TOK_PLUS, binary_op2->data.binary_op.op);

    ASTNode *literal1 = binary_op2->data.binary_op.left;
    mu_assert_int_eq(NODE_LITERAL, literal1->type);
    mu_assert_int_eq(1, literal1->data.literal.value.int_value);

    ASTNode *binary_op3 = binary_op2->data.binary_op.right;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op3->type);
    mu_assert_int_eq(TOK_STAR, binary_op3->data.binary_op.op);

    ASTNode *literal2 = binary_op3->data.binary_op.left;
    mu_assert_int_eq(NODE_LITERAL, literal2->type);
    mu_assert_int_eq(2, literal2->data.literal.value.int_value);

    ASTNode *literal3 = binary_op3->data.binary_op.right;
    mu_assert_int_eq(NODE_LITERAL, literal3->type);
    mu_assert_int_eq(3, literal3->data.literal.value.int_value);

    ASTNode *binary_op4 = binary_op1->data.binary_op.right;
    mu_assert_int_eq(NODE_BINARY_OP, binary_op4->type);
    mu_assert_int_eq(TOK_SLASH, binary_op4->data.binary_op.op);

    ASTNode *literal4 = binary_op4->data.binary_op.left;
    mu_assert_int_eq(NODE_LITERAL, literal4->type);
    mu_assert_int_eq(4, literal4->data.literal.value.int_value);

    ASTNode *literal5 = binary_op4->data.binary_op.right;
    mu_assert_int_eq(NODE_LITERAL, literal5->type);
    mu_assert_int_eq(5, literal5->data.literal.value.int_value);

    free_ast(ast);
}

MU_TEST(test_control_structures) {
    LOG_TRACE("Running test_control_structures");
    const char *input = "int main() { if (1) { int x = 0; } while (0) { int y = 1; } for (int i = 0; i < 10; i++) { int z = 2; } }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *if_stmt = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_IF_STMT, if_stmt->type);

    ASTNode *while_stmt = body->data.stmt_list.stmts[1];
    mu_assert_int_eq(NODE_WHILE_STMT, while_stmt->type);

    ASTNode *for_stmt = body->data.stmt_list.stmts[2];
    mu_assert_int_eq(NODE_FOR_STMT, for_stmt->type);

    free_ast(ast);
}

MU_TEST(test_function_parsing) {
    LOG_TRACE("Running test_function_parsing");
    const char *source = "int add(int a, int b) { return a + b; }";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program != NULL, "Expected parsing to succeed for valid function declaration");

    source = "int add(int a, int b);";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for function with missing body");
}

MU_TEST(test_parser_array_type) {
    LOG_TRACE("Running test_parser_array_type");
    const char *input = "int main() { int arr[10]; }"; // Fixed missing closing brace
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *var_decl = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl->type);
    mu_assert_string_eq("arr", var_decl->data.var_decl.name);
    mu_assert_int_eq(TYPE_ARRAY, var_decl->data.var_decl.type->kind);
    mu_assert_int_eq(10, var_decl->data.var_decl.type->array_size);

    free_ast(ast);
}

MU_TEST(test_parser_pointer_type) {
    LOG_TRACE("Running test_parser_pointer_type");
    const char *input = "int main() { int *ptr; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *var_decl = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl->type);
    mu_assert_string_eq("ptr", var_decl->data.var_decl.name);
    mu_assert_int_eq(TYPE_POINTER, var_decl->data.var_decl.type->kind);

    free_ast(ast);
}

MU_TEST(test_parser_postfix_operations) {
    LOG_TRACE("Running test_parser_postfix_operations");
    const char *input = "int main() { int x = 0; x++; x--; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    ASTNode *var_decl = body->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_VAR_DECL, var_decl->type);
    mu_assert_string_eq("x", var_decl->data.var_decl.name);

    ASTNode *postfix_inc = body->data.stmt_list.stmts[1];
    mu_assert_int_eq(NODE_UNARY_OP, postfix_inc->type);
    mu_assert_int_eq(TOK_PLUS_PLUS, postfix_inc->data.unary_op.op);
    mu_assert_string_eq("x", postfix_inc->data.unary_op.operand->data.var_ref.name);

    ASTNode *postfix_dec = body->data.stmt_list.stmts[2];
    mu_assert_int_eq(NODE_UNARY_OP, postfix_dec->type);
    mu_assert_int_eq(TOK_MINUS_MINUS, postfix_dec->data.unary_op.op);
    mu_assert_string_eq("x", postfix_dec->data.unary_op.operand->data.var_ref.name);

    free_ast(ast);
}

MU_TEST(test_parser_compound_assignment) {
    LOG_TRACE("Running test_parser_compound_assignment");
    const char *input = "int main() { x += 1; x -= 2; x *= 3; x /= 4; x %= 5; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);

    mu_assert(ast != NULL, "AST should not be NULL");
    mu_assert_int_eq(NODE_PROGRAM, ast->type);

    ASTNode *function = ast->data.stmt_list.stmts[0];
    mu_assert_int_eq(NODE_FUNCTION_DECL, function->type);

    ASTNode *body = function->data.function_decl.body;
    for (int i = 0; i < 5; i++) {
        ASTNode *compound_assign = body->data.stmt_list.stmts[i];
        mu_assert_int_eq(NODE_BINARY_OP, compound_assign->type);
    }

    free_ast(ast);
}

MU_TEST(test_parser_missing_semicolon) {
    LOG_TRACE("Running test_parser_missing_semicolon");
    const char *input = "int main() { int x = 42 }"; // Missing semicolon
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast == NULL, "AST should be NULL for invalid input");
}

MU_TEST(test_parser_error_recovery) {
    LOG_TRACE("Running test_parser_error_recovery");

    const char *source = "int main() { int x = 42; ; int y = 24; }";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program != NULL, "Expected parsing to succeed with recovery after semicolon");
    free_ast(program);

    source = "int main() { int x = 42 if (x) { return x; } }";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail with recovery at keyword");
}

MU_TEST(test_parser_array_declaration) {
    LOG_TRACE("Running test_parser_array_declaration");

    const char *source = "int main() { int arr[]; }";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for array with unspecified size");

    source = "int main() { int arr[\"hello\"]; }";
    lexer_init(&lexer, source);
    program = parse(&lexer);
    mu_assert(program == NULL, "Expected parsing to fail for array with invalid size");
}

MU_TEST(test_parser_logical_operators) {
    LOG_TRACE("Running test_parser_logical_operators");

    const char *source = "int main() { int x = 1 && 0 || 1; }";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program != NULL, "Expected parsing to succeed for logical operators");
    free_ast(program);
}

MU_TEST(test_parser_equality_operators) {
    LOG_TRACE("Running test_parser_equality_operators");

    const char *source = "int main() { int x = (1 == 1) != 0; }";
    Lexer lexer;
    lexer_init(&lexer, source);
    ASTNode *program = parse(&lexer);
    mu_assert(program != NULL, "Expected parsing to succeed for equality operators");
    free_ast(program);
}

MU_TEST(test_parser_memory_allocation_failure) {
    LOG_TRACE("Running test_parser_memory_allocation_failure");

    // Simulate memory allocation failure in create_var_decl_node
    const char *source = "int x = 42;";
    Lexer lexer;
    lexer_init(&lexer, source);
    // Mock malloc to return NULL (requires additional setup in the test framework)
    // For now, this is a placeholder to indicate the test case.
    // mu_assert(mock_malloc_failure(), "Expected malloc to fail");
}

MU_TEST_SUITE(parser_suite) {
    MU_RUN_TEST(test_parser_simple_program);
    MU_RUN_TEST(test_parser_nested_program);
    MU_RUN_TEST(test_parser_unary_operations);
    MU_RUN_TEST(test_parser_variable_declaration);
    MU_RUN_TEST(test_parser_if_statement);
    MU_RUN_TEST(test_parser_while_statement);
    MU_RUN_TEST(test_parser_for_statement);
    MU_RUN_TEST(test_parser_function_call);
    MU_RUN_TEST(test_parser_assignment);
    MU_RUN_TEST(test_parser_empty_block);
    MU_RUN_TEST(test_parser_invalid_token);
    MU_RUN_TEST(test_parser_array_type);
    MU_RUN_TEST(test_parser_pointer_type);
    MU_RUN_TEST(test_parser_postfix_operations);
    MU_RUN_TEST(test_parser_compound_assignment);
    MU_RUN_TEST(test_parser_missing_semicolon);
    MU_RUN_TEST(test_error_handling);
    MU_RUN_TEST(test_type_parsing);
    MU_RUN_TEST(test_unary_operations);
    MU_RUN_TEST(test_binary_operations);
    MU_RUN_TEST(test_control_structures);
    MU_RUN_TEST(test_function_parsing);
    MU_RUN_TEST(test_error_handling);
    MU_RUN_TEST(test_function_parsing);
    MU_RUN_TEST(test_error_handling_edge_cases);
    // MU_RUN_TEST(test_parser_error_recovery);
    // MU_RUN_TEST(test_parser_array_declaration);
    MU_RUN_TEST(test_parser_logical_operators);
    MU_RUN_TEST(test_parser_equality_operators);
    // MU_RUN_TEST(test_parser_memory_allocation_failure);
}

int main() {
    MU_RUN_SUITE(parser_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}