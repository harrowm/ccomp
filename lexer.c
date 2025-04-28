/*
 * File: lexer.c
 * Description: Implements the lexical analyzer for tokenizing source code.
 * Purpose: Converts source code into a stream of tokens for parsing.
 */

#include "lexer.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct {
    const char* str;
    TokenType type;
    size_t length;
} Keyword;

static const Keyword keywords[] = {
    // Standard C keywords
    {"auto", TOK_KW_AUTO, 4},
    {"break", TOK_KW_BREAK, 5},
    {"case", TOK_KW_CASE, 4},
    {"char", TOK_KW_CHAR, 4},
    {"const", TOK_KW_CONST, 5},
    {"continue", TOK_KW_CONTINUE, 8},
    {"default", TOK_KW_DEFAULT, 7},
    {"do", TOK_KW_DO, 2},
    {"double", TOK_KW_DOUBLE, 6},
    {"else", TOK_KW_ELSE, 4},
    {"enum", TOK_KW_ENUM, 4},
    {"extern", TOK_KW_EXTERN, 6},
    {"float", TOK_KW_FLOAT, 5},
    {"for", TOK_KW_FOR, 3},
    {"goto", TOK_KW_GOTO, 4},
    {"if", TOK_KW_IF, 2},
    {"inline", TOK_KW_INLINE, 6},
    {"int", TOK_KW_INT, 3},
    {"long", TOK_KW_LONG, 4},
    {"register", TOK_KW_REGISTER, 8},
    {"restrict", TOK_KW_RESTRICT, 8},
    {"return", TOK_KW_RETURN, 6},
    {"short", TOK_KW_SHORT, 5},
    {"signed", TOK_KW_SIGNED, 6},
    {"sizeof", TOK_KW_SIZEOF, 6},
    {"static", TOK_KW_STATIC, 6},
    {"struct", TOK_KW_STRUCT, 6},
    {"switch", TOK_KW_SWITCH, 6},
    {"typedef", TOK_KW_TYPEDEF, 7},
    {"union", TOK_KW_UNION, 5},
    {"unsigned", TOK_KW_UNSIGNED, 8},
    {"void", TOK_KW_VOID, 4},
    {"volatile", TOK_KW_VOLATILE, 8},
    {"while", TOK_KW_WHILE, 5},
    
    // C99 keywords
    {"_Bool", TOK_KW__BOOL, 5},
    {"_Complex", TOK_KW__COMPLEX, 8},
    {"_Imaginary", TOK_KW__IMAGINARY, 10},
    
    // C11 keywords
    {"_Alignas", TOK_KW__ALIGNAS, 8},
    {"_Alignof", TOK_KW__ALIGNOF, 8},
    {"_Atomic", TOK_KW__ATOMIC, 7},
    {"_Generic", TOK_KW__GENERIC, 8},
    {"_Noreturn", TOK_KW__NORETURN, 9},
    {"_Static_assert", TOK_KW__STATIC_ASSERT, 14},
    {"_Thread_local", TOK_KW__THREAD_LOCAL, 13},
    
    // C23 keywords
    {"_BitInt", TOK_KW__BITINT, 7},
    {"_Decimal128", TOK_KW__DECIMAL128, 11},
    {"_Decimal32", TOK_KW__DECIMAL32, 10},
    {"_Decimal64", TOK_KW__DECIMAL64, 10},
    {"false", TOK_KW_FALSE, 5},
    {"true", TOK_KW_TRUE, 4},
    {"nullptr", TOK_KW_NULLPTR, 7},
    {"typeof", TOK_KW_TYPEOF, 6},
    {"typeof_unqual", TOK_KW_TYPEOF_UNQUAL, 13}
};
static const size_t num_keywords = sizeof(keywords)/sizeof(keywords[0]);

/* Command-line: gperf -m10 -N is_keyword  */
/* Computed positions: -k'1,$' */

#define TOTAL_KEYWORDS 62
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 14
#define MIN_HASH_VALUE 8
#define MAX_HASH_VALUE 70
/* maximum key range = 63, duplicates = 0 */

