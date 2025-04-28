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