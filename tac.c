#include "tac.h"
#include "lexer.h" // Include token definitions like TOK_PLUS
#include "debug.h" // Include debug.h for logging macros
#include "parser.h" // For node_type_to_string
#include "cfg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Linear hash table for variable stacks
#define HASH_TABLE_SIZE 1024
typedef struct VarStack {
    char *name;
    int *versions;
    size_t size;
    size_t capacity;
    struct VarStack *next;
} VarStack;

static VarStack *hash_table[HASH_TABLE_SIZE];

static size_t hash(const char *str) {
    size_t hash = 0;
    while (*str) {
        hash = (hash * 31) + *str++;
    }
    return hash % HASH_TABLE_SIZE;
}

static VarStack *get_var_stack(const char *name) {
    size_t index = hash(name);
    VarStack *entry = hash_table[index];
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static void push_var_version(const char *name, int version) {
    size_t index = hash(name);
    VarStack *entry = hash_table[index];
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            if (entry->size == entry->capacity) {
                entry->capacity *= 2;
                entry->versions = realloc(entry->versions, entry->capacity * sizeof(int));
            }
            entry->versions[entry->size++] = version;
            return;
        }
        entry = entry->next;
    }

    // Create a new entry if not found
    entry = malloc(sizeof(VarStack));
    entry->name = strdup(name);
    entry->capacity = 4;
    entry->size = 1;
    entry->versions = malloc(entry->capacity * sizeof(int));
    entry->versions[0] = version;
    entry->next = hash_table[index];
    hash_table[index] = entry;
}

static int pop_var_version(const char *name) {
    VarStack *entry = get_var_stack(name);
    if (entry && entry->size > 0) {
        return entry->versions[--entry->size];
    }
    fprintf(stderr, "Error: Popping from empty stack for variable %s\n", name);
    exit(EXIT_FAILURE);
}

static int peek_var_version(const char *name) {
    VarStack *entry = get_var_stack(name);
    if (entry && entry->size > 0) {
        return entry->versions[entry->size - 1];
    }
    fprintf(stderr, "Error: Peeking from empty stack for variable %s\n", name);
    exit(EXIT_FAILURE);
}

// Helper function to create a new TAC instruction
static int label_counter = 0; // Global label counter

// Updated create_tac to use integer labels
static TAC *create_tac(TACType type, const char *result, const char *arg1, const char *arg2, const char *op, int *label) {
    TAC *tac = malloc(sizeof(TAC));
    if (!tac) {
        fprintf(stderr, "Error: Unable to allocate memory for TAC instruction\n");
        exit(EXIT_FAILURE);
    }
    tac->type = type;
    tac->result = result ? strdup(result) : NULL;
    tac->arg1 = arg1 ? strdup(arg1) : NULL;
    tac->arg2 = arg2 ? strdup(arg2) : NULL;
    tac->op = op ? strdup(op) : NULL;
    tac->label = label ? malloc(sizeof(int)) : NULL;
    if (label) {
        *(tac->label) = *label;
    }
    tac->int_value = 0; // Default to 0
    tac->ptr_value = NULL; // Default to NULL
    tac->next = NULL;
    return tac;
}

// Updated helper function to convert token types to string
static const char *operator_to_string(int op) {
    switch (op) {
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_AMP_AMP: return "&&";
        case TOK_PIPE_PIPE: return "||";
        case TOK_PLUS_EQ: return "+=";
        case TOK_MINUS_EQ: return "-=";
        case TOK_STAR_EQ: return "*=";
        case TOK_SLASH_EQ: return "/=";
        case TOK_PLUS_PLUS: return "++";
        case TOK_MINUS_MINUS: return "--";
        case TOK_GT: return ">";
        case TOK_LT: return "<";
        case TOK_LT_EQ: return "<=";
        case TOK_GT_EQ: return ">=";
        case TOK_EQ_EQ: return "==";
        case TOK_BANG_EQ: return "!=";
        case TOK_AMP: return "&";
        case TOK_PIPE: return "|";
        case TOK_CARET: return "^";
        case TOK_AMP_EQ: return "&=";
        case TOK_PIPE_EQ: return "|=";
        case TOK_CARET_EQ: return "^=";
        case TOK_UNKNOWN: 
        default: return "<unknown>";
    }
}

// Helper function to convert NodeType to string
static const char *cfg_node_type_to_string(NodeType type) {
    return node_type_to_string(type);
}

// Global variable to track the last temporary variable
static char last_temp_var[32] = "";

