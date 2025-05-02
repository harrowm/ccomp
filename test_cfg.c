#include "cfg.h"
#include "lexer.h"
#include "parser.h"
#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>

MU_TEST(test_cfg_creation) {
    const char *input = "int main() { return 0; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    free_cfg(cfg);
    free_ast(ast);
}


MU_TEST(test_cfg_detailed_structure) {
    const char *input = "int main() { if (1) return 42; else return 0; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // Verify entry and exit blocks
    mu_assert(cfg->entry != NULL, "CFG entry block should not be NULL");
    mu_assert(cfg->exit != NULL, "CFG exit block should not be NULL");

    // Walk through all blocks
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        mu_assert(block != NULL, "Block should not be NULL");
        mu_assert(block->id == i, "Block ID should match its index");

        // Check block type
        mu_assert(block->type >= BLOCK_NORMAL && block->type <= BLOCK_LOOP_BODY, "Block type should be valid");

        // Validate statements
        for (size_t j = 0; j < block->stmt_count; j++) {
            mu_assert(block->stmts[j] != NULL, "Statement in block should not be NULL");
        }

        // Validate predecessors and successors
        for (size_t j = 0; j < block->pred_count; j++) {
            mu_assert(block->preds[j] != NULL, "Predecessor should not be NULL");
        }
        for (size_t j = 0; j < block->succ_count; j++) {
            mu_assert(block->succs[j] != NULL, "Successor should not be NULL");
        }

        // Check dominance attributes
        if (block->dominator != NULL) {
            mu_assert(block->dominator->id < cfg->block_count, "Dominator ID should be valid");
        }
        for (size_t j = 0; j < block->dominated_count; j++) {
            mu_assert(block->dominated[j] != NULL, "Dominated block should not be NULL");
        }
    }

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_all_node_types) {
    const char *input = "int main() {\n"
                        "  int x = 42;\n"
                        "  if (x > 0) {\n"
                        "    x = x - 1;\n"
                        "  } else {\n"
                        "    x = x + 1;\n"
                        "  }\n"
                        "  while (x < 50) {\n"
                        "    x = x + 2;\n"
                        "  }\n"
                        "  for (int i = 0; i < 10; i++) {\n"
                        "    x = x * i;\n"
                        "  }\n"
                        "  return x;\n"
                        "}";

    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // Verify CFG structure
    mu_assert(cfg->entry != NULL, "CFG entry block should not be NULL");
    mu_assert(cfg->exit != NULL, "CFG exit block should not be NULL");
    mu_assert(cfg->block_count > 0, "CFG should have blocks");

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_dominator_tree) {
    const char *input = "int main() {\n"
                        "  int x = 42;\n"
                        "  if (x > 0) {\n"
                        "    x = x - 1;\n"
                        "  } else {\n"
                        "    x = x + 1;\n"
                        "  }\n"
                        "  return x;\n"
                        "}";

    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    compute_dominator_tree(cfg);

    // Verify dominator tree
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        if (block != cfg->entry) {
            mu_assert(block->dominator != NULL, "Block should have a dominator");
        }
    }

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_dot_file_generation) {
    const char *input = "int main() {\n"
                        "  int x = 42;\n"
                        "  if (x > 0) {\n"
                        "    x = x - 1;\n"
                        "  }\n"
                        "  return x;\n"
                        "}";

    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    const char *dot_filename = "test_cfg.dot";
    generate_dot_file(cfg, dot_filename);

    FILE *dot_file = fopen(dot_filename, "r");
    mu_assert(dot_file != NULL, "DOT file should be generated");
    fclose(dot_file);

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_if_no_else) {
    const char *input = "int main() {\n"
                        "  int x = 42;\n"
                        "  if (x > 0) {\n"
                        "    x = x - 1;\n"
                        "  }\n"
                        "  return x;\n"
                        "}";

    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // Verify CFG structure for if statement without else
    mu_assert(cfg->block_count > 0, "CFG should have blocks");
    bool found_if_then_block = false;
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        if (block->type == BLOCK_IF_THEN) {
            found_if_then_block = true;
            break;
        }
    }
    mu_assert(found_if_then_block, "CFG should have an if-then block");

    free_cfg(cfg);
    free_ast(ast);
}

// Helper function to recursively check for NODE_UNARY_OP in an expression tree
static bool contains_unary_op(ASTNode *node) {
    if (!node) return false;
    if (node->type == NODE_UNARY_OP) return true;

    switch (node->type) {
        case NODE_BINARY_OP:
            return contains_unary_op(node->data.binary_op.left) || contains_unary_op(node->data.binary_op.right);
        case NODE_RETURN:
            return contains_unary_op(node->data.return_stmt.value);
        default:
            return false;
    }
}

MU_TEST(test_cfg_unary_operations) {
    const char *input = "int main() { return -1 + !2; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // Verify CFG structure for unary operations
    mu_assert(cfg->block_count > 0, "CFG should have blocks");
    bool found_unary_op = false;
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        for (size_t j = 0; j < block->stmt_count; j++) {
            if (contains_unary_op(block->stmts[j])) {
                found_unary_op = true;
                break;
            }
        }
        if (found_unary_op) break;
    }
    mu_assert(found_unary_op, "CFG should have a unary operation");

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_function_call) {
    const char *input = "int main() { foo(42); return 0; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // Verify CFG structure for function calls
    mu_assert(cfg->block_count > 0, "CFG should have blocks");
    bool found_function_call = false;
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        for (size_t j = 0; j < block->stmt_count; j++) {
            if (block->stmts[j]->type == NODE_FUNCTION_CALL) {
                found_function_call = true;
                break;
            }
        }
        if (found_function_call) break;
    }
    mu_assert(found_function_call, "CFG should have a function call");

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_empty_program) {
    const char *input = "";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast == NULL, "AST should be NULL for empty input");
}

