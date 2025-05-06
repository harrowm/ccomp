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

// Insert φ-functions into the appropriate blocks based on dominance frontiers
void insert_phi_functions(CFG *cfg) {
    if (!cfg) return;

    LOG_INFO("Starting phi-function insertion");

    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        LOG_INFO("Processing block %zu", block->id);

        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            const char *var_name = NULL;
            if (stmt->type == NODE_VAR_DECL) {
                var_name = stmt->data.var_decl.name;
                LOG_INFO("Variable declaration found: %s", var_name);
            } else if (stmt->type == NODE_ASSIGNMENT) {
                var_name = stmt->data.assignment.name;
                LOG_INFO("Assignment found: %s", var_name);
            }

            if (var_name) {
                LOG_INFO("Processing variable %s for phi-function insertion", var_name);
            }
        }
    }

    LOG_INFO("Phi-function insertion completed");

    // Iterate over all blocks in the CFG
    LOG_INFO("Inserting φ-functions into CFG, block count: %zu", cfg->block_count);
    bool *has_phi = calloc(cfg->block_count, sizeof(bool));
    bool *work = calloc(cfg->block_count, sizeof(bool));

    // Refine φ-function insertion logic to only insert in dominance frontiers
    LOG_INFO("Initializing worklist for φ-function propagation");
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
                LOG_INFO("Variable %s defined in block %zu", var_name, i);
                memset(work, 0, sizeof(bool) * cfg->block_count);
                work[i] = true;

                while (true) {
                    bool changed = false;
                    for (size_t k = 0; k < cfg->block_count; k++) {
                        if (!work[k]) continue;
                        work[k] = false;

                        BasicBlock *current_block = cfg->blocks[k];
                        LOG_INFO("Processing block %zu for φ-function propagation", k);

                        // Adjust the algorithm to consider blocks dominated by the current block
                        LOG_INFO("Processing blocks dominated by block %zu", k);
                        for (size_t l = 0; l < current_block->dominated_count; l++) {
                            size_t dominated_block_id = current_block->dominated[l]->id;
                            LOG_INFO("Checking dominated block %zu of block %zu", dominated_block_id, k);

                            // Ensure φ-functions are inserted only in blocks where control flow merges
                            BasicBlock *dominated_block = cfg->blocks[dominated_block_id];
                            for (size_t m = 0; m < dominated_block->pred_count; m++) {
                                if (dominated_block->preds[m] != current_block) {
                                    LOG_INFO("Block %zu has multiple predecessors, inserting φ-function for variable %s", dominated_block_id, var_name);

                                    if (!has_phi[dominated_block_id]) {
                                        has_phi[dominated_block_id] = true;
                                        work[dominated_block_id] = true; // Add to worklist for further processing
                                        changed = true; // Ensure the loop continues

                                        // Insert φ-function for the variable in dominated_block_id
                                        ASTNode *phi_node = malloc(sizeof(ASTNode));
                                        phi_node->type = NODE_VAR_DECL; // Represent φ-function as a variable declaration
                                        phi_node->data.var_decl.name = strdup(var_name);
                                        phi_node->data.var_decl.type = NULL; // Type can be resolved later
                                        phi_node->data.var_decl.init_value = NULL; // φ-functions have no initial value

                                        if (dominated_block->stmt_count >= dominated_block->stmt_capacity) {
                                            size_t new_capacity = dominated_block->stmt_capacity == 0 ? 8 : dominated_block->stmt_capacity * 2;
                                            dominated_block->stmts = realloc(dominated_block->stmts, sizeof(ASTNode *) * new_capacity);
                                            dominated_block->stmt_capacity = new_capacity;
                                        }
                                        // Insert φ-function as the first instruction in the block
                                        for (size_t i = dominated_block->stmt_count; i > 0; i--) {
                                            dominated_block->stmts[i] = dominated_block->stmts[i - 1];
                                        }
                                        dominated_block->stmts[0] = phi_node;
                                        dominated_block->stmt_count++;
                                        LOG_INFO("φ-function for variable %s successfully inserted in block %zu", var_name, dominated_block_id);
                                    } else {
                                        LOG_INFO("Block %zu already has a φ-function for variable %s, skipping.", dominated_block_id, var_name);
                                    }
                                    break;
                                }
                            }
                        }

                        for (size_t l = 0; l < current_block->dom_frontier->count; l++) {
                            size_t df_block_id = current_block->dom_frontier->blocks[l]->id;
                            LOG_INFO("Checking dominance frontier block %zu for block %zu", df_block_id, k);
                            if (!has_phi[df_block_id]) {
                                has_phi[df_block_id] = true;
                                work[df_block_id] = true;
                                changed = true;

                                LOG_INFO("Inserting φ-function for variable %s in block %zu", var_name, df_block_id);
                                BasicBlock *df_block = cfg->blocks[df_block_id];
                                ASTNode *phi_node = malloc(sizeof(ASTNode));
                                phi_node->type = NODE_VAR_DECL;
                                phi_node->data.var_decl.name = strdup(var_name);
                                phi_node->data.var_decl.type = NULL;
                                phi_node->data.var_decl.init_value = NULL;

                                if (df_block->stmt_count >= df_block->stmt_capacity) {
                                    size_t new_capacity = df_block->stmt_capacity == 0 ? 8 : df_block->stmt_capacity * 2;
                                    df_block->stmts = realloc(df_block->stmts, sizeof(ASTNode *) * new_capacity);
                                    df_block->stmt_capacity = new_capacity;
                                }
                                // Insert φ-function as the first instruction in the block
                                for (size_t i = df_block->stmt_count; i > 0; i--) {
                                    df_block->stmts[i] = df_block->stmts[i - 1];
                                }
                                df_block->stmts[0] = phi_node;
                                df_block->stmt_count++;
                                LOG_INFO("φ-function for variable %s successfully inserted in block %zu", var_name, df_block_id);
                            } else {
                                LOG_INFO("Block %zu already has a φ-function for variable %s, skipping", df_block_id, var_name);
                            }
                        }
                    }

                    if (!changed) {
                        LOG_INFO("No changes detected, exiting worklist processing loop for variable %s", var_name);
                        break;
                    }
                }
            }
        }
    }

    free(has_phi);
    free(work);
}