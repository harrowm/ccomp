/*
 * File: cfg.c
 * Description: Implements the Control Flow Graph (CFG) construction and manipulation.
 * Purpose: Used to represent and analyze the flow of control in a program.
 */

// cfg.c
#include "cfg.h"
#include "ast.h"
#include "debug.h"
#include "lexer.h" // For token_type_to_string()
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BasicBlock* create_basic_block(CFG *cfg, BlockType type) {
    BasicBlock *block = malloc(sizeof(BasicBlock));
    if (!block) {
        LOG_ERROR("Unable to allocate memory for basic block");
        return NULL;
    }
    
    block->id = cfg->block_count;
    block->type = type;
    block->stmts = NULL;
    block->stmt_count = 0;
    block->stmt_capacity = 0;
    
    block->preds = NULL;
    block->pred_count = 0;
    block->pred_capacity = 0;
    
    block->succs = NULL;
    block->succ_count = 0;
    block->succ_capacity = 0;
    
    block->dominator = NULL;
    block->dom_frontier = NULL;
    block->df_count = 0;
    block->df_capacity = 0;

    block->dominated = NULL;
    block->dominated_count = 0;
    block->dominated_capacity = 0;

    block->function_name = NULL; // Initialize function_name to NULL

    LOG_INFO("Created basic block %zu of type %d", block->id, type);
    
    // Add to CFG
    if (cfg->block_count >= cfg->block_capacity) {
        size_t new_capacity = cfg->block_capacity == 0 ? 8 : cfg->block_capacity * 2;
        BasicBlock **new_blocks = realloc(cfg->blocks, sizeof(BasicBlock*) * new_capacity);
        if (!new_blocks) {
            free(block);
            LOG_ERROR("Unable to allocate memory for CFG blocks");
            free(block);
            return NULL;
        }
        cfg->blocks = new_blocks;
        cfg->block_capacity = new_capacity;
        LOG_INFO("Resized CFG blocks array to %zu", new_capacity);
    }
    
    cfg->blocks[cfg->block_count++] = block;
    
    return block;
}

static void add_statement(BasicBlock *block, ASTNode *stmt) {
    if (block->stmt_count >= block->stmt_capacity) {
        size_t new_capacity = block->stmt_capacity == 0 ? 8 : block->stmt_capacity * 2;
        ASTNode **new_stmts = realloc(block->stmts, sizeof(ASTNode*) * new_capacity);
        if (!new_stmts) return;
        block->stmts = new_stmts;
        block->stmt_capacity = new_capacity;
        LOG_INFO("Resized basic block %zu statements array to %zu", block->id, new_capacity);
    }
    
    block->stmts[block->stmt_count++] = stmt;
    LOG_INFO("Added statement number %zu to block %zu: %s", (block->stmt_count-1), block->id, node_type_to_string(stmt->type));
}

static void add_predecessor(BasicBlock *block, BasicBlock *pred) {
    if (block->pred_count >= block->pred_capacity) {
        size_t new_capacity = block->pred_capacity == 0 ? 4 : block->pred_capacity * 2;
        BasicBlock **new_preds = realloc(block->preds, sizeof(BasicBlock*) * new_capacity);
        if (!new_preds) return;
        block->preds = new_preds;
        block->pred_capacity = new_capacity;
        LOG_INFO("Resized basic block %zu predecessors array to %zu", block->id, new_capacity);
    }
    
    block->preds[block->pred_count++] = pred;
    LOG_INFO("Added predecessor to block %zu: %zu", block->id, pred->id);
}

static void add_successor(BasicBlock *block, BasicBlock *succ) {
    if (block->succ_count >= block->succ_capacity) {
        size_t new_capacity = block->succ_capacity == 0 ? 4 : block->succ_capacity * 2;
        BasicBlock **new_succs = realloc(block->succs, sizeof(BasicBlock*) * new_capacity);
        if (!new_succs) return;
        block->succs = new_succs;
        block->succ_capacity = new_capacity;
        LOG_INFO("Resized basic block %zu successors array to %zu", block->id, new_capacity);
    }
    
    block->succs[block->succ_count++] = succ;
    add_predecessor(succ, block);
    LOG_INFO("Added successor to block %zu: %zu", block->id, succ->id);
}