MU_TEST(test_cfg_printing) {
    const char *input = "int main() { int x = 42; if (x > 0) x = x - 1; return x; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // Test print_cfg
    FILE *cfg_stream = fopen("test_cfg_output.txt", "w");
    mu_assert(cfg_stream != NULL, "Failed to open file for writing CFG output");
    print_cfg(cfg, cfg_stream);
    fclose(cfg_stream);

    // Verify the output matches expected format
    FILE *expected_stream = fopen("test_cfg_expected_output.txt", "r");
    FILE *actual_stream = fopen("test_cfg_output.txt", "r");
    mu_assert(expected_stream != NULL && actual_stream != NULL, "Failed to open expected or actual output file");

    char expected_line[256], actual_line[256];
    while (fgets(expected_line, sizeof(expected_line), expected_stream) && fgets(actual_line, sizeof(actual_line), actual_stream)) {
        mu_assert(strcmp(expected_line, actual_line) == 0, "CFG output does not match expected format");
    }

    fclose(expected_stream);
    fclose(actual_stream);

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_printing_func_call) {
    const char *input = "int foo(int a) { return a + 1; } int main() { int x = foo(42); return x; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL for function call test");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL for function call test");

    // Test print_cfg with function call
    const char *output_filename = "test_cfg_func_call_output.txt"; // Use a different temp file
    FILE *cfg_stream = fopen(output_filename, "w");
    mu_assert(cfg_stream != NULL, "Failed to open file for writing CFG func call output");
    print_cfg(cfg, cfg_stream);
    fclose(cfg_stream);

    // Verify the output matches expected format
    const char *expected_filename = "test_cfg_func_call_expected_output.txt";
    FILE *expected_stream = fopen(expected_filename, "r");
    FILE *actual_stream = fopen(output_filename, "r");
    mu_assert(expected_stream != NULL, "Failed to open expected func call output file");
    mu_assert(actual_stream != NULL, "Failed to open actual func call output file");


    char expected_line[256], actual_line[256];
    char error_message[300]; // Buffer for formatted error message
    int line_num = 1;
    while (fgets(expected_line, sizeof(expected_line), expected_stream) && fgets(actual_line, sizeof(actual_line), actual_stream)) {
        // Format the error message before calling mu_assert
        snprintf(error_message, sizeof(error_message), "CFG func call output does not match expected format on line %d", line_num);
        mu_assert(strcmp(expected_line, actual_line) == 0, error_message);
        line_num++;
    }

    // Check if one file has more lines than the other by attempting one more read
    char extra_expected[2], extra_actual[2];
    bool expected_has_more = fgets(extra_expected, sizeof(extra_expected), expected_stream) != NULL;
    bool actual_has_more = fgets(extra_actual, sizeof(extra_actual), actual_stream) != NULL;

    // Assert that neither file has further content
    mu_assert(!expected_has_more && !actual_has_more, "Files appear to have different lengths or trailing characters");

    // Also check for read errors, just in case
    mu_assert(!ferror(expected_stream), "Error reading expected file after loop");
    mu_assert(!ferror(actual_stream), "Error reading actual file after loop");

    fclose(expected_stream);
    fclose(actual_stream);

    // Optional: remove the temporary output file
    // remove(output_filename);

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_cfg_multiple_functions) {
    const char *input = "int func1() { return 1; } int func2(int x) { return x * 2; } int main() { int a = func1(); int b = func2(a); return b; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL for multiple functions test");
    mu_assert(ast->type == NODE_PROGRAM, "AST root should be NODE_PROGRAM");
    mu_assert(ast->data.stmt_list.count == 3, "AST should contain 3 function declarations (func1, func2, main)");

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL for multiple functions test");

    // Verify CFG structure
    mu_assert(cfg->entry != NULL, "CFG entry block should exist");
    mu_assert(cfg->exit != NULL, "CFG exit block should exist");
    mu_assert(cfg->block_count > 2, "CFG should have more than just entry and exit blocks for multiple functions");

    // Check for function calls within variable initializers
    bool found_func1_call = false, found_func2_call = false;
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        mu_assert(block != NULL, "Block should not be NULL");

        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            if (stmt->type == NODE_VAR_DECL && stmt->data.var_decl.init_value) {
                ASTNode *init_value = stmt->data.var_decl.init_value;
                if (init_value->type == NODE_FUNCTION_CALL) {
                    const char *func_name = init_value->data.function_call.name;
                    if (strcmp(func_name, "func1") == 0) {
                        found_func1_call = true;
                    } else if (strcmp(func_name, "func2") == 0) {
                        found_func2_call = true;
                    }
                }
            }
        }
    }

    mu_assert(found_func1_call, "CFG should contain a call to func1 in a variable initializer");
    mu_assert(found_func2_call, "CFG should contain a call to func2 in a variable initializer");

    // Clean up
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST_SUITE(cfg_suite) {
    MU_RUN_TEST(test_cfg_creation);
    MU_RUN_TEST(test_cfg_detailed_structure);
    MU_RUN_TEST(test_cfg_all_node_types);
    MU_RUN_TEST(test_cfg_dominator_tree);
    MU_RUN_TEST(test_cfg_dot_file_generation);
    MU_RUN_TEST(test_cfg_if_no_else);
    MU_RUN_TEST(test_cfg_function_call);
    MU_RUN_TEST(test_cfg_empty_program);
    MU_RUN_TEST(test_cfg_unary_operations);
    MU_RUN_TEST(test_cfg_printing);
    MU_RUN_TEST(test_cfg_printing_func_call);
    MU_RUN_TEST(test_cfg_multiple_functions);
}

int main() {
    MU_RUN_SUITE(cfg_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}