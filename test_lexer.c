#include "lexer.h"
#include "minunit.h"
#include <string.h>

static int mu_assert_token_eq(const char* expected, const Token* token) {
    if (strncmp(expected, token->text, token->length) != 0 || strlen(expected) != token->length) {
        fprintf(stderr, "Expected token text: '%s', but got: '%.*s'\n", expected, (int)token->length, token->text);
        return 0;
    }
    return 1;
}

MU_TEST(test_lexer_single_token) {
    const char *input = "int";
    Lexer lexer;
    lexer_init(&lexer, input);
    Token token = next_token(&lexer);
    mu_assert_int_eq(TOK_KW_INT, token.type);
    mu_assert(mu_assert_token_eq("int", &token), "Expected token 'int'");
}

MU_TEST(test_lexer_multiple_tokens) {
    const char *input = "int x = 42;";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert_int_eq(TOK_KW_INT, token.type);
    mu_assert(mu_assert_token_eq("int", &token), "Expected token 'int'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("x", &token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_EQ, token.type);
    mu_assert(mu_assert_token_eq("=", &token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_INTEGER, token.type);
    mu_assert(mu_assert_token_eq("42", &token), "Expected token '42'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_SEMICOLON, token.type);
    mu_assert(mu_assert_token_eq(";", &token), "Expected token ';'");
}

MU_TEST_SUITE(lexer_suite) {
    MU_RUN_TEST(test_lexer_single_token);
    MU_RUN_TEST(test_lexer_multiple_tokens);
}

int main() {
    MU_RUN_SUITE(lexer_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}