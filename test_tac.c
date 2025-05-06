#include "tac.h"
#include "cfg.h"
#include "lexer.h"
#include "parser.h"
#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MU_TEST(test_tac_with_phi_function) {
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

    print_ast(ast, 0);

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    print_cfg(cfg, stdout);

    compute_dominator_tree(cfg);
    compute_dominance_frontiers(cfg);
    insert_phi_functions(cfg);

    print_cfg(cfg, stdout);

    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    const char *expected_output = "L0:\n"
                                  "goto L2\n"
                                  "L2:\n"
                                  "x = 42\n"
                                  "t201 = x > 0\n"
                                  "L3:\n"
                                  "if t201 goto L3_then\n"
                                  "goto L3_else\n"
                                  "L3_then:\n"
                                  "x = x - 1\n"
                                  "goto L3_end\n"
                                  "L3_else:\n"
                                  "x = x + 1\n"
                                  "goto L3_end\n"
                                  "L3_end:\n"
                                  "goto L4\n"
                                  "L4:\n"
                                  "return x\n"
                                  "goto L5\n"
                                  "L5:\n";

    // Redirect TAC output to a string buffer
    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(tac, output_stream);
    fclose(output_stream);

    printf("%s\n", actual_output);

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

    // Ensure both outputs have the same number of lines
    mu_assert(*expected_ptr == '\0' && *actual_ptr == '\0', "TAC output line count mismatch");

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST_SUITE(tac_suite) {
    MU_RUN_TEST(test_tac_with_phi_function);
}

int main() {
    MU_RUN_SUITE(tac_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}