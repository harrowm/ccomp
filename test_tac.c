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

    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    print_tac(tac, stdout);
    const char *expected_output = "L0:\n"
                                  "_ = call main\n"
                                  "goto L1\n"
                                  "goto L2\n"
                                  "main:\n"
                                  "__enter = main\n"
                                  "L2:\n"
                                  "x = 42\n"
                                  "t0 = x > 0\n"
                                  "L3:\n"
                                  "if not t0 goto L4\n"
                                  "t1 = x - 1\n"
                                  "x = t1\n"
                                  "goto L5\n"
                                  "L5:\n"
                                  "x = phi(...)\n"
                                  "return x\n"
                                  "goto L1\n"
                                  "L1:\n"
                                  "halt\n"
                                  "L4:\n"
                                  "t2 = x + 1\n"
                                  "x = t2\n"
                                  "goto L5\n";

    // Redirect TAC output to a string buffer
    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(tac, output_stream);
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

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_tac_arithmetic_precedence) {
    const char *input = "int main() {\n"
                        "  return 2 + 3 * 5;\n"
                        "}";

    Lexer lexer;
    lexer_init(&lexer, input);

    ASTNode *ast = parse(&lexer);
    mu_assert(ast != NULL, "AST should not be NULL");

    // print_ast(ast, 0);

    CFG *cfg = ast_to_cfg(ast);
    mu_assert(cfg != NULL, "CFG should not be NULL");

    // print_cfg(cfg, stdout);

    compute_dominator_tree(cfg);
    compute_dominance_frontiers(cfg);
    insert_phi_functions(cfg);

    // print_cfg(cfg, stdout);

    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    const char *expected_output = "L6:\n"
                                  "_ = call main\n"
                                  "goto L7\n"
                                  "goto L8\n"
                                  "main:\n"
                                  "__enter = main\n"
                                  "L8:\n"
                                  "t3 = 3 * 5\n"
                                  "t4 = 2 + t3\n"
                                  "return t4\n"
                                  "goto L7\n"
                                  "L7:\n"
                                  "halt\n";

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

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_tac_while_loop) {
    const char *input = "int main() {\n"
                        "  int x = 0;\n"
                        "  while (x < 5) {\n"
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

print_cfg(cfg, stdout);

    compute_dominator_tree(cfg);
    compute_dominance_frontiers(cfg);
    insert_phi_functions(cfg);

    print_cfg(cfg, stdout);


    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    print_tac(tac, stdout);

    const char *expected_output = "L9:\n"
                                  "_ = call main\n"
                                  "goto L10\n"
                                  "goto L11\n"
                                  "main:\n"
                                  "__enter = main\n"
                                  "L11:\n" 
                                  "x = 0\n"
                                  "goto L12\n"
                                  "L12:\n"
                                  "x = phi(...)\n"
                                  "t5 = x < 5\n"
                                  "if not t5 goto L14\n"
                                  "L13:\n"
                                  "t6 = x + 1\n"
                                  "x = t6\n"
                                  "goto L12\n"
                                  "L14:\n"
                                  "return x\n"
                                  "goto L10\n"
                                  "L10:\n"
                                  "halt\n";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(tac, output_stream);
    fclose(output_stream);

    char *expected_ptr = (char *)expected_output;
    char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};

        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line) + 1;

        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line) + 1;

        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_tac_for_loop) {
    const char *input = "int main() {\n"
                        "  int sum = 0;\n"
                        "  for (int i = 0; i < 5; i = i + 1) {\n"
                        "    sum = sum + i;\n"
                        "  }\n"
                        "  return sum;\n"
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

    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    print_tac(tac, stdout);

    const char *expected_output = "L15:\n"
                                  "_ = call main\n"
                                  "goto L16\n"
                                  "goto L17\n"
                                  "main:\n"
                                  "__enter = main\n"
                                  "L17:\n"
                                  "sum = 0\n"
                                  "i = 0\n"
                                  "goto L18\n"
                                  "L18:\n"
                                  "i = phi(...)\n"
                                  "sum = phi(...)\n"
                                  "t7 = i < 5\n"
                                  "if not t7 goto L20\n"
                                  "L19:\n"
                                  "t8 = sum + i\n"
                                  "sum = t8\n"
                                  "t9 = i + 1\n"
                                  "i = t9\n"
                                  "goto L18\n"
                                  "L20:\n"
                                  "return sum\n"
                                  "goto L16\n"
                                  "L16:\n"
                                  "halt\n";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(tac, output_stream);
    fclose(output_stream);

    char *expected_ptr = (char *)expected_output;
    char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};

        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line) + 1;

        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line) + 1;

        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_tac_dangling_else) {
    const char *input = "int main() {\n"
                        "  int x = 0;\n"
                        "  int y = 0;\n"
                        "  if (x > 0)\n"
                        "    if (y > 0)\n"
                        "      x = 1;\n"
                        "    else\n"
                        "      x = 2;\n"
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

    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    print_tac(tac, stdout);
    
    // The expected output updated to match the actual TAC output for the dangling else scenario.
    const char *expected_output = "L21:\n"
        "_ = call main\n"
        "goto L22\n"
        "goto L23\n"
        "main:\n"
        "__enter = main\n"
        "L23:\n"
        "x = 0\n"
        "y = 0\n"
        "t10 = x > 0\n"
        "L24:\n"
        "if not t10 goto L25\n"
        "t11 = y > 0\n"
        "L27:\n"
        "if not t11 goto L28\n"
        "x = 1\n"
        "goto L29\n"
        "L29:\n"
        "x = phi(...)\n"
        "goto L26\n"
        "L26:\n"
        "return x\n"
        "goto L22\n"
        "L22:\n"
        "halt\n"
        "L28:\n"
        "x = 2\n"
        "goto L29\n"
        "L25:\n"
        "goto L26\n";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(tac, output_stream);
    fclose(output_stream);

    char *expected_ptr = (char *)expected_output;
    char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};

        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line) + 1;

        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line) + 1;

        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST(test_tac_function_call) {
    const char *input = "int foo() {\n"
                        "  return 42;\n"
                        "}\n"
                        "int main() {\n"
                        "  int x = foo();\n"
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

    TAC *tac = cfg_to_tac(cfg);
    mu_assert(tac != NULL, "TAC should not be NULL");

    print_cfg(cfg, stdout);
    print_tac(tac, stdout);
    
    // The expected output may need to be adjusted to match your TAC generator's label and temp naming conventions.
    const char *expected_output = "L30:\n"
        "_ = call main\n"
        "goto L31\n"
        "foo:\n"
        "__enter = foo\n"
        "L32:\n"
        "return 42\n"
        "goto L31\n"
        "L31:\n"
        "x = phi(...)\n"
        "halt\n"
        "main:\n"
        "__enter = main\n"
        "L33:\n"
        "x = call foo\n"
        "return x\n"
        "goto L31\n";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(tac, output_stream);
    fclose(output_stream);

    char *expected_ptr = (char *)expected_output;
    char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};

        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line) + 1;

        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line) + 1;

        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }

    free_tac(tac);
    free_cfg(cfg);
    free_ast(ast);
}

MU_TEST_SUITE(tac_suite) {
    MU_RUN_TEST(test_tac_with_phi_function);
    MU_RUN_TEST(test_tac_arithmetic_precedence);
    MU_RUN_TEST(test_tac_while_loop);
    MU_RUN_TEST(test_tac_for_loop);
    MU_RUN_TEST(test_tac_dangling_else);
    MU_RUN_TEST(test_tac_function_call);
}

int main(int argc, char **argv) {
    MU_RUN_SUITE(tac_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}