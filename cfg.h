// cfg.h
#ifndef CFG_H
#define CFG_H

#include "ast.h"
#include "debug.h"
#include <stdbool.h>
#include <stdlib.h>

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
    struct BasicBlock **dom_frontier;
    size_t df_count;
    size_t df_capacity;
} BasicBlock;

typedef struct {
    BasicBlock *entry;
    BasicBlock *exit;
    BasicBlock **blocks;
    size_t block_count;
    size_t block_capacity;
} CFG;

CFG* ast_to_cfg(ASTNode *ast);
void free_cfg(CFG *cfg);
void print_cfg(CFG *cfg, FILE *stream);
void generate_dot_file(CFG *cfg, const char *filename);

#endif // CFG_H
