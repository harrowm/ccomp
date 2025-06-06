// SSA tests must be after all includes, not before!
#include "tac.h"
#include "cfg.h"
#include "lexer.h"
#include "parser.h"
#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
// --- SSA versions of the TAC tests ---
// Only one set of SSA tests, not duplicated!
// Forward declaration for SSA conversion
#include "tac.h"
void convert_to_ssa(CFG *cfg);


MU_TEST(test_tac_with_phi_function_ssa) {
    const char *input = "int main() {\n"
                        "  int x = 42;\n"
                        "  if (x > 0) {\n"
                        "    x = x - 1;\n"
                        "  } else {\n"
                        "    x = x + 1;\n"
                        "  }\n"
                        "  return x;\n"
                        "}";
    Lexer lexer; lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer); mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast); mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg); compute_dominance_frontiers(cfg); insert_phi_functions(cfg);
    create_tac(cfg);
    convert_to_ssa(cfg);

    // Check TAC output against expected canonical SSA form
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L34:\n"
        "call main\n"
        "goto L35\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L35:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L36:\n"
        "x_0 = 42\n"
        "t12_0 = x_0 > 0\n"
        "if not t12_0 goto L38\n"
        "goto L37\n"
        "\n"
        "# BasicBlock 3 (If-Then Block)\n"
        "L37:\n"
        "t13_0 = x_0 - 1\n"
        "x_1 = t13_0\n"
        "goto L39\n"
        "\n"
        "# BasicBlock 4 (If-Else Block)\n"
        "L38:\n"
        "t14_0 = x_0 + 1\n"
        "x_2 = t14_0\n"
        "goto L39\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L39:\n"
        "x_3 = phi(x_1,x_2)\n"
        "return x_3\n"
        "goto L35\n"
        "";

    char actual_output[2048] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream); fclose(output_stream);

    // Print for debug
    printf("%s\n", actual_output);

    // Compare expected and actual output line by line
    const char *expected_ptr = expected_output;
    const char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line);
        if (*expected_ptr == '\n') expected_ptr++;
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line);
        if (*actual_ptr == '\n') actual_ptr++;
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }
    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
    free_cfg(cfg); free_ast(ast);
}

MU_TEST(test_tac_arithmetic_precedence_ssa) {
    const char *input = "int main() {\n  return 2 + 3 * 5;\n}";
    Lexer lexer; lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer); mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast); mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg); compute_dominance_frontiers(cfg); insert_phi_functions(cfg);
    create_tac(cfg);
    convert_to_ssa(cfg);

    // Expected SSA TAC output for arithmetic precedence
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L40:\n"
        "call main\n"
        "goto L41\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L41:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L42:\n"
        "t15_0 = 3 * 5\n"
        "t16_0 = 2 + t15_0\n"
        "return t16_0\n"
        "goto L41\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream); fclose(output_stream);
    printf("%s\n", actual_output);

    // Compare expected and actual output line by line
    const char *expected_ptr = expected_output;
    const char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line);
        if (*expected_ptr == '\n') expected_ptr++;
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line);
        if (*actual_ptr == '\n') actual_ptr++;
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
    free_cfg(cfg); free_ast(ast);
}

MU_TEST(test_tac_while_loop_ssa) {
    const char *input = "int main() {\n  int x = 0;\n  while (x < 5) {\n    x = x + 1;\n  }\n  return x;\n}";
    Lexer lexer; lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer); mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast); mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg); compute_dominance_frontiers(cfg); insert_phi_functions(cfg);
    create_tac(cfg);
    convert_to_ssa(cfg);

    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L43:\n"
        "call main\n"
        "goto L44\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L44:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L45:\n"
        "x_0 = 0\n"
        "goto L46\n"
        "\n"
        "# BasicBlock 3 (Loop Header Block)\n"
        "L46:\n"
        "x_1 = phi(x_0,x_2)\n"
        "t17_0 = x_1 < 5\n"
        "if not t17_0 goto L48\n"
        "goto L47\n"
        "\n"
        "# BasicBlock 4 (Loop Body Block)\n"
        "L47:\n"
        "t18_0 = x_1 + 1\n"
        "x_2 = t18_0\n"
        "goto L46\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L48:\n"
        "return x_1\n"
        "goto L44\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream); fclose(output_stream);
    printf("%s\n", actual_output);

    // Compare expected and actual output line by line
    const char *expected_ptr = expected_output;
    const char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line);
        if (*expected_ptr == '\n') expected_ptr++;
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line);
        if (*actual_ptr == '\n') actual_ptr++;
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }
    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
    free_cfg(cfg); free_ast(ast);
}

