#ifndef TAC_H
#define TAC_H

#include "cfg.h"
#include <stdio.h>
#include <stdlib.h>

// TAC instruction types
typedef enum {
    TAC_ASSIGN,      // x = y
    TAC_BINARY_OP,   // x = y op z
    TAC_UNARY_OP,    // x = op y
    TAC_LABEL,       // label:
    TAC_GOTO,        // goto label
    TAC_IF_GOTO,     // if x goto label
    TAC_RETURN,      // return x
    TAC_PHI,         // x = phi(y, z)
    TAC_CALL         // Function call
} TACType;

// TAC instruction structure
typedef struct TAC {
    TACType type;
    char *result;    // Result variable
    char *arg1;      // First argument
    char *arg2;      // Second argument (if applicable)
    char *op;        // Operator (if applicable)
    int *int_label;  // Integer label (for block labels, NULL if not used)
    char *str_label; // String label (for function names, NULL if not used)
    int int_value;   // Integer value (if applicable)
    void *ptr_value; // Pointer value (if applicable)
    struct TAC *next; // Pointer to the next TAC instruction
} TAC;

// Function declarations
void create_tac(CFG *cfg);
void print_tac(CFG *cfg, FILE *stream);
void free_tac(TAC *tac);

#endif // TAC_H