static void process_statement(CFG *cfg, BasicBlock **current_block, ASTNode *stmt) {
    LOG_INFO("Processing statement: %s", node_type_to_string(stmt->type));
    LOG_INFO("AST Traversal: Node type: %s", node_type_to_string(stmt->type));

    switch (stmt->type) {
        case NODE_BINARY_OP:
            if (!stmt->data.binary_op.left || !stmt->data.binary_op.right) {
                LOG_ERROR("Binary operation has NULL operands: left=%p, right=%p", stmt->data.binary_op.left, stmt->data.binary_op.right);
                return;
            }
            add_statement(*current_block, stmt);
            break;

        case NODE_LITERAL:
        case NODE_VAR_REF:
        case NODE_VAR_DECL:
        case NODE_ASSIGNMENT:
        case NODE_RETURN:
            add_statement(*current_block, stmt);
            break;

        case NODE_UNARY_OP:
            LOG_INFO("Processing unary operation node");
            add_statement(*current_block, stmt);
            break;

        case NODE_FUNCTION_CALL:
            LOG_INFO("Processing function call: %s", stmt->data.function_call.name);
            add_statement(*current_block, stmt);
            break;
            
        case NODE_IF_STMT: {
            // Create blocks for if-then-else structure
            BasicBlock *then_block = create_basic_block(cfg, BLOCK_IF_THEN);
            BasicBlock *else_block = create_basic_block(cfg, BLOCK_IF_ELSE);
            BasicBlock *merge_block = create_basic_block(cfg, BLOCK_NORMAL);
            
            // Add conditional branch
            add_statement(*current_block, stmt->data.if_stmt.condition);
            add_successor(*current_block, then_block);
            add_successor(*current_block, else_block);
            
            // Process then block
            *current_block = then_block;
            process_statement(cfg, current_block, stmt->data.if_stmt.then_branch);
            add_successor(*current_block, merge_block);
            
            // Process else block if exists
            if (stmt->data.if_stmt.else_branch) {
                *current_block = else_block;
                process_statement(cfg, current_block, stmt->data.if_stmt.else_branch);
                add_successor(*current_block, merge_block);
            } else {
                add_successor(else_block, merge_block);
            }
            
            // Continue with merge block
            *current_block = merge_block;
            break;
        }
            
        case NODE_WHILE_STMT: {
            BasicBlock *header = create_basic_block(cfg, BLOCK_LOOP_HEADER);
            BasicBlock *body = create_basic_block(cfg, BLOCK_LOOP_BODY);
            BasicBlock *exit = create_basic_block(cfg, BLOCK_NORMAL);

            // Current block flows to loop header
            add_successor(*current_block, header);

            // Header has two successors: body and exit
            add_statement(header, stmt->data.while_stmt.condition);
            add_successor(header, body);
            add_successor(header, exit);

            // Process body
            *current_block = body;
            process_statement(cfg, current_block, stmt->data.while_stmt.body);
            add_successor(*current_block, header); // Loop back

            // Continue with exit block
            *current_block = exit;
            break;
        }

        case NODE_FOR_STMT: {
            BasicBlock *header = create_basic_block(cfg, BLOCK_LOOP_HEADER);
            BasicBlock *body = create_basic_block(cfg, BLOCK_LOOP_BODY);
            BasicBlock *exit = create_basic_block(cfg, BLOCK_NORMAL);

            // Process initializer if exists
            if (stmt->data.for_stmt.init) {
                process_statement(cfg, current_block, stmt->data.for_stmt.init);
            }

            // Current block flows to loop header
            add_successor(*current_block, header);

            // Header has two successors: body and exit
            if (stmt->data.for_stmt.condition) {
                add_statement(header, stmt->data.for_stmt.condition);
            }
            add_successor(header, body);
            add_successor(header, exit);

            // Process body
            *current_block = body;
            process_statement(cfg, current_block, stmt->data.for_stmt.body);

            // Process update if exists
            if (stmt->data.for_stmt.update) {
                process_statement(cfg, current_block, stmt->data.for_stmt.update);
            }

            // Loop back to header
            add_successor(*current_block, header);

            // Continue with exit block
            *current_block = exit;
            break;
        }
            
        case NODE_STMT_LIST:
            for (size_t i = 0; i < stmt->data.stmt_list.count; i++) {
                process_statement(cfg, current_block, stmt->data.stmt_list.stmts[i]);
            }
            break;
            
        default:
            LOG_ERROR("Unhandled statement type in CFG construction: %s", 
                     node_type_to_string(stmt->type));
            break;
    }
}

