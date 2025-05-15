/*
 * File: dominance.c
 * Description: Implements functions for computing and managing dominance frontiers.
 * Purpose: Used to analyze dominance relationships in a Control Flow Graph (CFG).
 */

#include "cfg.h" 
#include "debug.h" // Include debug.h for logging macros
#include <stdlib.h>
#include <stdio.h>

// Helper function to add a block to a dominance frontier
static void add_to_dominance_frontier(DominanceFrontier *df, BasicBlock *block) {
    if (df->count >= df->capacity) {
        size_t new_capacity = df->capacity == 0 ? 4 : df->capacity * 2;
        BasicBlock **new_blocks = realloc(df->blocks, sizeof(BasicBlock *) * new_capacity);
        if (!new_blocks) {
            LOG_ERROR("Error: Unable to allocate memory for dominance frontier.");
            return;
        }
        df->blocks = new_blocks;
        df->capacity = new_capacity;
    }
    df->blocks[df->count++] = block;
}

// Compute dominance frontiers for all blocks in the CFG
void compute_dominance_frontiers(CFG *cfg) {
    if (!cfg) {
        LOG_ERROR("NULL CFG pointer");
        return;
    }

    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        block->dom_frontier = malloc(sizeof(DominanceFrontier));
        if (!block->dom_frontier) {
            LOG_ERROR("Unable to allocate memory for dominance frontier");
            continue;
        }
        block->dom_frontier->blocks = NULL;
        block->dom_frontier->count = 0;
        block->dom_frontier->capacity = 0;

        LOG_INFO("Computing dominance frontiers for block %zu", block->id);

        // Debugging local dominance frontier computation
        for (size_t j = 0; j < block->succ_count; j++) {
            BasicBlock *succ = block->succs[j];
            if (succ->dominator != block) {
                LOG_INFO("Adding successor Block%zu to dominance frontier of Block%zu", succ->id, block->id);
                add_to_dominance_frontier(block->dom_frontier, succ);
            }
        }

        // Ensure dominator's dominance frontier is initialized
        if (block->dominator && !block->dominator->dom_frontier) {
            block->dominator->dom_frontier = malloc(sizeof(DominanceFrontier));
            if (!block->dominator->dom_frontier) {
                LOG_ERROR("Unable to allocate memory for dominator's dominance frontier");
                continue;
            }
            block->dominator->dom_frontier->blocks = NULL;
            block->dominator->dom_frontier->count = 0;
            block->dominator->dom_frontier->capacity = 0;
        }

        // Debugging upward propagation
        if (block->dominator) {
            DominanceFrontier *dom_frontier = block->dominator->dom_frontier;
            for (size_t j = 0; j < dom_frontier->count; j++) {
                BasicBlock *df_block = dom_frontier->blocks[j];
                if (df_block->dominator != block) {
                    LOG_INFO("Propagating Block%zu to dominance frontier of Block%zu", df_block->id, block->id);
                    add_to_dominance_frontier(block->dom_frontier, df_block);
                }
            }
        }

        // Debugging final dominance frontier for the block
        LOG_INFO("Final dominance frontier for Block%zu:", block->id);
        for (size_t j = 0; j < block->dom_frontier->count; j++) {
            LOG_INFO("  Block%zu", block->dom_frontier->blocks[j]->id);
        }
    }
}

// Free the memory allocated for dominance frontiers
void free_dominance_frontiers(CFG *cfg) {
    if (!cfg) return;

    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        if (block->dom_frontier) {
            free(block->dom_frontier->blocks);
            free(block->dom_frontier);
        }
    }
}

// Generate a DOT file representing the dominance frontiers of a CFG
void generate_dominance_frontiers_dot(CFG *cfg, const char *filename) {
    if (!cfg || !filename) return;

    FILE *file = fopen(filename, "w");
    if (!file) {
        LOG_ERROR("Could not open file %s for writing", filename);
        return;
    }

    fprintf(file, "digraph DominanceFrontiers {\n");
    fprintf(file, "  node [shape=ellipse];\n");

    // Print nodes
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        fprintf(file, "  Block%zu [label=\"Block %zu\"];\n", block->id, block->id);
    }

    // Print dominance frontier edges
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        if (block->dom_frontier) {
            for (size_t j = 0; j < block->dom_frontier->count; j++) {
                BasicBlock *df_block = block->dom_frontier->blocks[j];
                if (df_block) { // Ensure the block is in the dominance frontier
                    fprintf(file, "  Block%zu -> Block%zu;\n", block->id, df_block->id);
                }
            }
        }
    }

    fprintf(file, "}\n");
    fclose(file);

    LOG_INFO("Dominance frontiers DOT file generated: %s", filename);
}

// Helper to check if a variable already has a phi in a block
static int has_phi_var(char **block_phi_vars, size_t block_phi_var_count, const char *var) {
    for (size_t _i = 0; _i < block_phi_var_count; _i++) {
        if (strcmp(block_phi_vars[_i], var) == 0) return 1;
    }
    return 0;
}

// Helper to add a variable to the phi set for a block
static void add_phi_var(char ***block_phi_vars, size_t *block_phi_var_count, size_t *block_phi_var_cap, const char *var) {
    if (*block_phi_var_count == *block_phi_var_cap) {
        size_t new_cap = *block_phi_var_cap ? *block_phi_var_cap * 2 : 4;
        *block_phi_vars = realloc(*block_phi_vars, new_cap * sizeof(char*));
        *block_phi_var_cap = new_cap;
    }
    (*block_phi_vars)[(*block_phi_var_count)++] = strdup(var);
}