// Update the last_temp_var whenever a new temporary variable is created
#define UPDATE_LAST_TEMP_VAR(temp) snprintf(last_temp_var, sizeof(last_temp_var), "%s", temp)

// Define the hash table for block labels
#define BLOCK_LABEL_TABLE_SIZE 1024

typedef struct BlockLabelEntry {
    size_t block_id;
    int label;
    struct BlockLabelEntry *next;
} BlockLabelEntry;

static BlockLabelEntry *block_label_table[BLOCK_LABEL_TABLE_SIZE];

static size_t hash_block_id(size_t block_id) {
    return block_id % BLOCK_LABEL_TABLE_SIZE;
}

static void insert_block_label(size_t block_id, int label) {
    size_t index = hash_block_id(block_id);
    BlockLabelEntry *entry = malloc(sizeof(BlockLabelEntry));
    if (!entry) {
        fprintf(stderr, "Error: Unable to allocate memory for block label entry\n");
        exit(EXIT_FAILURE);
    }
    entry->block_id = block_id;
    entry->label = label;
    entry->next = block_label_table[index];
    block_label_table[index] = entry;
}

static int get_block_label(size_t block_id) {
    size_t index = hash_block_id(block_id);
    BlockLabelEntry *entry = block_label_table[index];
    while (entry) {
        if (entry->block_id == block_id) {
            return entry->label;
        }
        entry = entry->next;
    }
    fprintf(stderr, "Error: Block ID %zu not found in label table\n", block_id);
    exit(EXIT_FAILURE);
}

static void free_block_label_table() {
    for (size_t i = 0; i < BLOCK_LABEL_TABLE_SIZE; i++) {
        BlockLabelEntry *entry = block_label_table[i];
        while (entry) {
            BlockLabelEntry *next = entry->next;
            free(entry);
            entry = next;
        }
        block_label_table[i] = NULL;
    }
}

// Preassign labels to all blocks in the CFG
static void preassign_block_labels(CFG *cfg) {
    for (size_t i = 0; i < cfg->block_count; i++) {
        int label = label_counter++;
        insert_block_label(cfg->blocks[i]->id, label);
    }
}

// Helper function to extract node information
typedef struct {
    enum { NODE_TYPE_LITERAL, NODE_TYPE_VAR_REF, NODE_TYPE_TEMP_VAR } type;
    union {
        char string_value[32]; // Unified string representation for literals and variable names
    } data;
} NodeValue;

// Updated to use the unified string_value field
static NodeValue extract_node_value(ASTNode *node) {
    NodeValue value;
    if (node->type == NODE_LITERAL) {
        value.type = NODE_TYPE_LITERAL;
        snprintf(value.data.string_value, sizeof(value.data.string_value), "%d", node->data.literal.value.int_value);
    } else if (node->type == NODE_VAR_REF) {
        value.type = NODE_TYPE_VAR_REF;
        strncpy(value.data.string_value, node->data.var_ref.name, sizeof(value.data.string_value));
    } else if (node->type == NODE_BINARY_OP) {
        LOG_ERROR("Binary operations are not directly supported in extract_node_value. Use a higher-level handler.");
        exit(EXIT_FAILURE);
    } else if (node->type == NODE_UNARY_OP) {
        LOG_ERROR("Unary operations are not directly supported in extract_node_value. Use a higher-level handler.");
        exit(EXIT_FAILURE);
    } else if (node->type == NODE_FUNCTION_CALL) {
        LOG_ERROR("Function calls are not directly supported in extract_node_value. Use a higher-level handler.");
        exit(EXIT_FAILURE);
    } else if (node->temp_var) {
        value.type = NODE_TYPE_TEMP_VAR;
        strncpy(value.data.string_value, node->temp_var, sizeof(value.data.string_value));
    } else {
        LOG_ERROR("Unsupported node type: %s", node_type_to_string(node->type));
        exit(EXIT_FAILURE);
    }
    return value;
}

