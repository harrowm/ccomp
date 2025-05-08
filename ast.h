/*
 * File: ast.h
 * Description: Defines the Abstract Syntax Tree (AST) structure and related functions.
 * Purpose: Used to represent the structure of the parsed source code in memory.
 */

#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DECL,
    NODE_VAR_DECL,
    NODE_VAR_REF,
    NODE_ASSIGNMENT,
    NODE_BINARY_OP,
    NODE_RETURN,
    NODE_LITERAL,
    NODE_PARAM_LIST,
    NODE_STMT_LIST,
    NODE_TYPE_SPECIFIER,
    NODE_IF_STMT,
    NODE_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_UNARY_OP, // For unary operations like --, ++, etc.
    NODE_FUNCTION_CALL, // For function calls like foo(a, b)
    INVALID_NODE_TYPE,
    UNKNOWN_NODE_TYPE
} NodeType;

// Add expression types
typedef enum {
    EXPR_VAR, // Variable reference
    EXPR_LITERAL, // Literal value
    EXPR_BINARY_OP, // Binary operation
    EXPR_UNARY_OP // Unary operation
} ExprType;

typedef enum {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_POINTER,
    TYPE_ARRAY
} TypeKind;
    
typedef struct Type {
    TypeKind kind;
    struct Type *base; // For pointer/array types
    size_t array_size; // For array types
} Type;

typedef union {
    int int_value;
    void *ptr_value;
} LiteralValue;

typedef struct ASTNode {
    NodeType type;
    char *temp_var; // Temporary variable associated with this node
    union {
        // Literal value
        struct {
            LiteralValue value;
            Type *type;
        } literal;

        // Binary operation
        struct {
            int op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;

        // Unary operation
        struct {
            int op; // Operator type (e.g., --, ++)
            struct ASTNode *operand; // Operand of the operation
            bool is_prefix; // True if prefix, false if postfix
        } unary_op;
        
        // Variable reference
        struct {
            char *name;
            Type *type;
        } var_ref;
        
        // Variable declaration
        struct {
            char *name;
            Type *type;
            struct ASTNode *init_value;
        } var_decl;
        
        // Assignment
        struct {
            char *name;
            struct ASTNode *value;
        } assignment;

        // If statement
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch; // Can be NULL
        } if_stmt;
        
        // While loop
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // For loop
        struct {
            struct ASTNode *init;      // Can be NULL
            struct ASTNode *condition; // Can be NULL
            struct ASTNode *update;    // Can be NULL
            struct ASTNode *body;
        } for_stmt;
        
        // Function declaration
        struct {
            char *name;
            Type *return_type;
            struct ASTNode *params;
            struct ASTNode *body;
        } function_decl;
        
        // Function call
        struct {
            char *name; // Function name
            struct ASTNode **args; // Array of argument expressions
            size_t arg_count; // Number of arguments
        } function_call;

        // Return statement
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // Parameter list
        struct {
            struct ASTNode **params;
            size_t count;
        } param_list;
        
        // Statement list
        struct {
            struct ASTNode **stmts;
            size_t count;
        } stmt_list;
        
        // Type specifier
        struct {
            Type *type;
        } type_spec;

    } data;
} ASTNode;

#endif // AST_H