static unsigned int hash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 57, 71, 56, 71, 71, 71, 53, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,  1, 71, 31, 36, 19,
       4, 21, 13, 46, 53, 34, 71, 21, 20, 34, 26,  6, 71, 71, 22,  4,  0, 35, 35, 32,
      31,  5, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}



static TokenType get_keyword_type(register const char *str, register size_t len)
{
    static const char * wordlist[] = {
      "", "", "", "", "", "", "", "",
      "_BitInt", "short", "struct", "default", "do", "_Alignas", "signed", "_Static_assert",
      "_Imaginary", "static_assert", "float", "typeof", "typedef", "_typeof", "_Alignof", "sizeof",
      "const", "true", "_Bool", "_Atomic", "_Generic", "static", "restrict", "double", "thread_local",
      "typeof_unqual", "_Thread_local", "_typeof_unqual", "_Noreturn", "int", "for", "false", "_Complex",
      "auto", "alignas", "void", "case", "char", "else", "unsigned", "continue", "if", "constexpr",
      "alignof", "register", "extern", "return", "nullptr", "goto", "complex", "while", "enum", "bool",
      "inline", "break", "switch", "volatile", "_Decimal128", "union", "_Decimal64", "_Decimal32",
      "", "long" 
    };

    static const TokenType hash_to_token[MAX_HASH_VALUE + 1] = {
        // Default initialization
        [0] = TOK_IDENTIFIER, [1] = TOK_IDENTIFIER, [2] = TOK_IDENTIFIER,
        [3] = TOK_IDENTIFIER, [4] = TOK_IDENTIFIER, [5] = TOK_IDENTIFIER,
        [6] = TOK_IDENTIFIER, [7] = TOK_IDENTIFIER,
        
        // Actual mappings
        [8] = TOK_KW__BITINT, [9] = TOK_KW_SHORT, [10] = TOK_KW_STRUCT, [11] = TOK_KW_DEFAULT,
        [12] = TOK_KW_DO, [13] = TOK_KW__ALIGNAS, [14] = TOK_KW_SIGNED, [15] = TOK_KW__STATIC_ASSERT,
        [16] = TOK_KW__IMAGINARY, [17] = TOK_KW__STATIC_ASSERT, [18] = TOK_KW_FLOAT, [19] = TOK_KW_TYPEOF,
        [20] = TOK_KW_TYPEDEF, [21] = TOK_KW_TYPEOF, [22] = TOK_KW__ALIGNOF, [23] = TOK_KW_SIZEOF,
        [24] = TOK_KW_CONST, [25] = TOK_KW_TRUE, [26] = TOK_KW__BOOL, [27] = TOK_KW__ATOMIC,
        [28] = TOK_KW__GENERIC, [29] = TOK_KW_STATIC, [30] = TOK_KW_RESTRICT, [31] = TOK_KW_DOUBLE,
        [32] = TOK_KW__THREAD_LOCAL, [33] = TOK_KW_TYPEOF_UNQUAL, [34] = TOK_KW__THREAD_LOCAL, [35] = TOK_KW_TYPEOF_UNQUAL,
        [36] = TOK_KW__NORETURN, [37] = TOK_KW_INT, [38] = TOK_KW_FOR, [39] = TOK_KW_FALSE,
        [40] = TOK_KW__COMPLEX, [41] = TOK_KW_AUTO, [42] = TOK_KW__ALIGNAS, [43] = TOK_KW_VOID,
        [44] = TOK_KW_CASE, [45] = TOK_KW_CHAR, [46] = TOK_KW_ELSE, [47] = TOK_KW_UNSIGNED,
        [48] = TOK_KW_CONTINUE, [49] = TOK_KW_IF,
          // 50 is constexpr which maps to IDENTIFIER (default)
        [51] = TOK_KW__ALIGNOF, [52] = TOK_KW_REGISTER, [53] = TOK_KW_EXTERN, [54] = TOK_KW_RETURN,
        [55] = TOK_KW_NULLPTR, [56] = TOK_KW_GOTO, [57] = TOK_KW__COMPLEX, [58] = TOK_KW_WHILE,
        [59] = TOK_KW_ENUM, [60] = TOK_KW__BOOL, [61] = TOK_KW_INLINE, [62] = TOK_KW_BREAK,
        [63] = TOK_KW_SWITCH, [64] = TOK_KW_VOLATILE, [65] = TOK_KW__DECIMAL128, [66] = TOK_KW_UNION,
        [67] = TOK_KW__DECIMAL64, [68] = TOK_KW__DECIMAL32,
        // 69 is empty (defaults to IDENTIFIER)
        [70] = TOK_KW_LONG
    };

    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
        register unsigned int key = hash (str, len);

        LOG_INFO("lexer hash: %u, key: %u, str: %.*s", key, key, (int)len, str);

        if ((key <= MAX_HASH_VALUE) && (!strncmp (str, wordlist[key], len)))
            return hash_to_token[key];
    }
    return TOK_IDENTIFIER;
}

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
}

