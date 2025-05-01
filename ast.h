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
    //NODE_EXPR, // For general expressions
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

        // Expressions
        // struct {
        //     ExprType op; // Expression type
        //     union {
        //         char *var_name; // Variable name for EXPR_VAR
        //         struct {
        //             LiteralValue value; // Literal value for EXPR_LITERAL
        //         } literal;
        //         struct {
        //             int op; // Operator type for EXPR_BINARY_OP
        //             struct ASTNode *left;
        //             struct ASTNode *right;
        //         } binary_op;
        //         struct {
        //             int op; // Operator type for EXPR_UNARY_OP
        //             struct ASTNode *operand;
        //         } unary_op;
        //     } data;
        // } expr;
    } data;
} ASTNode;

// Function declarations
// ASTNode *create_literal_node(int value, Type *type);
// ASTNode *create_binary_op_node(int op, ASTNode *left, ASTNode *right);
// ASTNode *create_var_ref_node(const char *name, Type *type);
// ASTNode *create_var_decl_node(const char *name, Type *type, ASTNode *init_value);
// ASTNode *create_assignment_node(const char *name, ASTNode *value);
// ASTNode *create_function_decl_node(const char *name, Type *return_type, ASTNode *params, ASTNode *body);
// ASTNode *create_return_node(ASTNode *value);
// ASTNode *create_param_list_node(ASTNode **params, size_t count);
// ASTNode *create_stmt_list_node(ASTNode **stmts, size_t count);
// ASTNode *create_type_spec_node(Type *type);

// const char *node_type_to_string(NodeType type);

// Type *create_type(TypeKind kind);
// Type *create_pointer_type(Type *base);
// Type *create_array_type(Type *base, size_t size);

// void free_ast(ASTNode *node);
// void print_ast(ASTNode *node, int indent);

#endif // AST_H