MU_TEST(test_tac_for_loop_ssa) {
    const char *input = "int main() {\n  int sum = 0;\n  for (int i = 0; i < 5; i = i + 1) {\n    sum = sum + i;\n  }\n  return sum;\n}";
    Lexer lexer; lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer); mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast); mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg); compute_dominance_frontiers(cfg); insert_phi_functions(cfg);
    create_tac(cfg);
    convert_to_ssa(cfg);
    // Expected SSA TAC output for for-loop
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L49:\n"
        "call main\n"
        "goto L50\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L50:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L51:\n"
        "sum_0 = 0\n"
        "i_0 = 0\n"
        "goto L52\n"
        "\n"
        "# BasicBlock 3 (Loop Header Block)\n"
        "L52:\n"
        "sum_1 = phi(sum_0,sum_2)\n"
        "i_1 = phi(i_0,i_2)\n"
        "t19_0 = i_1 < 5\n"
        "if not t19_0 goto L54\n"
        "goto L53\n"
        "\n"
        "# BasicBlock 4 (Loop Body Block)\n"
        "L53:\n"
        "t20_0 = sum_1 + i_1\n"
        "sum_2 = t20_0\n"
        "t21_0 = i_1 + 1\n"
        "i_2 = t21_0\n"
        "goto L52\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L54:\n"
        "return sum_1\n"
        "goto L50\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream); fclose(output_stream);
    printf("%s\n", actual_output);

    // Compare expected and actual output line by line
    const char *expected_ptr = expected_output;
    const char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line);
        if (*expected_ptr == '\n') expected_ptr++;
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line);
        if (*actual_ptr == '\n') actual_ptr++;
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }
    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
    free_cfg(cfg); free_ast(ast);
}

MU_TEST(test_tac_dangling_else_ssa) {
    const char *input = "int main() {\n  int x = 0;\n  int y = 0;\n  if (x > 0)\n    if (y > 0)\n      x = 1;\n    else\n      x = 2;\n  return x;\n}";
    Lexer lexer; lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer); mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast); mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg); compute_dominance_frontiers(cfg); insert_phi_functions(cfg);
    create_tac(cfg);
    convert_to_ssa(cfg);
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L55:\n"
        "call main\n"
        "goto L56\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L56:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L57:\n"
        "x_0 = 0\n"
        "y_0 = 0\n"
        "t22_0 = x_0 > 0\n"
        "if not t22_0 goto L59\n"
        "goto L58\n"
        "\n"
        "# BasicBlock 3 (If-Then Block)\n"
        "L58:\n"
        "t23_0 = y_0 > 0\n"
        "if not t23_0 goto L62\n"
        "goto L61\n"
        "\n"
        "# BasicBlock 4 (If-Else Block)\n"
        "L59:\n"
        "goto L60\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L60:\n"
        "return x_0\n"
        "goto L56\n"
        "\n"
        "# BasicBlock 6 (If-Then Block)\n"
        "L61:\n"
        "x_1 = 1\n"
        "goto L63\n"
        "\n"
        "# BasicBlock 7 (If-Else Block)\n"
        "L62:\n"
        "x_2 = 2\n"
        "goto L63\n"
        "\n"
        "# BasicBlock 8 (Normal Block)\n"
        "L63:\n"
        "x_3 = phi(x_1,x_2)\n"
        "goto L60\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream); fclose(output_stream);
    printf("%s\n", actual_output);

    // Compare expected and actual output line by line
    const char *expected_ptr = expected_output;
    const char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line);
        if (*expected_ptr == '\n') expected_ptr++;
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line);
        if (*actual_ptr == '\n') actual_ptr++;
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }
    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
    free_cfg(cfg); free_ast(ast);
}