static Token make_token(Lexer* lexer, TokenType type, const char* start, size_t length) {
    return (Token){
        .type = type,
        .text = start,
        .length = length,
        .line = lexer->line,
        .column = lexer->column - (uint_least32_t)length
    };
}

static void advance(Lexer* lexer) {
    if (*lexer->current == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->current++;
}

static Token lex_identifier(Lexer* lexer) {
    const char* start = lexer->current;
    while (isalnum(*lexer->current) || *lexer->current == '_') {
        advance(lexer);
    }
    size_t length = lexer->current - start;
    TokenType type = get_keyword_type(start, length);
    return make_token(lexer, type, start, length);
}

static Token lex_number(Lexer* lexer) {
    const char* start = lexer->current;
    bool is_float = false;
    
    while (isdigit(*lexer->current)) advance(lexer);
    
    if (*lexer->current == '.') {
        is_float = true;
        advance(lexer);
        while (isdigit(*lexer->current)) advance(lexer);
    }
    
    if (*lexer->current == 'e' || *lexer->current == 'E') {
        is_float = true;
        advance(lexer);
        if (*lexer->current == '+' || *lexer->current == '-') advance(lexer);
        while (isdigit(*lexer->current)) advance(lexer);
    }
    
    if (*lexer->current == 'u' || *lexer->current == 'U') {
        advance(lexer);
        if (*lexer->current == 'l' || *lexer->current == 'L') advance(lexer);
    }
    else if (*lexer->current == 'l' || *lexer->current == 'L') {
        advance(lexer);
        if (*lexer->current == 'u' || *lexer->current == 'U') advance(lexer);
    }
    else if (*lexer->current == 'f' || *lexer->current == 'F') {
        is_float = true;
        advance(lexer);
    }
    
    size_t length = lexer->current - start;
    return make_token(lexer, is_float ? TOK_FLOAT : TOK_INTEGER, start, length);
}

static Token lex_string(Lexer* lexer, bool raw) {
    const char* start = lexer->current;
    char quote = *lexer->current;
    advance(lexer);
    
    while (*lexer->current != quote && *lexer->current != '\0') {
        if (*lexer->current == '\\' && !raw) advance(lexer);
        advance(lexer);
    }
    
    if (*lexer->current == quote) advance(lexer);
    
    size_t length = lexer->current - start;
    return make_token(lexer, raw ? TOK_RAW_STRING : TOK_STRING, start, length);
}

static Token lex_comment(Lexer* lexer) {
    const char* start = lexer->current;
   
    if (*lexer->current == '/') {
        advance(lexer);
        while (*lexer->current != '\n' && *lexer->current != '\0') advance(lexer);
    } else if (*lexer->current == '*') {
        advance(lexer);
        while (*lexer->current != '\0') {
            if (*lexer->current == '*' && lexer->current[1] == '/') {
                advance(lexer);
                advance(lexer);
                break;
            }
            advance(lexer);
        }
    }
    
    size_t length = lexer->current - start;
    return make_token(lexer, TOK_COMMENT, start, length);
}

Token next_token(Lexer* lexer) {
    while (isspace(*lexer->current)) advance(lexer);
    
    const char* start = lexer->current;
    
    if (*lexer->current == '\0') return make_token(lexer, TOK_EOF, start, 0);
    
    if (isalpha(*lexer->current) || *lexer->current == '_') return lex_identifier(lexer);
    if (isdigit(*lexer->current)) return lex_number(lexer);
    
    switch (*lexer->current) {
        case '=':
        advance(lexer);
        if (*lexer->current == '=') {
            advance(lexer);
            return make_token(lexer, TOK_EQ_EQ, start, 2);
        }
        return make_token(lexer, TOK_EQ, start, 1);
        case '!':
            advance(lexer);
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_BANG_EQ, start, 2);
            }
            return make_token(lexer, TOK_BANG, start, 1);
        case '<':
            advance(lexer);
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_LT_EQ, start, 2);
            }
            return make_token(lexer, TOK_LT, start, 1);
        case '>':
            advance(lexer);
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_GT_EQ, start, 2);
            }
            return make_token(lexer, TOK_GT, start, 1);
        case '&':
            advance(lexer);
            if (*lexer->current == '&') {
                advance(lexer);
                return make_token(lexer, TOK_AMP_AMP, start, 2);
            }
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_AMP_EQ, start, 2);
            }
            return make_token(lexer, TOK_AMP, start, 1);
        case '|':
            advance(lexer);
            if (*lexer->current == '|') {
                advance(lexer);
                return make_token(lexer, TOK_PIPE_PIPE, start, 2);
            }
            break;
        case ',': advance(lexer); return make_token(lexer, TOK_COMMA, start, 1);
        case '(': advance(lexer); return make_token(lexer, TOK_LPAREN, start, 1);
        case ')': advance(lexer); return make_token(lexer, TOK_RPAREN, start, 1);
        case '{': advance(lexer); return make_token(lexer, TOK_LBRACE, start, 1);
        case '}': advance(lexer); return make_token(lexer, TOK_RBRACE, start, 1);
        case ';': advance(lexer); return make_token(lexer, TOK_SEMICOLON, start, 1);
        case '[': advance(lexer); return make_token(lexer, TOK_LBRACKET, start, 1);
        case ']': advance(lexer); return make_token(lexer, TOK_RBRACKET, start, 1);
        case ':': advance(lexer); return make_token(lexer, TOK_COLON, start, 1);
        case '?': advance(lexer); return make_token(lexer, TOK_QUESTION, start, 1);
        case '~': advance(lexer); return make_token(lexer, TOK_TILDE, start, 1);
        case '%':
            advance(lexer);
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_PERCENT_EQ, start, 2);
            }
            return make_token(lexer, TOK_PERCENT, start, 1);
        case '^':
            advance(lexer);
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_CARET_EQ, start, 2);
            }
            return make_token(lexer, TOK_CARET, start, 1);
        
        case '.': 
            advance(lexer);
            if (*lexer->current == '.' && lexer->current[1] == '.') {
                advance(lexer); advance(lexer);
                return make_token(lexer, TOK_ELLIPSIS, start, 3);
            }
            return make_token(lexer, TOK_DOT, start, 1);
        case '-':
            advance(lexer);
            if (*lexer->current == '>') {
                advance(lexer);
                return make_token(lexer, TOK_ARROW, start, 2);
            }
            if (*lexer->current == '-') {
                advance(lexer);
                return make_token(lexer, TOK_MINUS_MINUS, start, 2);
            }
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_MINUS_EQ, start, 2);
            }
            return make_token(lexer, TOK_MINUS, start, 1);
        case '+':
            advance(lexer);
            if (*lexer->current == '+') {
                advance(lexer);
                return make_token(lexer, TOK_PLUS_PLUS, start, 2);
            }
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_PLUS_EQ, start, 2);
            }
            return make_token(lexer, TOK_PLUS, start, 1);
        // * is a pain as it can be a pointer, multiplication, power or dereference
        // apart from *= treat everything as STAR and let the parser figure it out
        case '*':
            advance(lexer);
            if (*lexer->current == '=') {  // *=
                advance(lexer);
                return make_token(lexer, TOK_STAR_EQ, start, 2);
            }
            return make_token(lexer, TOK_STAR, start, 1);
        case '/':
            advance(lexer);
            if (*lexer->current == '/' || *lexer->current == '*') 
                return lex_comment(lexer);
            if (*lexer->current == '=') {
                advance(lexer);
                return make_token(lexer, TOK_SLASH_EQ, start, 2);
            }
            return make_token(lexer, TOK_SLASH, start, 1);
        case '\'': return lex_string(lexer, false);
        case '"': return lex_string(lexer, false);
        case 'R':
            if (lexer->current[1] == '"') {
                advance(lexer);
                return lex_string(lexer, true);
            }
            break;
        case '#':
            advance(lexer);
            if (*lexer->current == '#') {
                advance(lexer);
                return make_token(lexer, TOK_PP_HASHHASH, start, 2);
            }
            return make_token(lexer, TOK_PP_HASH, start, 1);
    }
    
    advance(lexer);
    return make_token(lexer, TOK_UNKNOWN, start, 1);
}

