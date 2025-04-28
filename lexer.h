/*
 * File: lexer.h
 * Description: Declares the structures and functions for lexical analysis.
 * Purpose: Provides an interface for tokenizing source code.
 */

// Missing fro token types are:
// - constexpr
// - attribute types [[fallthrough]] etc

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    // Keywords (C23)
    TOK_KW_AUTO, TOK_KW_BREAK, TOK_KW_CASE, TOK_KW_CHAR,
    TOK_KW_CONST, TOK_KW_CONTINUE, TOK_KW_DEFAULT, TOK_KW_DO,
    TOK_KW_DOUBLE, TOK_KW_ELSE, TOK_KW_ENUM, TOK_KW_EXTERN,
    TOK_KW_FLOAT, TOK_KW_FOR, TOK_KW_GOTO, TOK_KW_IF,
    TOK_KW_INLINE, TOK_KW_INT, TOK_KW_LONG, TOK_KW_REGISTER,
    TOK_KW_RESTRICT, TOK_KW_RETURN, TOK_KW_SHORT, TOK_KW_SIGNED,
    TOK_KW_SIZEOF, TOK_KW_STATIC, TOK_KW_STRUCT, TOK_KW_SWITCH,
    TOK_KW_TYPEDEF, TOK_KW_UNION, TOK_KW_UNSIGNED, TOK_KW_VOID,
    TOK_KW_VOLATILE, TOK_KW_WHILE,
    TOK_KW__BOOL, TOK_KW__COMPLEX, TOK_KW__IMAGINARY,
    TOK_KW__ALIGNAS, TOK_KW__ALIGNOF, TOK_KW__ATOMIC,
    TOK_KW__GENERIC, TOK_KW__NORETURN, TOK_KW__STATIC_ASSERT,
    TOK_KW__THREAD_LOCAL, TOK_KW__BITINT, TOK_KW__DECIMAL128,
    TOK_KW__DECIMAL32, TOK_KW__DECIMAL64, TOK_KW_TRUE,
    TOK_KW_FALSE, TOK_KW_NULLPTR, TOK_KW_TYPEOF, TOK_KW_TYPEOF_UNQUAL,
    
    // Literals
    TOK_INTEGER, TOK_FLOAT, TOK_CHAR, TOK_STRING, TOK_RAW_STRING,
    
    // Identifiers
    TOK_IDENTIFIER,
    
    // Operators
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_AMP, TOK_PIPE, TOK_CARET, TOK_TILDE, TOK_BANG,
    TOK_QUESTION, TOK_COLON, TOK_EQ, TOK_LT, TOK_GT,
    TOK_PLUS_EQ, TOK_MINUS_EQ, TOK_STAR_EQ, TOK_SLASH_EQ,
    TOK_PERCENT_EQ, TOK_AMP_EQ, TOK_PIPE_EQ, TOK_CARET_EQ,
    TOK_LSHIFT, TOK_RSHIFT, TOK_LSHIFT_EQ, TOK_RSHIFT_EQ,
    TOK_EQ_EQ, TOK_BANG_EQ, TOK_LT_EQ, TOK_GT_EQ,
    TOK_AMP_AMP, TOK_PIPE_PIPE, TOK_PLUS_PLUS, TOK_MINUS_MINUS,
    TOK_ARROW, TOK_DOT, TOK_ELLIPSIS,
    
    // Punctuation
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET, TOK_COMMA, TOK_SEMICOLON,
    
    // Preprocessor
    TOK_PP_HASH, TOK_PP_HASHHASH, TOK_PP_HASH_PASTE,
    
    // Special
    TOK_EOF, TOK_UNKNOWN, TOK_COMMENT, TOK_WHITESPACE
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    const char* text;
    size_t length;
    uint_least32_t line;
    uint_least32_t column;
} Token;

/* Lexer state structure */
typedef struct {
    const char* source;
    const char* current;
    uint_least32_t line;
    uint_least32_t column;
} Lexer;

/* Initialization */
void lexer_init(Lexer* lexer, const char* source);

/* Core lexing function */
Token next_token(Lexer* lexer);

/* Utility functions */
const char* token_type_to_string(TokenType type);


#endif /* LEXER_H */