CFG* ast_to_cfg(ASTNode *ast) {
    if (!ast || ast->type != NODE_PROGRAM) {
        LOG_ERROR("Invalid AST root node for CFG construction: type=%s", ast ? node_type_to_string(ast->type) : "NULL");
        return NULL;
    }

    CFG *cfg = malloc(sizeof(CFG));
    if (!cfg) {
        LOG_ERROR("Unable to allocate memory for CFG");
        return NULL;
    }

    cfg->blocks = NULL;
    cfg->block_count = 0;
    cfg->block_capacity = 0;

    // Create entry and exit blocks
    cfg->entry = create_basic_block(cfg, BLOCK_ENTRY);
    cfg->exit = create_basic_block(cfg, BLOCK_EXIT);
    LOG_INFO("Created entry and exit blocks");

    // Process all functions in the program
    for (size_t i = 0; i < ast->data.stmt_list.count; i++) {
        ASTNode *func = ast->data.stmt_list.stmts[i];
        if (func->type != NODE_FUNCTION_DECL) continue;

        // Create a new block for the function body
        BasicBlock *func_block = create_basic_block(cfg, BLOCK_NORMAL);
        if (func->data.function_decl.name) {
            func_block->function_name = strdup(func->data.function_decl.name);
        }
        add_successor(cfg->entry, func_block);

        // Process function body
        BasicBlock *current_block = func_block;
        process_statement(cfg, &current_block, func->data.function_decl.body);

        // Connect the last block of the function to the exit block
        if (current_block != cfg->exit) {
            add_successor(current_block, cfg->exit);
        }
    }

    return cfg;
}

// Function to compute the dominator tree for the CFG
void compute_dominator_tree(CFG *cfg) {
    if (!cfg || !cfg->entry) {
        LOG_ERROR("Invalid CFG or entry block");
        return;
    }

    // Initialize dominators
    for (size_t i = 0; i < cfg->block_count; i++) {
        cfg->blocks[i]->dominator = NULL;
    }
    cfg->entry->dominator = cfg->entry; // Entry block dominates itself

    bool changed;
    do {
        changed = false;

        for (size_t i = 0; i < cfg->block_count; i++) {
            BasicBlock *block = cfg->blocks[i];
            if (block == cfg->entry) continue; // Skip the entry block

            BasicBlock *new_idom = NULL;
            for (size_t j = 0; j < block->pred_count; j++) {
                BasicBlock *pred = block->preds[j];
                if (pred->dominator) {
                    if (!new_idom) {
                        new_idom = pred;
                    } else {
                        // Intersect dominators
                        BasicBlock *finger1 = pred;
                        BasicBlock *finger2 = new_idom;
                        while (finger1 != finger2) {
                            while (finger1 && finger1->id > finger2->id) {
                                finger1 = finger1->dominator;
                            }
                            while (finger2 && finger2->id > finger1->id) {
                                finger2 = finger2->dominator;
                            }
                        }
                        new_idom = finger1;
                    }
                }
            }

            if (block->dominator != new_idom) {
                block->dominator = new_idom;
                changed = true;
            }
        }
    } while (changed);

    LOG_INFO("Dominator tree computed successfully");

    // Initialize the dominated array for each block
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        block->dominated = NULL;
        block->dominated_count = 0;
        block->dominated_capacity = 0;
    }

    // Populate the dominated array based on dominator relationships
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        if (block->dominator) {
            BasicBlock *dominator = block->dominator;

            // Ensure the dominator's dominated array has enough capacity
            if (dominator->dominated_count >= dominator->dominated_capacity) {
                size_t new_capacity = dominator->dominated_capacity == 0 ? 4 : dominator->dominated_capacity * 2;
                dominator->dominated = realloc(dominator->dominated, sizeof(BasicBlock *) * new_capacity);
                dominator->dominated_capacity = new_capacity;
            }

            // Add the current block to the dominator's dominated array
            dominator->dominated[dominator->dominated_count++] = block;
        }
    }
}