// Insert φ-functions into the appropriate blocks based on dominance frontiers
void insert_phi_functions(CFG *cfg) {
    if (!cfg) return;

    LOG_INFO("Starting phi-function insertion");

    // Track which variables have phi functions in each block
    // We'll use a dynamic array of variable names per block
    char ***block_phi_vars = calloc(cfg->block_count, sizeof(char **));
    size_t *block_phi_var_counts = calloc(cfg->block_count, sizeof(size_t));
    size_t *block_phi_var_caps = calloc(cfg->block_count, sizeof(size_t));

    // Collect all variables assigned anywhere in the CFG
    // (This is a simple approach; for large CFGs, use a set)
    char **all_vars = NULL;
    size_t all_vars_count = 0, all_vars_cap = 0;
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            const char *var_name = NULL;
            if (stmt->type == NODE_VAR_DECL) {
                var_name = stmt->data.var_decl.name;
            } else if (stmt->type == NODE_ASSIGNMENT) {
                var_name = stmt->data.assignment.name;
            }
            if (var_name) {
                // Add to all_vars if not already present
                int found = 0;
                for (size_t k = 0; k < all_vars_count; k++) {
                    if (strcmp(all_vars[k], var_name) == 0) { found = 1; break; }
                }
                if (!found) {
                    if (all_vars_count == all_vars_cap) {
                        size_t new_cap = all_vars_cap ? all_vars_cap * 2 : 8;
                        all_vars = realloc(all_vars, new_cap * sizeof(char*));
                        all_vars_cap = new_cap;
                    }
                    all_vars[all_vars_count++] = strdup(var_name);
                }
            }
        }
    }

    // For each variable, insert phi functions in its dominance frontier blocks
    for (size_t v = 0; v < all_vars_count; v++) {
        const char *var = all_vars[v];
        // 1. Collect all blocks that assign to var
        int *assign_blocks = calloc(cfg->block_count, sizeof(int));
        for (size_t i = 0; i < cfg->block_count; i++) {
            BasicBlock *block = cfg->blocks[i];
            for (size_t j = 0; j < block->stmt_count; j++) {
                ASTNode *stmt = block->stmts[j];
                const char *var_name = NULL;
                if (stmt->type == NODE_VAR_DECL) {
                    var_name = stmt->data.var_decl.name;
                } else if (stmt->type == NODE_ASSIGNMENT) {
                    var_name = stmt->data.assignment.name;
                }
                if (var_name && strcmp(var_name, var) == 0) {
                    assign_blocks[i] = 1;
                    break;
                }
            }
        }
        // 2. Build a set of all blocks in the dominance frontier of any assign block (phi_blocks)
        int *phi_blocks = calloc(cfg->block_count, sizeof(int));
        for (size_t i = 0; i < cfg->block_count; i++) {
            if (!assign_blocks[i]) continue;
            BasicBlock *block = cfg->blocks[i];
            if (!block->dom_frontier) continue;
            for (size_t j = 0; j < block->dom_frontier->count; j++) {
                BasicBlock *df_block = block->dom_frontier->blocks[j];
                phi_blocks[df_block->id] = 1;
            }
        }
        // 3. Insert phi for var in each phi_block (only once)
        for (size_t i = 0; i < cfg->block_count; i++) {
            if (!phi_blocks[i]) continue;
            if (!has_phi_var(block_phi_vars[i], block_phi_var_counts[i], var)) {
                BasicBlock *df_block = cfg->blocks[i];
                ASTNode *phi_node = malloc(sizeof(ASTNode));
                phi_node->type = NODE_VAR_DECL; // Represent φ-function as a variable declaration
                phi_node->data.var_decl.name = strdup(var);
                phi_node->data.var_decl.type = NULL;
                phi_node->data.var_decl.init_value = NULL;
                if (df_block->stmt_count >= df_block->stmt_capacity) {
                    size_t new_capacity = df_block->stmt_capacity == 0 ? 8 : df_block->stmt_capacity * 2;
                    df_block->stmts = realloc(df_block->stmts, sizeof(ASTNode *) * new_capacity);
                    df_block->stmt_capacity = new_capacity;
                }
                // Insert phi at the start, but only once per variable per block
                for (size_t k = df_block->stmt_count; k > 0; k--) {
                    df_block->stmts[k] = df_block->stmts[k - 1];
                }
                df_block->stmts[0] = phi_node;
                df_block->stmt_count++;
                add_phi_var(&block_phi_vars[i], &block_phi_var_counts[i], &block_phi_var_caps[i], var);
                LOG_INFO("Inserted φ-function for variable %s in block %zu", var, (size_t)i);
            }
        }
        free(assign_blocks);
        free(phi_blocks);
    }

    // Free temp structures
    for (size_t i = 0; i < cfg->block_count; i++) {
        for (size_t j = 0; j < block_phi_var_counts[i]; j++) {
            free(block_phi_vars[i][j]);
        }
        free(block_phi_vars[i]);
    }
    free(block_phi_vars);
    free(block_phi_var_counts);
    free(block_phi_var_caps);
    for (size_t i = 0; i < all_vars_count; i++) free(all_vars[i]);
    free(all_vars);

    LOG_INFO("Phi-function insertion completed");
}