static char *extract_node_value_as_string(ASTNode *node) {
    char *result = malloc(32); // Allocate memory for the string
    if (!result) {
        LOG_ERROR("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    if (node->type == NODE_LITERAL) {
        snprintf(result, 32, "%d", node->data.literal.value.int_value);
    } else if (node->type == NODE_VAR_REF) {
        strncpy(result, node->data.var_ref.name, 32);
    } else if (node->type == NODE_BINARY_OP) {
        if (node->temp_var) {
            strncpy(result, node->temp_var, 32);
        } else {
            LOG_ERROR("Binary operation node reached without temp_var set");
            // free(result);
            // exit(EXIT_FAILURE);
        }
    } else {
        LOG_ERROR("Unsupported node type: %s", node_type_to_string(node->type));
        free(result);
        exit(EXIT_FAILURE);
    }

    return result;
}

// Helper function to generate unique variable names
static void generate_unique_var_name(char *buffer, size_t buffer_size, const char *prefix) {
    static int unique_counter = 0; // Static counter to ensure unique names
    snprintf(buffer, buffer_size, "%s%d", prefix, unique_counter++);
}

// Updated process_statement to include binary operation handling
static void process_statement(ASTNode *stmt, BasicBlock *block, size_t stmt_index, TAC **tail) {
    LOG_INFO("Processing statement %zu in block %zu of type %s", stmt_index, block->id, cfg_node_type_to_string(stmt->type));
    TAC *new_tac = NULL;

    switch (stmt->type) {
        case NODE_VAR_DECL:
            LOG_INFO("Variable declaration: %s", stmt->data.var_decl.name);
            if (stmt->data.var_decl.init_value == NULL) {
                LOG_INFO("Detected φ-function for variable: %s", stmt->data.var_decl.name);
                new_tac = create_tac(TAC_PHI, stmt->data.var_decl.name, NULL, NULL, NULL, NULL);
            } else {
                NodeValue init_value = extract_node_value(stmt->data.var_decl.init_value);
                if (init_value.type == NODE_TYPE_LITERAL) {
                    new_tac = create_tac(TAC_ASSIGN, stmt->data.var_decl.name, init_value.data.string_value, NULL, NULL, NULL);
                } else if (init_value.type == NODE_TYPE_VAR_REF) {
                    new_tac = create_tac(TAC_ASSIGN, stmt->data.var_decl.name, init_value.data.string_value, NULL, NULL, NULL);
                } else {
                    LOG_ERROR("Unsupported initializer type for variable declaration");
                }
            }

            if (new_tac) {
                (*tail)->next = new_tac;
                *tail = new_tac;
                LOG_INFO("Linked TAC for variable declaration: %s", stmt->data.var_decl.name);
            }
            break;

        case NODE_ASSIGNMENT:
            LOG_INFO("Assignment: %s", stmt->data.assignment.name);
            if (stmt->data.assignment.value) {
                if (stmt->data.assignment.value->type == NODE_BINARY_OP) {
                    char temp_var[32];
                    generate_unique_var_name(temp_var, sizeof(temp_var), "t");
                    stmt->temp_var = strdup(temp_var); // Store temp variable in ASTNode

                    char *left_operand = extract_node_value_as_string(stmt->data.assignment.value->data.binary_op.left);
                    char *right_operand = extract_node_value_as_string(stmt->data.assignment.value->data.binary_op.right);

                    const char *op = operator_to_string(stmt->data.assignment.value->data.binary_op.op);
                    TAC *binary_tac = create_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);

                    (*tail)->next = binary_tac;
                    *tail = binary_tac;

                    free(left_operand);
                    free(right_operand);

                    new_tac = create_tac(TAC_ASSIGN, stmt->data.assignment.name, temp_var, NULL, NULL, NULL);
                } else {
                    NodeValue assign_value = extract_node_value(stmt->data.assignment.value);
                    if (assign_value.type == NODE_TYPE_LITERAL) {
                        new_tac = create_tac(TAC_ASSIGN, stmt->data.assignment.name, NULL, NULL, NULL, NULL);
                        new_tac->int_value = atoi(assign_value.data.string_value);
                    } else if (assign_value.type == NODE_TYPE_VAR_REF) {
                        new_tac = create_tac(TAC_ASSIGN, stmt->data.assignment.name, assign_value.data.string_value, NULL, NULL, NULL);
                    } else {
                        LOG_ERROR("Unsupported value type for assignment");
                    }
                }
            }
            break;

        case NODE_BINARY_OP:
            LOG_INFO("Binary operation");
            if (stmt->data.binary_op.left && stmt->data.binary_op.right) {
                LOG_INFO("Processing left operand of BinaryOp");
                process_statement(stmt->data.binary_op.left, block, stmt_index, tail);
                LOG_INFO("Left operand temp_var after processing: %s", stmt->data.binary_op.left->temp_var);

                LOG_INFO("Processing right operand of BinaryOp");
                process_statement(stmt->data.binary_op.right, block, stmt_index, tail);
                LOG_INFO("Right operand temp_var after processing: %s", stmt->data.binary_op.right->temp_var);

                char temp_var[32];
                generate_unique_var_name(temp_var, sizeof(temp_var), "t");
                stmt->temp_var = strdup(temp_var); // Store temp variable in ASTNode

                LOG_INFO("Generated temp_var for BinaryOp: %s", temp_var);

                char *left_operand = extract_node_value_as_string(stmt->data.binary_op.left);
                char *right_operand = extract_node_value_as_string(stmt->data.binary_op.right);

                const char *op = operator_to_string(stmt->data.binary_op.op);
                new_tac = create_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);

                (*tail)->next = new_tac;
                *tail = new_tac;

                LOG_INFO("Generated TAC for binary operation: %s = %s %s %s", temp_var, left_operand, op, right_operand);

                free(left_operand);
                free(right_operand);
            } else {
                LOG_ERROR("Binary operation has NULL operands");
            }
            break;

        case NODE_RETURN:
            LOG_INFO("Return statement");
            if (stmt->data.return_stmt.value) {
                if (stmt->data.return_stmt.value->type == NODE_BINARY_OP) {
                    LOG_INFO("Processing BinaryOp in return statement");

                    // Recursive processing for left operand
                    LOG_INFO("Processing left operand of BinaryOp in return");
                    process_statement(stmt->data.return_stmt.value->data.binary_op.left, block, stmt_index, tail);
                    LOG_INFO("Left operand temp_var after processing: %s", stmt->data.return_stmt.value->data.binary_op.left->temp_var);

                    // Recursive processing for right operand
                    LOG_INFO("Processing right operand of BinaryOp in return");
                    process_statement(stmt->data.return_stmt.value->data.binary_op.right, block, stmt_index, tail);
                    LOG_INFO("Right operand temp_var after processing: %s", stmt->data.return_stmt.value->data.binary_op.right->temp_var);

                    char temp_var[32];
                    generate_unique_var_name(temp_var, sizeof(temp_var), "t");
                    stmt->data.return_stmt.value->temp_var = strdup(temp_var); // Store temp variable in ASTNode

                    LOG_INFO("Generated temp_var for BinaryOp in return: %s", temp_var);

                    char *left_operand = extract_node_value_as_string(stmt->data.return_stmt.value->data.binary_op.left);
                    char *right_operand = extract_node_value_as_string(stmt->data.return_stmt.value->data.binary_op.right);

                    const char *op = operator_to_string(stmt->data.return_stmt.value->data.binary_op.op);
                    TAC *binary_tac = create_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);

                    (*tail)->next = binary_tac;
                    *tail = binary_tac;

                    free(left_operand);
                    free(right_operand);

                    LOG_INFO("Generated TAC for BinaryOp in return: %s = %s %s %s", temp_var, left_operand, op, right_operand);

                    new_tac = create_tac(TAC_RETURN, temp_var, NULL, NULL, NULL, NULL);
                } else {
                    NodeValue return_value = extract_node_value(stmt->data.return_stmt.value);
                    if (return_value.type == NODE_TYPE_LITERAL) {
                        new_tac = create_tac(TAC_RETURN, NULL, NULL, NULL, NULL, NULL);
                        new_tac->int_value = atoi(return_value.data.string_value);
                    } else if (return_value.type == NODE_TYPE_VAR_REF) {
                        new_tac = create_tac(TAC_RETURN, return_value.data.string_value, NULL, NULL, NULL, NULL);
                    } else {
                        LOG_ERROR("Unsupported return value type");
                    }
                }
            }
            break;

        // dont need to process these 
        case NODE_LITERAL:
        case NODE_VAR_REF:
            break;

        default:
            LOG_INFO("Unsupported AST node type in CFG to TAC conversion: %d - %s", stmt->type, cfg_node_type_to_string(stmt->type));
            break;
    }

    if (new_tac) {
        LOG_INFO("Generated TAC: type=%d, result=%s, arg1=%s, arg2=%s, op=%s, label=%s", 
                 new_tac->type, new_tac->result, new_tac->arg1, new_tac->arg2, new_tac->op, new_tac->label);
        (*tail)->next = new_tac;
        *tail = new_tac;
    }
}

