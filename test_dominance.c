#include "cfg.h"
#include "lexer.h"
#include "parser.h"
#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>

MU_TEST(test_dominator_tree_basic) {
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

MU_TEST(test_dominator_tree_with_df) {
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
    compute_dominance_frontiers(cfg);

    // Verify dominance frontiers for each block
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        mu_assert(block != NULL, "Block should not be NULL");

        printf("Block %zu dominance frontier: ", i);
        for (size_t j = 0; j < block->dom_frontier->count; j++) {
            BasicBlock *df_block = block->dom_frontier->blocks[j];
            mu_assert(df_block != NULL, "Dominance frontier block should not be NULL");
            printf("%zu ", df_block->id);
        }
        printf("\n");
    }

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_generate_df_dot_file) {
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
    compute_dominance_frontiers(cfg);

    const char *dot_filename = "test_df_output.dot";
    generate_dominance_frontiers_dot(cfg, dot_filename);

    FILE *dot_file = fopen(dot_filename, "r");
    mu_assert(dot_file != NULL, "DOT file should be generated");

    FILE *expected_file = fopen("test_df_expected.dot", "r");
    mu_assert(expected_file != NULL, "Expected DOT file should exist");

    char dot_line[256], expected_line[256];
    while (fgets(dot_line, sizeof(dot_line), dot_file) && fgets(expected_line, sizeof(expected_line), expected_file)) {
        mu_assert_string_eq(expected_line, dot_line);
    }

    fclose(dot_file);
    fclose(expected_file);

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_phi_function_insertion) {
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
    compute_dominance_frontiers(cfg);
    insert_phi_functions(cfg);

    // Verify CFG structure and phi function placement
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        mu_assert(block != NULL, "Block should not be NULL");

        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            if (stmt->type == NODE_VAR_DECL && stmt->data.var_decl.init_value == NULL) {
                mu_assert(j == 0, "Phi functions should be the first statements in the block");
                mu_assert(stmt->data.var_decl.name != NULL, "Phi function variable name should not be NULL");
            }
        }
    }

    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST_SUITE(dominance_suite) {
    MU_RUN_TEST(test_dominator_tree_basic);
    MU_RUN_TEST(test_dominator_tree_with_df);
    MU_RUN_TEST(test_generate_df_dot_file);
    MU_RUN_TEST(test_phi_function_insertion);
}

int main() {
    MU_RUN_SUITE(dominance_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}