const char* token_type_to_string(TokenType type) {
    static const char* const names[] = {
        // Keywords
        [TOK_KW_AUTO] = "KW_AUTO",
        [TOK_KW_BREAK] = "KW_BREAK",
        [TOK_KW_CASE] = "KW_CASE",
        [TOK_KW_CHAR] = "KW_CHAR",
        [TOK_KW_CONST] = "KW_CONST",
        [TOK_KW_CONTINUE] = "KW_CONTINUE",
        [TOK_KW_DEFAULT] = "KW_DEFAULT",
        [TOK_KW_DO] = "KW_DO",
        [TOK_KW_DOUBLE] = "KW_DOUBLE",
        [TOK_KW_ELSE] = "KW_ELSE",
        [TOK_KW_ENUM] = "KW_ENUM",
        [TOK_KW_EXTERN] = "KW_EXTERN",
        [TOK_KW_FLOAT] = "KW_FLOAT",
        [TOK_KW_FOR] = "KW_FOR",
        [TOK_KW_GOTO] = "KW_GOTO",
        [TOK_KW_IF] = "KW_IF",
        [TOK_KW_INLINE] = "KW_INLINE",
        [TOK_KW_INT] = "KW_INT",
        [TOK_KW_LONG] = "KW_LONG",
        [TOK_KW_REGISTER] = "KW_REGISTER",
        [TOK_KW_RESTRICT] = "KW_RESTRICT",
        [TOK_KW_RETURN] = "KW_RETURN",
        [TOK_KW_SHORT] = "KW_SHORT",
        [TOK_KW_SIGNED] = "KW_SIGNED",
        [TOK_KW_SIZEOF] = "KW_SIZEOF",
        [TOK_KW_STATIC] = "KW_STATIC",
        [TOK_KW_STRUCT] = "KW_STRUCT",
        [TOK_KW_SWITCH] = "KW_SWITCH",
        [TOK_KW_TYPEDEF] = "KW_TYPEDEF",
        [TOK_KW_UNION] = "KW_UNION",
        [TOK_KW_UNSIGNED] = "KW_UNSIGNED",
        [TOK_KW_VOID] = "KW_VOID",
        [TOK_KW_VOLATILE] = "KW_VOLATILE",
        [TOK_KW_WHILE] = "KW_WHILE",
        [TOK_KW__BOOL] = "KW__BOOL",
        [TOK_KW__COMPLEX] = "KW__COMPLEX",
        [TOK_KW__IMAGINARY] = "KW__IMAGINARY",
        [TOK_KW__ALIGNAS] = "KW__ALIGNAS",
        [TOK_KW__ALIGNOF] = "KW__ALIGNOF",
        [TOK_KW__ATOMIC] = "KW__ATOMIC",
        [TOK_KW__GENERIC] = "KW__GENERIC",
        [TOK_KW__NORETURN] = "KW__NORETURN",
        [TOK_KW__STATIC_ASSERT] = "KW__STATIC_ASSERT",
        [TOK_KW__THREAD_LOCAL] = "KW__THREAD_LOCAL",
        [TOK_KW__BITINT] = "KW__BITINT",
        [TOK_KW__DECIMAL128] = "KW__DECIMAL128",
        [TOK_KW__DECIMAL32] = "KW__DECIMAL32",
        [TOK_KW__DECIMAL64] = "KW__DECIMAL64",
        [TOK_KW_TRUE] = "KW_TRUE",
        [TOK_KW_FALSE] = "KW_FALSE",
        [TOK_KW_NULLPTR] = "KW_NULLPTR",
        [TOK_KW_TYPEOF] = "KW_TYPEOF",
        [TOK_KW_TYPEOF_UNQUAL] = "KW_TYPEOF_UNQUAL",
        
        // Literals
        [TOK_INTEGER] = "INTEGER",
        [TOK_FLOAT] = "FLOAT",
        [TOK_CHAR] = "CHAR",
        [TOK_STRING] = "STRING",
        
        // Identifiers
        [TOK_IDENTIFIER] = "IDENTIFIER",
        
        // Operators
        [TOK_PLUS] = "PLUS",
        [TOK_MINUS] = "MINUS",
        [TOK_STAR] = "STAR",
        [TOK_SLASH] = "SLASH",
        [TOK_PERCENT] = "PERCENT",
        [TOK_EQ] = "EQ",
        [TOK_EQ_EQ] = "EQ_EQ",
        [TOK_BANG_EQ] = "BANG_EQ",
        [TOK_LT] = "LT",
        [TOK_LT_EQ] = "LT_EQ",
        [TOK_GT] = "GT",
        [TOK_GT_EQ] = "GT_EQ",
        [TOK_AMP_AMP] = "AMP_AMP",
        [TOK_PIPE_PIPE] = "PIPE_PIPE",
        [TOK_PLUS_PLUS] = "INCREMENT",
        [TOK_MINUS_MINUS] = "DECREMENT",
        
        // Punctuation
        [TOK_LPAREN] = "LPAREN",
        [TOK_RPAREN] = "RPAREN",
        [TOK_LBRACKET] = "LBRACKET",
        [TOK_RBRACKET] = "RBRACKET",
        [TOK_LBRACE] = "LBRACE",
        [TOK_RBRACE] = "RBRACE",
        [TOK_SEMICOLON] = "SEMICOLON",
        [TOK_COMMA] = "COMMA",
        
        // Special
        [TOK_EOF] = "EOF",
        [TOK_UNKNOWN] = "UNKNOWN",
        
        // Additional operators
        [TOK_AMP] = "AMPERSAND",
        [TOK_PIPE] = "PIPE",
        [TOK_TILDE] = "TILDE",
        [TOK_CARET] = "CARET",
        [TOK_LSHIFT] = "LSHIFT",
        [TOK_RSHIFT] = "RSHIFT",
        
        // Compound assignment operators
        [TOK_PLUS_EQ] = "PLUS_EQ",
        [TOK_MINUS_EQ] = "MINUS_EQ",
        [TOK_STAR_EQ] = "STAR_EQ",
        [TOK_SLASH_EQ] = "SLASH_EQ",
        [TOK_PERCENT_EQ] = "PERCENT_EQ",
        [TOK_AMP_EQ] = "AMP_EQ",
        [TOK_PIPE_EQ] = "PIPE_EQ",
        [TOK_CARET_EQ] = "CARET_EQ",
        [TOK_LSHIFT_EQ] = "LSHIFT_EQ",
        [TOK_RSHIFT_EQ] = "RSHIFT_EQ",
        
        // Other operators
        [TOK_BANG] = "BANG",
        [TOK_QUESTION] = "QUESTION",
        [TOK_COLON] = "COLON",
        [TOK_ARROW] = "ARROW",
        [TOK_DOT] = "DOT",
        [TOK_ELLIPSIS] = "ELLIPSIS"
    };
    
    // Safety check
    if (type >= 0 && type < (sizeof(names)/sizeof(names[0]))) {
        return names[type] ? names[type] : "UNKNOWN_TOKEN";
    }
    return "INVALID_TOKEN";
}