void free_cfg(CFG *cfg) {
    if (!cfg) {
        LOG_ERROR("NULL CFG pointer");
        return;
    } 
    
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        if (block) {
            free(block->stmts);
            free(block->preds);
            free(block->succs);
            free(block->dom_frontier);
            free(block->dominated);
            free(block);
        }
    }
    
    free(cfg->blocks);
    free(cfg);
}

const char* block_type_to_string(BlockType type) {
    switch (type) {
        case BLOCK_NORMAL: return "Normal Block";
        case BLOCK_ENTRY: return "Entry Block";
        case BLOCK_EXIT: return "Exit Block";
        case BLOCK_IF_THEN: return "If-Then Block";
        case BLOCK_IF_ELSE: return "If-Else Block";
        case BLOCK_LOOP_HEADER: return "Loop Header Block";
        case BLOCK_LOOP_BODY: return "Loop Body Block";
        default: return "Unknown Block Type";
    }
}

// Helper function to print newline and indentation
static void newline_indent(int spaces, FILE *f) {
    fprintf(f, "\n");
    for (int i = 0; i < spaces; i++) fprintf(f, " ");
}

// Function previously named print_ast_to_stream, rewritten for specific CFG formatting
void print_ast_node_for_cfg(ASTNode *node, int current_indent_spaces, FILE *stream) {
    if (!node || !stream) return;

    switch (node->type) {
        // These node types are not typically expected in a CFG block statement list
        case NODE_PROGRAM: 
        case NODE_FUNCTION_DECL: 
        case NODE_PARAM_LIST: 
        case NODE_STMT_LIST: 
        case NODE_IF_STMT: 
        case NODE_WHILE_STMT: 
        case NODE_FOR_STMT: 
        case NODE_TYPE_SPECIFIER:
            LOG_ERROR("Unexpected node type in CFG: %s", node_type_to_string(node->type));
            break;

        case NODE_VAR_DECL:
            fprintf(stream, "VarDecl: %s", node->data.var_decl.name);
            if (node->data.var_decl.init_value) {
                newline_indent(current_indent_spaces, stream); // Same indent for Initializer label
                fprintf(stream, "Initializer:");
                newline_indent(current_indent_spaces + 2, stream); // Indent value +2
                print_ast_node_for_cfg(node->data.var_decl.init_value, current_indent_spaces + 2, stream);
            } else {
                fprintf(stream, "\n"); // End line if no initializer
            }
            break;

        case NODE_ASSIGNMENT:
            fprintf(stream, "Assignment: %s", node->data.assignment.name);
            newline_indent(current_indent_spaces, stream); // Same indent for value
            print_ast_node_for_cfg(node->data.assignment.value, current_indent_spaces, stream);
            break;

        case NODE_BINARY_OP:
            fprintf(stream, "BinaryOp: %s", token_type_to_string(node->data.binary_op.op));
            newline_indent(current_indent_spaces, stream); // Same indent for Left label
            fprintf(stream, "Left:");
            newline_indent(current_indent_spaces + 2, stream); // Indent left operand +2
            print_ast_node_for_cfg(node->data.binary_op.left, current_indent_spaces + 2, stream);
            newline_indent(current_indent_spaces, stream); // Same indent for Right label
            fprintf(stream, "Right:");
            newline_indent(current_indent_spaces + 2, stream); // Indent right operand +2
            print_ast_node_for_cfg(node->data.binary_op.right, current_indent_spaces + 2, stream);
            break;

        case NODE_UNARY_OP:
             fprintf(stream, "UnaryOp: %s (%s)", token_type_to_string(node->data.unary_op.op),
                     node->data.unary_op.is_prefix ? "prefix" : "postfix");
             newline_indent(current_indent_spaces + 2, stream); // Indent operand +2
             print_ast_node_for_cfg(node->data.unary_op.operand, current_indent_spaces + 2, stream);
             break;

        case NODE_RETURN:
            fprintf(stream, "Return");
            if (node->data.return_stmt.value) {
                newline_indent(current_indent_spaces, stream); // Same indent for return value
                print_ast_node_for_cfg(node->data.return_stmt.value, current_indent_spaces, stream);
            } else {
                 fprintf(stream, "\n"); // End line if no value
            }
            break;

        case NODE_LITERAL:
            if (node->data.literal.type && node->data.literal.type->kind == TYPE_POINTER) {
                fprintf(stream, "Literal (pointer): %p\n", node->data.literal.value.ptr_value);
            } else {
                // Assuming int for simplicity based on expected output
                fprintf(stream, "Literal: %d\n", node->data.literal.value.int_value);
            }
            break;

        case NODE_VAR_REF:
            fprintf(stream, "VarRef: %s\n", node->data.var_ref.name);
            break;

        case NODE_FUNCTION_CALL:
            fprintf(stream, "FunctionCall: %s", node->data.function_call.name);
            if (node->data.function_call.arg_count > 0) {
                 newline_indent(current_indent_spaces, stream); // Same indent for Args label
                 fprintf(stream, "Arguments:");
                 for (size_t i = 0; i < node->data.function_call.arg_count; i++) {
                     newline_indent(current_indent_spaces + 2, stream); // Indent args +2
                     print_ast_node_for_cfg(node->data.function_call.args[i], current_indent_spaces + 2, stream);
                 }
            } else {
                 fprintf(stream, "\n"); // End line if no args
            }
            break;

        default:
            fprintf(stream, "Unknown node type (%d)\n", node->type);
            break;
    }
}

