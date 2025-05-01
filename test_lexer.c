#include "lexer.h"
#include "minunit.h"
#include <string.h>

static int mu_assert_token_eq(TokenType expected_type, const char* expected_text, const Token token) {
    if (expected_type != token.type) {
        fprintf(stderr, "Expected token type: '%s', but got: '%s'\n", token_type_to_string(expected_type), token_type_to_string(token.type));
        return 0;
    }
    if (strncmp(expected_text, token.text, token.length) != 0 || strlen(expected_text) != token.length) {
        fprintf(stderr, "Expected token text: '%s', but got: '%.*s'\n", expected_text, (int)token.length, token.text);
        return 0;
    }
    return 1;
}

MU_TEST(test_lexer_single_token) {
    const char *input = "int";
    Lexer lexer;
    lexer_init(&lexer, input);
    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_KW_INT, "int", token), "Expected token 'int'");
}

MU_TEST(test_lexer_multiple_tokens) {
    const char *input = "int x = 42;";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_KW_INT, "int", token), "Expected token 'int'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "x", token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ, "=", token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "42", token), "Expected token '42'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SEMICOLON, ";", token), "Expected token ';'");
}

MU_TEST(test_lexer_all_tokens) {
    const char *input = "int x = 42; float y = 3.14; if (x < y) { x = x + 1; } else { y = y - 1; }";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_KW_INT, "int", token), "Expected token 'int'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "x", token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ, "=", token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "42", token), "Expected token '42'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SEMICOLON, ";", token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_KW_FLOAT, "float", token), "Expected token 'float'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "y", token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ, "=", token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_FLOAT, "3.14", token), "Expected token '3.14'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SEMICOLON, ";", token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_KW_IF, "if", token), "Expected token 'if'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_LPAREN, "(", token), "Expected token '('");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "x", token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_LT, "<", token), "Expected token '<'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "y", token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_RPAREN, ")", token), "Expected token ')'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_LBRACE, "{", token), "Expected token '{'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "x", token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ, "=", token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "x", token), "Expected token 'x'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PLUS, "+", token), "Expected token '+'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "1", token), "Expected token '1'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SEMICOLON, ";", token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_RBRACE, "}", token), "Expected token '}'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_KW_ELSE, "else", token), "Expected token 'else'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_LBRACE, "{", token), "Expected token '{'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "y", token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ, "=", token), "Expected token '='");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "y", token), "Expected token 'y'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_MINUS, "-", token), "Expected token '-'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "1", token), "Expected token '1'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SEMICOLON, ";", token), "Expected token ';'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_RBRACE, "}", token), "Expected token '}'");
}

MU_TEST(test_lexer_string_literals) {
    const char *input = "\"hello\" 'c'";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_STRING, "\"hello\"", token), "Expected string literal \"hello\"");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_CHAR, "'c'", token), "Expected char literal 'c'");
}

MU_TEST(test_lexer_comments) {
    const char *input = "// single-line comment\n/* multi-line\ncomment */";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_COMMENT, "// single-line comment", token), "Expected single-line comment");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_COMMENT, "/* multi-line\ncomment */", token), "Expected multi-line comment");
}

MU_TEST(test_lexer_scientific_notation) {
    const char *input = "1.23e4 5E-2";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_FLOAT, "1.23e4", token), "Expected scientific notation 1.23e4");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_FLOAT, "5E-2", token), "Expected scientific notation 5E-2");
}

MU_TEST(test_lexer_operators_and_punctuation) {
    const char *input = "* / % ^ ~ # ##";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_STAR, "*", token), "Expected operator *");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SLASH, "/", token), "Expected operator /");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PERCENT, "%", token), "Expected operator %");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_CARET, "^", token), "Expected operator ^");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_TILDE, "~", token), "Expected operator ~");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PP_HASH, "#", token), "Expected preprocessor token #");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PP_HASHHASH, "##", token), "Expected preprocessor token ##");
}

MU_TEST(test_lexer_char_literals_with_escape_sequences) {
    const char *input = "'\\n' '\\''";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_CHAR, "'\\n'", token), "Expected char literal '\\n'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_CHAR, "'\\''", token), "Expected char literal '\\''");
}