// Updated handling for CFG block types in TAC generation
static void traverse_cfg(BasicBlock *block, bool *visited, TAC **head, TAC **tail) {
    if (visited[block->id]) {
        return; // Block already visited
    }
    visited[block->id] = true;

    // Retrieve the preassigned label for the block
    int label = get_block_label(block->id);
    LOG_INFO("Adding block label: L%d", label);
    TAC *label_tac = create_tac(TAC_LABEL, NULL, NULL, NULL, NULL, &label);
    if (!*head) {
        *head = *tail = label_tac;
    } else {
        (*tail)->next = label_tac;
        *tail = label_tac;
    }

    // Handle block types for control flow
    if (block->type == BLOCK_IF_THEN) {
        // Retrieve the condition variable from the last statement of the previous block
        BasicBlock *prev_block = block->preds[0];
        if (prev_block->stmt_count == 0) {
            LOG_ERROR("Previous block has no statements to derive condition variable");
            return;
        }

        ASTNode *last_stmt = prev_block->stmts[prev_block->stmt_count - 1];
        const char *temp_var = last_stmt->temp_var;
        if (!temp_var) {
            LOG_ERROR("Condition variable not set in the last statement of the previous block");
            return;
        }

        // Generate a jump in case the IF fails
        if (block->preds[0]->succ_count == 2) { // There is an else block
            int else_label = get_block_label(block->preds[0]->succs[1]->id);
            TAC *if_goto_tac = create_tac(TAC_IF_GOTO, NULL, temp_var, NULL, NULL, &else_label);
            LOG_INFO("Adding if-goto TAC for condition: L%d", else_label);
            (*tail)->next = if_goto_tac;
            *tail = if_goto_tac;
        } else {
            // Add a goto to the successor block if no else block exists
            int successor_label = get_block_label(block->succs[0]->id);
            TAC *goto_tac = create_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &successor_label);
            LOG_INFO("Adding goto TAC for successor block: L%d", successor_label);
            (*tail)->next = goto_tac;
            *tail = goto_tac;
        }
    }

    if (block->type == BLOCK_LOOP_HEADER) {
        // Skip phi functions (VarDecl with no initializer)
        size_t stmt_index = 0;
        while (stmt_index < block->stmt_count &&
               block->stmts[stmt_index]->type == NODE_VAR_DECL &&
               block->stmts[stmt_index]->data.var_decl.init_value == NULL) {
            LOG_INFO("Detected φ-function for variable: %s", block->stmts[stmt_index]->data.var_decl.name);
            TAC *new_tac = create_tac(TAC_PHI, block->stmts[stmt_index]->data.var_decl.name, NULL, NULL, NULL, NULL);
            (*tail)->next = new_tac;
            *tail = new_tac;
            stmt_index++;
        }

        // Ensure there is a condition to process
        if (stmt_index >= block->stmt_count) {
            LOG_ERROR("No condition found in loop header block");
            return;
        }

        // Process the condition (BinaryOp or expression)
        ASTNode *condition_stmt = block->stmts[stmt_index];
        if (condition_stmt->type == NODE_BINARY_OP || condition_stmt->temp_var) {
            process_statement(condition_stmt, block, stmt_index, tail);

            // Use the resulting temp_var to generate the `if not` statement
            const char *temp_var = condition_stmt->temp_var;
            if (!temp_var) {
                LOG_ERROR("Condition variable not set after processing condition statement");
                return;
            }

            int exit_label = get_block_label(block->succs[1]->id); // Exit block is the second successor
            TAC *if_not_tac = create_tac(TAC_IF_GOTO, NULL, temp_var, NULL, NULL, &exit_label);
            LOG_INFO("Adding if-not TAC for loop condition: L%d", exit_label);
            (*tail)->next = if_not_tac;
            *tail = if_not_tac;
        } else {
            LOG_ERROR("Unsupported condition type in loop header block");
        }
    }

    // Process statements in the block
    if (block->type != BLOCK_LOOP_HEADER) {
        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            if (stmt->type == NODE_BINARY_OP) {
                LOG_INFO("Binary operation detected in CFG");
                if (stmt->temp_var) {
                    LOG_INFO("Using temp_var for BinaryOp: %s", stmt->temp_var);
                } else {
                    LOG_ERROR("temp_var for BinaryOp is NULL");
                }
                // Ensure temp_var is propagated to the CFG
                block->stmts[j]->temp_var = stmt->temp_var;
            }
            process_statement(stmt, block, j, tail);
        }
    }

    // Ensure a jump to the successor block for non IF / ELSE blocks
    if (block->succ_count == 1) {
        int successor_label = get_block_label(block->succs[0]->id);
        TAC *goto_tac = create_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &successor_label);
        LOG_INFO("Adding unconditional goto TAC for successor block: L%d", successor_label);
        (*tail)->next = goto_tac;
        *tail = goto_tac;
    }

    // Recursively process successor blocks
    for (size_t i = 0; i < block->succ_count; i++) {
        traverse_cfg(block->succs[i], visited, head, tail);
    }
}