void print_cfg(CFG *cfg, FILE *stream) {
    if (!cfg || !stream) return;

    fprintf(stream, "Control Flow Graph:\n");
    fprintf(stream, "==================\n");

    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        fprintf(stream, "Block %zu (%s):\n", block->id, block_type_to_string(block->type));

        fprintf(stream, "  Statements: count %zu capacity %zu\n", block->stmt_count, block->stmt_capacity);
        if (block->stmt_count == 0) {
            fprintf(stream, "    (none)\n");
        } else {
            for (size_t j = 0; j < block->stmt_count; j++) {
                fprintf(stream, "    - "); // 6 characters prefix
                // Call the new function with the starting indentation level (6 spaces)
                print_ast_node_for_cfg(block->stmts[j], 6, stream);
            }
        }

        fprintf(stream, "  Predecessors: ");
        if (block->pred_count == 0) {
            fprintf(stream, "(none)");
        } else {
            for (size_t j = 0; j < block->pred_count; j++) {
                fprintf(stream, "%zu%s", block->preds[j]->id, j < block->pred_count - 1 ? ", " : "");
            }
        }
        fprintf(stream, "\n");

        fprintf(stream, "  Successors: ");
        if (block->succ_count == 0) {
            fprintf(stream, "(none)");
        } else {
            for (size_t j = 0; j < block->succ_count; j++) {
                fprintf(stream, "%zu%s", block->succs[j]->id, j < block->succ_count - 1 ? ", " : "");
            }
        }
        fprintf(stream, "\n\n"); // Add an extra newline for spacing between blocks
    }
}

void generate_dot_file(CFG *cfg, const char *filename) {
    if (!cfg || !filename) return;

    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return;
    }

    fprintf(file, "digraph CFG {\n");
    fprintf(file, "  node [shape=box];\n");

    // Print nodes with statements
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        fprintf(file, "  Block%zu [label=\"Block %zu\\nType: %s", block->id, block->id, block_type_to_string(block->type));

        // Add statements to the label
        for (size_t j = 0; j < block->stmt_count; j++) {
            fprintf(file, "\\nStmt %zu: ", j);
            char stmt_buffer[256] = {0};
            FILE *stmt_stream = fmemopen(stmt_buffer, sizeof(stmt_buffer), "w");
            if (stmt_stream) {
                print_ast_node_for_cfg(block->stmts[j], 0, stmt_stream);
                fclose(stmt_stream);
            }
            fprintf(file, "%s", stmt_buffer);
        }

        fprintf(file, "\"];\n");
    }

    // Print edges
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        for (size_t j = 0; j < block->succ_count; j++) {
            fprintf(file, "  Block%zu -> Block%zu;\n", block->id, block->succs[j]->id);
        }
    }

    fprintf(file, "}\n");
    fclose(file);

    printf("DOT file generated: %s\n", filename);
}