MU_TEST(test_tac_function_call_ssa) {
    const char *input = "int foo() {\n  return 42;\n}\nint main() {\n  int x = foo();\n  return x;\n}";
    Lexer lexer; lexer_init(&lexer, input);
    ASTNode *ast = parse(&lexer); mu_assert(ast != NULL, "AST should not be NULL");
    CFG *cfg = ast_to_cfg(ast); mu_assert(cfg != NULL, "CFG should not be NULL");
    compute_dominator_tree(cfg); compute_dominance_frontiers(cfg); insert_phi_functions(cfg);
    create_tac(cfg);
    convert_to_ssa(cfg);
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L64:\n"
        "call main\n"
        "goto L65\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L65:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "foo:\n"
        "__enter = foo\n"
        "L66:\n"
        "return 42\n"
        "goto L65\n"
        "\n"
        "# BasicBlock 3 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L67:\n"
        "x_0 = call foo\n"
        "return x_0\n"
        "goto L65\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream); fclose(output_stream);
    printf("%s\n", actual_output);

    // Compare expected and actual output line by line
    const char *expected_ptr = expected_output;
    const char *actual_ptr = actual_output;
    while (*expected_ptr && *actual_ptr) {
        char expected_line[256] = {0};
        char actual_line[256] = {0};
        sscanf(expected_ptr, "%255[^\n]\n", expected_line);
        expected_ptr += strlen(expected_line);
        if (*expected_ptr == '\n') expected_ptr++;
        sscanf(actual_ptr, "%255[^\n]\n", actual_line);
        actual_ptr += strlen(actual_line);
        if (*actual_ptr == '\n') actual_ptr++;
        if (strcmp(expected_line, actual_line) != 0) {
            fprintf(stderr, "Mismatch: Actual: '%s' | Expected: '%s'\n", actual_line, expected_line);
        }
        mu_assert(strcmp(expected_line, actual_line) == 0, "TAC output mismatch");
    }
    free_cfg(cfg); free_ast(ast);
}
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

    create_tac(cfg);
    print_tac(cfg, stdout);
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L0:\n"
        "call main\n"
        "goto L1\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L1:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L2:\n"
        "x = 42\n"
        "t0 = x > 0\n"
        "if not t0 goto L4\n"
        "goto L3\n"
        "\n"
        "# BasicBlock 3 (If-Then Block)\n"
        "L3:\n"
        "t1 = x - 1\n"
        "x = t1\n"
        "goto L5\n"
        "\n"
        "# BasicBlock 4 (If-Else Block)\n"
        "L4:\n"
        "t2 = x + 1\n"
        "x = t2\n"
        "goto L5\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L5:\n"
        "x = phi(...)\n"
        "return x\n"
        "";

    // Redirect TAC output to a string buffer
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

    create_tac(cfg);

    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L6:\n"
        "call main\n"
        "goto L7\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L7:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L8:\n"
        "t3 = 3 * 5\n"
        "t4 = 2 + t3\n"
        "return t4\n"
        "";

    // Redirect TAC output to a string buffer
    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream);
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

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
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


    create_tac(cfg);

    print_tac(cfg, stdout);

    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L9:\n"
        "call main\n"
        "goto L10\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L10:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L11:\n"
        "x = 0\n"
        "goto L12\n"
        "\n"
        "# BasicBlock 3 (Loop Header Block)\n"
        "L12:\n"
        "x = phi(...)\n"
        "t5 = x < 5\n"
        "if not t5 goto L14\n"
        "goto L13\n"
        "\n"
        "# BasicBlock 4 (Loop Body Block)\n"
        "L13:\n"
        "t6 = x + 1\n"
        "x = t6\n"
        "goto L12\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L14:\n"
        "return x\n"
        "goto L10\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream);
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

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
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

    create_tac(cfg);

    print_tac(cfg, stdout);

    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L15:\n"
        "call main\n"
        "goto L16\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L16:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L17:\n"
        "sum = 0\n"
        "i = 0\n"
        "goto L18\n"
        "\n"
        "# BasicBlock 3 (Loop Header Block)\n"
        "L18:\n"
        "sum = phi(...)\n"
        "i = phi(...)\n"
        "t7 = i < 5\n"
        "if not t7 goto L20\n"
        "goto L19\n"
        "\n"
        "# BasicBlock 4 (Loop Body Block)\n"
        "L19:\n"
        "t8 = sum + i\n"
        "sum = t8\n"
        "t9 = i + 1\n"
        "i = t9\n"
        "goto L18\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L20:\n"
        "return sum\n"
        "goto L16\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream);
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

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
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

    create_tac(cfg);

    print_tac(cfg, stdout);
    
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L21:\n"
        "call main\n"
        "goto L22\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L22:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L23:\n"
        "x = 0\n"
        "y = 0\n"
        "t10 = x > 0\n"
        "if not t10 goto L25\n"
        "goto L24\n"
        "\n"
        "# BasicBlock 3 (If-Then Block)\n"
        "L24:\n"
        "t11 = y > 0\n"
        "if not t11 goto L28\n"
        "goto L27\n"
        "\n"
        "# BasicBlock 4 (If-Else Block)\n"
        "L25:\n"
        "goto L26\n"
        "\n"
        "# BasicBlock 5 (Normal Block)\n"
        "L26:\n"
        "return x\n"
        "goto L22\n"
        "\n"
        "# BasicBlock 6 (If-Then Block)\n"
        "L27:\n"
        "x = 1\n"
        "goto L29\n"
        "\n"
        "# BasicBlock 7 (If-Else Block)\n"
        "L28:\n"
        "x = 2\n"
        "goto L29\n"
        "\n"
        "# BasicBlock 8 (Normal Block)\n"
        "L29:\n"
        "x = phi(...)\n"
        "goto L26\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream);
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

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
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

    create_tac(cfg);

    print_cfg(cfg, stdout);
    print_tac(cfg, stdout);
    
    const char *expected_output =
        "# BasicBlock 0 (Entry Block)\n"
        "L30:\n"
        "call main\n"
        "goto L31\n"
        "\n"
        "# BasicBlock 1 (Exit Block)\n"
        "L31:\n"
        "halt\n"
        "\n"
        "# BasicBlock 2 (Normal Block)\n"
        "foo:\n"
        "__enter = foo\n"
        "L32:\n"
        "return 42\n"
        "goto L31\n"
        "\n"
        "# BasicBlock 3 (Normal Block)\n"
        "main:\n"
        "__enter = main\n"
        "L33:\n"
        "x = call foo\n"
        "return x\n"
        "goto L31\n"
        "";

    char actual_output[1024] = {0};
    FILE *output_stream = fmemopen(actual_output, sizeof(actual_output), "w");
    print_tac(cfg, output_stream);
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

    for (size_t i = 0; i < cfg->block_count; ++i) free_tac(cfg->blocks[i]->tac_head);
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
    MU_RUN_TEST(test_tac_with_phi_function_ssa);
    MU_RUN_TEST(test_tac_arithmetic_precedence_ssa);
    MU_RUN_TEST(test_tac_while_loop_ssa);
    MU_RUN_TEST(test_tac_for_loop_ssa);
    MU_RUN_TEST(test_tac_dangling_else_ssa);
    MU_RUN_TEST(test_tac_function_call_ssa);
}

int main(int argc, char **argv) {
    MU_RUN_SUITE(tac_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}