// Convert a CFG to TAC
TAC *cfg_to_tac(CFG *cfg) {
    LOG_INFO("CFG to TAC conversion started");

    TAC *head = NULL, *tail = NULL;

    bool *visited = calloc(cfg->block_count, sizeof(bool));
    if (!visited) {
        fprintf(stderr, "Error: Unable to allocate memory for visited array\n");
        exit(EXIT_FAILURE);
    }

    preassign_block_labels(cfg);
    traverse_cfg(cfg->blocks[0], visited, &head, &tail);

    free(visited);
    free_block_label_table();

    LOG_INFO("CFG to TAC conversion completed");
    return head;
}

static void rename_variables_in_ssa(TAC *tac, CFG *cfg) {
    LOG_INFO("Renaming variables in SSA format");
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            if (stmt->type == NODE_VAR_DECL || stmt->type == NODE_ASSIGNMENT) {
                const char *var_name = stmt->data.var_decl.name;
                int new_version = peek_var_version(var_name) + 1;
                push_var_version(var_name, new_version);
                char new_var_name[64];
                snprintf(new_var_name, sizeof(new_var_name), "%s_%d", var_name, new_version);
                stmt->data.var_decl.name = strdup(new_var_name);
            }
        }
    }
}

static void patch_phi_functions(TAC *tac, CFG *cfg) {
    LOG_INFO("Patching φ-functions");
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            if (stmt->type == NODE_VAR_DECL && stmt->data.var_decl.init_value == NULL) {
                const char *var_name = stmt->data.var_decl.name;
                for (size_t k = 0; k < block->pred_count; k++) {
                    BasicBlock *pred = block->preds[k];
                    int version = peek_var_version(var_name);
                    char pred_var_name[64];
                    snprintf(pred_var_name, sizeof(pred_var_name), "%s_%d", var_name, version);
                    // Update φ-function arguments here
                }
            }
        }
    }
}

