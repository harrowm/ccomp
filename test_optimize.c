#include "tac.h"
#include "cfg.h"
#include "lexer.h"
#include "parser.h"
#include "minunit.h"
#include "optimize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MU_TEST(test_optimize_noop) {
    const char *input = "int main() { return 1 + 2; }";
    Lexer lexer;
    lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg);
    compute_dominance_frontiers(cfg);
    insert_phi_functions(cfg);
    create_tac(cfg);
    optimize_tac(cfg);

    // Expected output after constant folding: return 3
    const char *expected_output =
        "L0:\n"
        "_ = call main\n"
        "goto L1\n"
        "goto L2\n"
        "main:\n"
        "__enter = main\n"
        "L2:\n"
        "return 3\n"
        "goto L1\n"
        "L1:\n"
        "halt\n";

    print_tac(cfg, stdout);

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream);
    fclose(output_stream);

    // Compare expected and actual output line by line
    char *expected_ptr = (char *)expected_output;
    char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};

        // Read a line from expected output
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line) + 1;

        // Read a line from actual output
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line) + 1;

        // Compare the lines
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST_SUITE(optimize_suite) {
    MU_RUN_TEST(test_optimize_noop);
}

int main(int argc, char **argv) {
    MU_RUN_SUITE(optimize_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
