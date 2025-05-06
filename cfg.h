// cfg.h
/*
 * File: cfg.h
 * Description: Declares the structures and functions for Control Flow Graph (CFG) management.
 * Purpose: Provides an interface for constructing and analyzing CFGs.
 */

#ifndef CFG_H
#define CFG_H

#include "ast.h"
#include "debug.h"
#include <stdbool.h>
#include <stdlib.h>

// Forward declarations to resolve circular dependencies
struct BasicBlock;
struct CFG;

// Structure to represent dominance frontiers
typedef struct DominanceFrontier {
    struct BasicBlock **blocks; // Array of blocks in the dominance frontier
    size_t count;       // Number of blocks in the frontier
    size_t capacity;    // Capacity of the array
} DominanceFrontier;

typedef enum {
    BLOCK_NORMAL,
    BLOCK_ENTRY,
    BLOCK_EXIT,
    BLOCK_IF_THEN,
    BLOCK_IF_ELSE,
    BLOCK_LOOP_HEADER,
    BLOCK_LOOP_BODY
} BlockType;

typedef struct BasicBlock {
    size_t id;
    BlockType type;
    ASTNode **stmts;
    size_t stmt_count;
    size_t stmt_capacity;
    
    struct BasicBlock **preds;
    size_t pred_count;
    size_t pred_capacity;
    
    struct BasicBlock **succs;
    size_t succ_count;
    size_t succ_capacity;
    
    // For dominance calculations
    struct BasicBlock *dominator;
    DominanceFrontier *dom_frontier; // Pointer to the dominance frontier structure
    size_t df_count;
    size_t df_capacity;

    struct BasicBlock **dominated; // Array of blocks dominated by this block
    size_t dominated_count;       // Number of blocks dominated
    size_t dominated_capacity;    // Capacity of the dominated array
} BasicBlock;

typedef struct {
    BasicBlock *entry;
    BasicBlock *exit;
    BasicBlock **blocks; // the dominance frontier
    size_t block_count;
    size_t block_capacity;
} CFG;

CFG* ast_to_cfg(ASTNode *ast);
void free_cfg(CFG *cfg);
void print_cfg(CFG *cfg, FILE *stream);
void generate_dot_file(CFG *cfg, const char *filename);

// Function declarations for dominance frontiers
void compute_dominance_frontiers(CFG *cfg);
void free_dominance_frontiers(CFG *cfg);
void generate_dominance_frontiers_dot(CFG *cfg, const char *filename);
void print_dominance_frontiers(CFG *cfg, FILE *stream);
void compute_dominator_tree(CFG *cfg);
void insert_phi_functions(CFG *cfg);

const char* block_type_to_string(BlockType type);

#endif // CFG_H