// Convert TAC to SSA
void convert_to_ssa(TAC *tac, CFG *cfg) {
    LOG_INFO("Converting TAC to SSA form");

    // Step 1: Rename variables in SSA format
    rename_variables_in_ssa(tac, cfg);

    // Step 2: Patch φ-functions
    patch_phi_functions(tac, cfg);

    LOG_INFO("SSA conversion completed");
}

// Updated TAC printing to prepend 'L' to integer labels
void print_tac(TAC *tac, FILE *stream) {
    while (tac) {
        switch (tac->type) {
            case TAC_ASSIGN:
                if (tac->int_value != 0) {
                    fprintf(stream, "%s = %d\n", tac->result, tac->int_value);
                } else if (tac->ptr_value != NULL) {
                    fprintf(stream, "%s = %p\n", tac->result, tac->ptr_value);
                } else {
                    fprintf(stream, "%s = %s\n", tac->result, tac->arg1);
                }
                break;
            case TAC_BINARY_OP:
                fprintf(stream, "%s = %s %s %s\n", tac->result, tac->arg1, tac->op, tac->arg2);
                break;
            case TAC_UNARY_OP:
                fprintf(stream, "%s = %s %s\n", tac->result, tac->op, tac->arg1);
                break;
            case TAC_LABEL:
                fprintf(stream, "L%d:\n", *(tac->label));
                break;
            case TAC_GOTO:
                fprintf(stream, "goto L%d\n", *(tac->label));
                break;
            case TAC_IF_GOTO:
                fprintf(stream, "if not %s goto L%d\n", tac->arg1, *(tac->label));
                break;
            case TAC_RETURN:
                if (tac->int_value != 0) {
                    fprintf(stream, "return %d\n", tac->int_value);
                } else if (tac->ptr_value != NULL) {
                    fprintf(stream, "return %p\n", tac->ptr_value);
                } else {
                    fprintf(stream, "return %s\n", tac->result);
                }
                break;
            case TAC_PHI:
                fprintf(stream, "%s = phi(...)\n", tac->result);
                break;
            default:
                fprintf(stream, "Unknown TAC type\n");
                break;
        }
        tac = tac->next;
    }
}

// Free TAC instructions
void free_tac(TAC *tac) {
    while (tac) {
        TAC *next = tac->next;
        free(tac->result);
        free(tac->arg1);
        free(tac->arg2);
        free(tac->op);
        free(tac->label);
        free(tac);
        tac = next;
    }
}