MU_TEST(test_lexer_edge_case_numbers) {
    const char *input = "0x1F 0755 1. .5";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "0x1F", token), "Expected hexadecimal number 0x1F");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "0755", token), "Expected octal number 0755");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_FLOAT, "1.", token), "Expected floating-point number 1.");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_FLOAT, ".5", token), "Expected floating-point number .5");
}

MU_TEST(test_lexer_unmatched_string) {
    const char *input = "\"unterminated string";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_UNKNOWN, "\"unterminated string", token), "Expected unmatched string token");
}

MU_TEST(test_lexer_unmatched_char) {
    const char *input = "'unterminated char";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_UNKNOWN, "'u", token), "Expected unmatched char token");
}

MU_TEST(test_lexer_complex_operators) {
    const char *input = "+= -= && ||";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PLUS_EQ, "+=", token), "Expected operator +=");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_MINUS_EQ, "-=", token), "Expected operator -=");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_AMP_AMP, "&&", token), "Expected operator &&");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PIPE_PIPE, "||", token), "Expected operator ||");
}

MU_TEST(test_lexer_unrecognized_tokens) {
    const char *input = "@ $";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_UNKNOWN, "@", token), "Expected unrecognized token @");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_UNKNOWN, "$", token), "Expected unrecognized token $");
}

MU_TEST(test_lexer_assignment_tokens) {
    const char *input = "x = 42;";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_IDENTIFIER, "x", token), "Expected identifier 'x'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ, "=", token), "Expected '=' token");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_INTEGER, "42", token), "Expected integer '42'");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SEMICOLON, ";", token), "Expected ';' token");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EOF, "", token), "Expected EOF token");
}

MU_TEST(test_lexer_operators) {
    const char *input = "== != < > &= & , [ ] : ? %= ... . -> -- ++ /=";
    Lexer lexer;
    lexer_init(&lexer, input);

    Token token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_EQ_EQ, "==", token), "Expected operator ==");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_BANG_EQ, "!=", token), "Expected operator !=");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_LT, "<", token), "Expected operator <");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_GT, ">", token), "Expected operator >");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_AMP_EQ, "&=", token), "Expected operator &=");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_AMP, "&", token), "Expected operator &");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_COMMA, ",", token), "Expected operator ,");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_LBRACKET, "[", token), "Expected operator [");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_RBRACKET, "]", token), "Expected operator ]");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_COLON, ":", token), "Expected operator :");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_QUESTION, "?", token), "Expected operator ?");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PERCENT_EQ, "%=", token), "Expected operator %=");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_ELLIPSIS, "...", token), "Expected operator ...");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_DOT, ".", token), "Expected operator .");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_ARROW, "->", token), "Expected operator ->");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_MINUS_MINUS, "--", token), "Expected operator --");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_PLUS_PLUS, "++", token), "Expected operator ++");

    token = next_token(&lexer);
    mu_assert(mu_assert_token_eq(TOK_SLASH_EQ, "/=", token), "Expected operator /=");
}

MU_TEST_SUITE(lexer_suite) {
    MU_RUN_TEST(test_lexer_single_token);
    MU_RUN_TEST(test_lexer_multiple_tokens);
    MU_RUN_TEST(test_lexer_all_tokens);
    MU_RUN_TEST(test_lexer_string_literals);
    MU_RUN_TEST(test_lexer_comments);
    MU_RUN_TEST(test_lexer_scientific_notation);
    MU_RUN_TEST(test_lexer_operators_and_punctuation);
    MU_RUN_TEST(test_lexer_char_literals_with_escape_sequences);
    MU_RUN_TEST(test_lexer_edge_case_numbers);
    MU_RUN_TEST(test_lexer_unmatched_string);
    MU_RUN_TEST(test_lexer_unmatched_char);
    MU_RUN_TEST(test_lexer_complex_operators);
    MU_RUN_TEST(test_lexer_unrecognized_tokens);
    MU_RUN_TEST(test_lexer_assignment_tokens);
    // MU_RUN_TEST(test_lexer_raw_string_literals);
    // MU_RUN_TEST(test_lexer_unrecognized_escape_sequences);
    // MU_RUN_TEST(test_lexer_multiline_comment_unterminated);
    MU_RUN_TEST(test_lexer_operators);
}

int main() {
    MU_RUN_SUITE(lexer_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}