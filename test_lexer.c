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

MU_TEST(test_lexer_all_tokens) {
    const char *input = "int x = 42; float y = 3.14; if (x < y) { x = x + 1; } else { y = y - 1; }";
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

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_KW_FLOAT, token.type);
    mu_assert(mu_assert_token_eq("float", &token), "Expected token 'float'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("y", &token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_EQ, token.type);
    mu_assert(mu_assert_token_eq("=", &token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_FLOAT, token.type);
    mu_assert(mu_assert_token_eq("3.14", &token), "Expected token '3.14'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_SEMICOLON, token.type);
    mu_assert(mu_assert_token_eq(";", &token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_KW_IF, token.type);
    mu_assert(mu_assert_token_eq("if", &token), "Expected token 'if'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_LPAREN, token.type);
    mu_assert(mu_assert_token_eq("(", &token), "Expected token '('");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("x", &token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_LT, token.type);
    mu_assert(mu_assert_token_eq("<", &token), "Expected token '<'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("y", &token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_RPAREN, token.type);
    mu_assert(mu_assert_token_eq(")", &token), "Expected token ')'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_LBRACE, token.type);
    mu_assert(mu_assert_token_eq("{", &token), "Expected token '{'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("x", &token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_EQ, token.type);
    mu_assert(mu_assert_token_eq("=", &token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("x", &token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_PLUS, token.type);
    mu_assert(mu_assert_token_eq("+", &token), "Expected token '+'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_INTEGER, token.type);
    mu_assert(mu_assert_token_eq("1", &token), "Expected token '1'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_SEMICOLON, token.type);
    mu_assert(mu_assert_token_eq(";", &token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_RBRACE, token.type);
    mu_assert(mu_assert_token_eq("}", &token), "Expected token '}'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_KW_ELSE, token.type);
    mu_assert(mu_assert_token_eq("else", &token), "Expected token 'else'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_LBRACE, token.type);
    mu_assert(mu_assert_token_eq("{", &token), "Expected token '{'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("y", &token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_EQ, token.type);
    mu_assert(mu_assert_token_eq("=", &token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_IDENTIFIER, token.type);
    mu_assert(mu_assert_token_eq("y", &token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_MINUS, token.type);
    mu_assert(mu_assert_token_eq("-", &token), "Expected token '-'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_INTEGER, token.type);
    mu_assert(mu_assert_token_eq("1", &token), "Expected token '1'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_SEMICOLON, token.type);
    mu_assert(mu_assert_token_eq(";", &token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert_int_eq(TOK_RBRACE, token.type);
    mu_assert(mu_assert_token_eq("}", &token), "Expected token '}'");
}

MU_TEST_SUITE(lexer_suite) {
    MU_RUN_TEST(test_lexer_single_token);
    MU_RUN_TEST(test_lexer_multiple_tokens);
    MU_RUN_TEST(test_lexer_all_tokens);
}

int main() {
    MU_RUN_SUITE(lexer_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}