// Minimal SSA renaming for TAC list (single-pass, no phi insertion)
// This does not handle control flow joins, but is enough for straight-line code and simple tests.
#include "tac.h"
#include <string.h>
void tac_rename_variables_ssa(TAC *tac) {
    typedef struct { char name[64]; int version; } VarVersion;
    VarVersion versions[256];
    int nvars = 0;

    static char buf[128];
    #define GET_VERSIONED(name, version) (snprintf(buf, sizeof(buf), "%s_%d", name, version), buf)

    for (TAC *curr = tac; curr != NULL; curr = curr->next) {
        if (curr->result && curr->type != TAC_LABEL && curr->type != TAC_PHI && curr->type != TAC_CALL) {
            int idx = -1;
            for (int i = 0; i < nvars; ++i) {
                if (strcmp(versions[i].name, curr->result) == 0) { idx = i; break; }
            }
            if (idx == -1) {
                idx = nvars++;
                strncpy(versions[idx].name, curr->result, 63);
                versions[idx].name[63] = 0;
                versions[idx].version = 0;
            } else {
                versions[idx].version++;
            }
            free(curr->result);
            curr->result = strdup(GET_VERSIONED(versions[idx].name, versions[idx].version));
        }
        if (curr->arg1) {
            for (int i = 0; i < nvars; ++i) {
                size_t len = strlen(versions[i].name);
                if (strncmp(curr->arg1, versions[i].name, len) == 0 && (curr->arg1[len] == 0 || curr->arg1[len] == '_')) {
                    free(curr->arg1);
                    curr->arg1 = strdup(GET_VERSIONED(versions[i].name, versions[i].version));
                    break;
                }
            }
        }
        if (curr->arg2) {
            for (int i = 0; i < nvars; ++i) {
                size_t len = strlen(versions[i].name);
                if (strncmp(curr->arg2, versions[i].name, len) == 0 && (curr->arg2[len] == 0 || curr->arg2[len] == '_')) {
                    free(curr->arg2);
                    curr->arg2 = strdup(GET_VERSIONED(versions[i].name, versions[i].version));
                    break;
                }
            }
        }
        if (curr->type == TAC_RETURN && curr->result) {
            for (int i = 0; i < nvars; ++i) {
                size_t len = strlen(versions[i].name);
                if (strncmp(curr->result, versions[i].name, len) == 0 && (curr->result[len] == 0 || curr->result[len] == '_')) {
                    free(curr->result);
                    curr->result = strdup(GET_VERSIONED(versions[i].name, versions[i].version));
                    break;
                }
            }
        }
    }
    #undef GET_VERSIONED
}

#include "tac.h"
#include "lexer.h" // Include token definitions like TOK_PLUS
#include "debug.h" // Include debug.h for logging macros
#include "parser.h" // For node_type_to_string
#include "cfg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h> // For boolean type
#include <stddef.h>  // For size_t

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

// Helper to create a new TAC instruction (renamed to avoid conflict with create_tac(CFG *))
static TAC *make_tac(TACType type, const char *result, const char *arg1, const char *arg2, const char *op, int *label) {
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
    tac->int_label = NULL;
    tac->str_label = NULL;
    if (type == TAC_LABEL && op) {
        tac->str_label = strdup(op);
    } else if ((type == TAC_LABEL || type == TAC_GOTO || type == TAC_IF_GOTO) && label) {
        tac->int_label = malloc(sizeof(int));
        *(tac->int_label) = *label;
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

// Helper function to check if a variable is already in the set
static bool is_var_in_set(const char **set, size_t set_size, const char *var_name) {
    for (size_t i = 0; i < set_size; i++) {
        if (strcmp(set[i], var_name) == 0) {
            return true;
        }
    }
    return false;
}

// Helper function to add a variable to the set
static void add_var_to_set(const char **set, size_t *set_size, size_t set_capacity, const char *var_name) {
    if (*set_size < set_capacity) {
        set[*set_size] = var_name;
        (*set_size)++;
    } else {
        fprintf(stderr, "Error: Variable set capacity exceeded\n");
        exit(EXIT_FAILURE);
    }
}

// Updated process_statement to include binary operation handling
static void process_statement(ASTNode *stmt, BasicBlock *block, size_t stmt_index) {
    LOG_INFO("Processing statement %zu in block %zu of type %s", stmt_index, block->id, cfg_node_type_to_string(stmt->type));
    TAC *new_tac = NULL;

    switch (stmt->type) {
        case NODE_VAR_DECL:
            LOG_INFO("Variable declaration: %s", stmt->data.var_decl.name);
            if (stmt->data.var_decl.init_value == NULL) {
                // Do not emit TAC for phi here; handled in create_tac for join/merge blocks
                break;
            } else if (stmt->data.var_decl.init_value->type == NODE_FUNCTION_CALL) {
                // Emit param TACs for each argument
                ASTNode *call = stmt->data.var_decl.init_value;
                for (size_t i = 0; i < call->data.function_call.arg_count; i++) {
                    char *arg_val = extract_node_value_as_string(call->data.function_call.args[i]);
                    TAC *param_tac = make_tac(TAC_ASSIGN, "param", arg_val, NULL, NULL, NULL);
                    if (block->tac_tail == NULL) {
                        block->tac_head = block->tac_tail = param_tac;
                    } else {
                        block->tac_tail->next = param_tac;
                        block->tac_tail = param_tac;
                    }
                    free(arg_val);
                }
                // Handle function call initializer: x = call foo
                new_tac = make_tac(TAC_CALL, stmt->data.var_decl.name, call->data.function_call.name, NULL, NULL, NULL);
            } else {
                NodeValue init_value = extract_node_value(stmt->data.var_decl.init_value);
                if (init_value.type == NODE_TYPE_LITERAL) {
                    new_tac = make_tac(TAC_ASSIGN, stmt->data.var_decl.name, init_value.data.string_value, NULL, NULL, NULL);
                } else if (init_value.type == NODE_TYPE_VAR_REF) {
                    new_tac = make_tac(TAC_ASSIGN, stmt->data.var_decl.name, init_value.data.string_value, NULL, NULL, NULL);
                } else {
                    LOG_ERROR("Unsupported initializer type for variable declaration");
                }
            }

            if (new_tac) {
                if (block->tac_tail == NULL) {
                    block->tac_head = block->tac_tail = new_tac;
                } else {
                    block->tac_tail->next = new_tac;
                    block->tac_tail = new_tac;
                }
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
                    TAC *binary_tac = make_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);

                    if (block->tac_tail == NULL) {
                        block->tac_head = block->tac_tail = binary_tac;
                    } else {
                        block->tac_tail->next = binary_tac;
                        block->tac_tail = binary_tac;
                    }

                    free(left_operand);
                    free(right_operand);

                    // Instead of assigning to x here, assign to temp in then/else blocks
                    // new_tac = make_tac(TAC_ASSIGN, stmt->data.assignment.name, temp_var, NULL, NULL, NULL);
                    // Do not emit assignment to x here; let phi handle merge in join block
                } else {
                    NodeValue assign_value = extract_node_value(stmt->data.assignment.value);
                    if (assign_value.type == NODE_TYPE_LITERAL) {
                        new_tac = make_tac(TAC_ASSIGN, stmt->data.assignment.name, NULL, NULL, NULL, NULL);
                        new_tac->int_value = atoi(assign_value.data.string_value);
                    } else if (assign_value.type == NODE_TYPE_VAR_REF) {
                        new_tac = make_tac(TAC_ASSIGN, stmt->data.assignment.name, assign_value.data.string_value, NULL, NULL, NULL);
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
                process_statement(stmt->data.binary_op.left, block, stmt_index);
                LOG_INFO("Left operand temp_var after processing: %s", stmt->data.binary_op.left->temp_var);

                LOG_INFO("Processing right operand of BinaryOp");
                process_statement(stmt->data.binary_op.right, block, stmt_index);
                LOG_INFO("Right operand temp_var after processing: %s", stmt->data.binary_op.right->temp_var);

                char temp_var[32];
                generate_unique_var_name(temp_var, sizeof(temp_var), "t");
                stmt->temp_var = strdup(temp_var); // Store temp variable in ASTNode

                LOG_INFO("Generated temp_var for BinaryOp: %s", temp_var);

                char *left_operand = extract_node_value_as_string(stmt->data.binary_op.left);
                char *right_operand = extract_node_value_as_string(stmt->data.binary_op.right);

                const char *op = operator_to_string(stmt->data.binary_op.op);
                new_tac = make_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);

                if (block->tac_tail == NULL) {
                    block->tac_head = block->tac_tail = new_tac;
                } else {
                    block->tac_tail->next = new_tac;
                    block->tac_tail = new_tac;
                }

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
                    process_statement(stmt->data.return_stmt.value->data.binary_op.left, block, stmt_index);
                    LOG_INFO("Left operand temp_var after processing: %s", stmt->data.return_stmt.value->data.binary_op.left->temp_var);

                    // Recursive processing for right operand
                    LOG_INFO("Processing right operand of BinaryOp in return");
                    process_statement(stmt->data.return_stmt.value->data.binary_op.right, block, stmt_index);
                    LOG_INFO("Right operand temp_var after processing: %s", stmt->data.return_stmt.value->data.binary_op.right->temp_var);

                    char temp_var[32];
                    generate_unique_var_name(temp_var, sizeof(temp_var), "t");
                    stmt->data.return_stmt.value->temp_var = strdup(temp_var); // Store temp variable in ASTNode

                    LOG_INFO("Generated temp_var for BinaryOp in return: %s", temp_var);

                    char *left_operand = extract_node_value_as_string(stmt->data.return_stmt.value->data.binary_op.left);
                    char *right_operand = extract_node_value_as_string(stmt->data.return_stmt.value->data.binary_op.right);

                    const char *op = operator_to_string(stmt->data.return_stmt.value->data.binary_op.op);
                    TAC *binary_tac = make_tac(TAC_BINARY_OP, temp_var, left_operand, right_operand, op, NULL);

                    if (block->tac_tail == NULL) {
                        block->tac_head = block->tac_tail = binary_tac;
                    } else {
                        block->tac_tail->next = binary_tac;
                        block->tac_tail = binary_tac;
                    }

                    LOG_INFO("Generated TAC for BinaryOp in return: %s = %s %s %s", temp_var, left_operand, op, right_operand);

                    free(left_operand);
                    free(right_operand);

                    new_tac = make_tac(TAC_RETURN, temp_var, NULL, NULL, NULL, NULL);
                } else {
                    NodeValue return_value = extract_node_value(stmt->data.return_stmt.value);
                    if (return_value.type == NODE_TYPE_LITERAL) {
                        new_tac = make_tac(TAC_RETURN, NULL, NULL, NULL, NULL, NULL);
                        new_tac->int_value = atoi(return_value.data.string_value);
                    } else if (return_value.type == NODE_TYPE_VAR_REF) {
                        new_tac = make_tac(TAC_RETURN, return_value.data.string_value, NULL, NULL, NULL, NULL);
                    } else {
                        LOG_ERROR("Unsupported return value type");
                    }
                }
            }
            break;

        case NODE_FUNCTION_CALL:
            // Standalone function call (not in assignment or var_decl)
            for (size_t i = 0; i < stmt->data.function_call.arg_count; i++) {
                char *arg_val = extract_node_value_as_string(stmt->data.function_call.args[i]);
                TAC *param_tac = make_tac(TAC_ASSIGN, "param", arg_val, NULL, NULL, NULL);
                if (block->tac_tail == NULL) {
                    block->tac_head = block->tac_tail = param_tac;
                } else {
                    block->tac_tail->next = param_tac;
                    block->tac_tail = param_tac;
                }
                free(arg_val);
            }
            TAC *call_tac = make_tac(TAC_CALL, NULL, stmt->data.function_call.name, NULL, NULL, NULL);
            if (block->tac_tail == NULL) {
                block->tac_head = block->tac_tail = call_tac;
            } else {
                block->tac_tail->next = call_tac;
                block->tac_tail = call_tac;
            }
            break;

        case NODE_LITERAL:
        case NODE_VAR_REF:
            break;

        default:
            LOG_INFO("Unsupported AST node type in CFG to TAC conversion: %d - %s", stmt->type, cfg_node_type_to_string(stmt->type));
            break;
    }

    if (new_tac) {
        LOG_INFO("Generated TAC: type=%d, result=%s, arg1=%s, arg2=%s, op=%s, int_label=%s, str_label=%s", 
                 new_tac->type, new_tac->result, new_tac->arg1, new_tac->arg2, new_tac->op, 
                 new_tac->int_label ? "set" : "NULL", new_tac->str_label ? new_tac->str_label : "NULL");
        if (block->tac_tail == NULL) {
            block->tac_head = block->tac_tail = new_tac;
        } else {
            block->tac_tail->next = new_tac;
            block->tac_tail = new_tac;
        }
    }
}

// Updated handling for CFG block types in TAC generation
static void traverse_cfg(CFG *cfg, BasicBlock *block, bool *visited, TAC **head, TAC **tail) {
    if (visited[block->id]) {
        return; // Block already visited
    }
    visited[block->id] = true;

    // Emit a function label if this is a function entry block
    if (block->function_name) {
        TAC *func_label_tac = make_tac(TAC_LABEL, NULL, NULL, NULL, block->function_name, NULL);
        if (block->tac_tail == NULL) {
            block->tac_head = block->tac_tail = func_label_tac;
        } else {
            block->tac_tail->next = func_label_tac;
            block->tac_tail = func_label_tac;
        }
        TAC *prologue_tac = make_tac(TAC_ASSIGN, "__enter", block->function_name, NULL, NULL, NULL);
        block->tac_tail->next = prologue_tac;
        block->tac_tail = prologue_tac;
    }

    int label = get_block_label(block->id);
    LOG_INFO("Adding block label: L%d", label);
    TAC *label_tac = make_tac(TAC_LABEL, NULL, NULL, NULL, NULL, &label);
    if (block->tac_tail == NULL) {
        block->tac_head = block->tac_tail = label_tac;
    } else {
        block->tac_tail->next = label_tac;
        block->tac_tail = label_tac;
    }

    LOG_DEBUG("Checking for entry block: block=%p, entry=%p, stmt_count=%zu, succ_count=%zu", (void*)block, (void*)cfg->blocks[0], (size_t)block->stmt_count, (size_t)block->succ_count);
    // Special case: entry block is empty, emit call to main if main exists, then goto exit block
    if (block == cfg->blocks[0] && block->stmt_count == 0 && block->succ_count > 0) {
        LOG_DEBUG("Entry block is empty and has successors, searching for main block by function_name...");
        size_t main_block_id = (size_t)-1;
        for (size_t i = 0; i < cfg->block_count; i++) {
            BasicBlock *b = cfg->blocks[i];
            if (b->function_name && strcmp(b->function_name, "main") == 0) {
                main_block_id = b->id;
                LOG_DEBUG("Found main() entry block: id=%zu", main_block_id);
                break;
            }
        }
        if (main_block_id != (size_t)-1) {
            // Emit call to main
            TAC *call_main = make_tac(TAC_CALL, NULL, "main", NULL, NULL, NULL);
            if (block->tac_tail == NULL) {
                block->tac_head = block->tac_tail = call_main;
            } else {
                block->tac_tail->next = call_main;
                block->tac_tail = call_main;
            }
            // Emit goto to exit block
            int exit_label = get_block_label(cfg->exit->id);
            TAC *goto_exit = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &exit_label);
            block->tac_tail->next = goto_exit;
            block->tac_tail = goto_exit;
        } else {
            LOG_DEBUG("No main() function entry block found in CFG.");
        }
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
            TAC *if_goto_tac = make_tac(TAC_IF_GOTO, NULL, temp_var, NULL, NULL, &else_label);
            LOG_INFO("Adding if-goto TAC for condition: L%d", else_label);
            if (*tail == NULL) {
                *tail = if_goto_tac;
            } else {
                (*tail)->next = if_goto_tac;
                *tail = if_goto_tac;
            }
        } else {
            // Add a goto to the successor block if no else block exists
            int successor_label = get_block_label(block->succs[0]->id);
            TAC *goto_tac = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &successor_label);
            LOG_INFO("Adding goto TAC for successor block: L%d", successor_label);
            if (*tail == NULL) {
                *tail = goto_tac;
            } else {
                (*tail)->next = goto_tac;
                *tail = goto_tac;
            }
        }
    }

    // Only emit phi functions at the top of join/merge blocks (BLOCK_NORMAL with >1 predecessor)
    if (block->type == BLOCK_NORMAL && block->pred_count > 1 && block->phi_count > 0) {
        for (size_t i = 0; i < block->phi_count; ++i) {
            const char *var_name = block->phi_vars[i];
            // Only emit phi if not already present in TAC for this block
            int already_emitted = 0;
            for (TAC *t = block->tac_head; t != NULL; t = t->next) {
                if (t->type == TAC_PHI && t->result && strcmp(t->result, var_name) == 0) {
                    already_emitted = 1;
                    break;
                }
            }
            if (!already_emitted) {
                LOG_INFO("Detected φ-function for variable: %s", var_name);
                TAC *phi_tac = make_tac(TAC_PHI, var_name, NULL, NULL, NULL, NULL);
                if (*tail == NULL) {
                    *tail = phi_tac;
                } else {
                    (*tail)->next = phi_tac;
                    *tail = phi_tac;
                }
                if (block->tac_head == NULL) block->tac_head = phi_tac;
            }
        }
    }

    // Only emit statements for blocks that are not empty (skip empty blocks)
    if (block->stmt_count > 0) {
        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            process_statement(stmt, block, j);
        }
    }

    // Ensure a jump to the successor block for non IF / ELSE blocks
    if (block->succ_count == 1) {
        int successor_label = get_block_label(block->succs[0]->id);
        TAC *goto_tac = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &successor_label);
        LOG_INFO("Adding unconditional goto TAC for successor block: L%d", successor_label);
        if (*tail == NULL) {
            *tail = goto_tac;
        } else {
            (*tail)->next = goto_tac;
            *tail = goto_tac;
        }
    }

    // Recursively process successor blocks
    for (size_t i = 0; i < block->succ_count; i++) {
        traverse_cfg(cfg, block->succs[i], visited, head, tail);
    }

    // Emit halt in the exit block
    if (block->type == BLOCK_EXIT) {
        TAC *halt_tac = make_tac(TAC_ASSIGN, "halt", NULL, NULL, NULL, NULL);
        if (block->tac_tail == NULL) {
            block->tac_head = block->tac_tail = halt_tac;
        } else {
            block->tac_tail->next = halt_tac;
            block->tac_tail = halt_tac;
        }
    }
}

// Convert a CFG to TAC (void version)
void create_tac(CFG *cfg) {
    LOG_INFO("CFG to TAC conversion started");

    preassign_block_labels(cfg);

    // For each block, emit TAC in canonical order (no recursion)
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        block->tac_head = block->tac_tail = NULL;

        // Emit function label and prologue if function entry
        if (block->function_name) {
            TAC *func_label_tac = make_tac(TAC_LABEL, NULL, NULL, NULL, block->function_name, NULL);
            block->tac_head = block->tac_tail = func_label_tac;
            TAC *prologue_tac = make_tac(TAC_ASSIGN, "__enter", block->function_name, NULL, NULL, NULL);
            block->tac_tail->next = prologue_tac;
            block->tac_tail = prologue_tac;
        }

        // Emit block label
        int label = get_block_label(block->id);
        TAC *label_tac = make_tac(TAC_LABEL, NULL, NULL, NULL, NULL, &label);
        if (block->tac_tail == NULL) {
            block->tac_head = block->tac_tail = label_tac;
        } else {
            block->tac_tail->next = label_tac;
            block->tac_tail = label_tac;
        }

        // Special case: entry block emits call to main and goto exit
        if (block == cfg->blocks[0] && block->stmt_count == 0 && block->succ_count > 0) {
            size_t main_block_id = (size_t)-1;
            for (size_t j = 0; j < cfg->block_count; j++) {
                BasicBlock *b = cfg->blocks[j];
                if (b->function_name && strcmp(b->function_name, "main") == 0) {
                    main_block_id = b->id;
                    break;
                }
            }
            if (main_block_id != (size_t)-1) {
                TAC *call_main = make_tac(TAC_CALL, NULL, "main", NULL, NULL, NULL);
                block->tac_tail->next = call_main;
                block->tac_tail = call_main;
                int exit_label = get_block_label(cfg->exit->id);
                TAC *goto_exit = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &exit_label);
                block->tac_tail->next = goto_exit;
                block->tac_tail = goto_exit;
            }
            continue;
        }

        // Emit phi functions at top of join/merge blocks (only once per variable)
        if (block->type == BLOCK_NORMAL && block->pred_count > 1 && block->phi_count > 0) {
            for (size_t k = 0; k < block->phi_count; ++k) {
                const char *var_name = block->phi_vars[k];
                // Only emit phi if not already present in TAC for this block
                int already_emitted = 0;
                for (TAC *t = block->tac_head; t != NULL; t = t->next) {
                    if (t->type == TAC_PHI && t->result && strcmp(t->result, var_name) == 0) {
                        already_emitted = 1;
                        break;
                    }
                }
                if (!already_emitted) {
                    TAC *phi_tac = make_tac(TAC_PHI, var_name, NULL, NULL, NULL, NULL);
                    if (block->tac_tail == NULL) {
                        block->tac_head = block->tac_tail = phi_tac;
                    } else {
                        block->tac_tail->next = phi_tac;
                        block->tac_tail = phi_tac;
                    }
                }
            }
        }

        // Remove duplicate phi TACs (keep only the first for each variable)
        if (block->type == BLOCK_NORMAL && block->pred_count > 1 && block->phi_count > 0) {
            TAC *prev = NULL, *curr = block->tac_head;
            while (curr) {
                if (curr->type == TAC_PHI) {
                    // Check for duplicate phi for same variable later in the list
                    TAC *runner = curr->next, *runner_prev = curr;
                    while (runner) {
                        if (runner->type == TAC_PHI && runner->result && curr->result && strcmp(runner->result, curr->result) == 0) {
                            // Remove runner
                            runner_prev->next = runner->next;
                            if (runner == block->tac_tail) block->tac_tail = runner_prev;
                            free(runner->result); free(runner->arg1); free(runner->arg2); free(runner->op);
                            if (runner->int_label) free(runner->int_label);
                            if (runner->str_label) free(runner->str_label);
                            TAC *to_free = runner;
                            runner = runner->next;
                            free(to_free);
                        } else {
                            runner_prev = runner;
                            runner = runner->next;
                        }
                    }
                }
                prev = curr;
                curr = curr->next;
            }
        }



        // Emit statements for this block
        for (size_t j = 0; j < block->stmt_count; j++) {
            ASTNode *stmt = block->stmts[j];
            process_statement(stmt, block, j);
            // No need to update tac_tail here; process_statement maintains it.
        }

        // Emit if/else as: if (cond) goto then; goto else;
        // Only for blocks with exactly 2 successors and a conditional at the end
        if (block->succ_count == 2 && block->tac_tail && block->tac_tail->type == TAC_BINARY_OP) {
            // Find the last temp var (the condition)
            const char *cond_var = block->tac_tail->result;
            int else_label = get_block_label(block->succs[1]->id);
            int then_label = get_block_label(block->succs[0]->id);
            // Emit: if not cond goto else
            TAC *if_goto = make_tac(TAC_IF_GOTO, NULL, cond_var, NULL, NULL, &else_label);
            block->tac_tail->next = if_goto;
            block->tac_tail = if_goto;
            // Emit: goto then
            TAC *goto_then = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &then_label);
            block->tac_tail->next = goto_then;
            block->tac_tail = goto_then;
        } else if (block->succ_count == 1) {
            // Emit unconditional goto for blocks with a single successor
            int successor_label = get_block_label(block->succs[0]->id);
            TAC *goto_tac = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &successor_label);
            block->tac_tail->next = goto_tac;
            block->tac_tail = goto_tac;
        } else if (block->succ_count > 1) {
            // Fallback: emit gotos for all successors (should not happen in canonical SSA)
            for (size_t s = 0; s < block->succ_count; ++s) {
                int successor_label = get_block_label(block->succs[s]->id);
                TAC *goto_tac = make_tac(TAC_GOTO, NULL, NULL, NULL, NULL, &successor_label);
                block->tac_tail->next = goto_tac;
                block->tac_tail = goto_tac;
            }
        }

        // Emit halt in the exit block
        if (block->type == BLOCK_EXIT) {
            TAC *halt_tac = make_tac(TAC_ASSIGN, "halt", NULL, NULL, NULL, NULL);
            block->tac_tail->next = halt_tac;
            block->tac_tail = halt_tac;
        }
    }

    free_block_label_table();
    LOG_INFO("CFG to TAC conversion completed");
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
    // For each phi node in the TAC list, patch its arguments with SSA names from predecessors
    for (TAC *phi = tac; phi != NULL; phi = phi->next) {
        if (phi->type != TAC_PHI || !phi->result) continue;
        // Find the block this phi belongs to
        BasicBlock *block = NULL;
        for (size_t i = 0; i < cfg->block_count; i++) {
            for (size_t j = 0; j < cfg->blocks[i]->stmt_count; j++) {
                ASTNode *stmt = cfg->blocks[i]->stmts[j];
                if (stmt->type == NODE_VAR_DECL && stmt->data.var_decl.name && strcmp(stmt->data.var_decl.name, phi->result) == 0) {
                    block = cfg->blocks[i];
                    break;
                }
            }
            if (block) break;
        }
        if (!block) continue;
        // For each predecessor, find the last assignment to this variable in that pred's TACs
        char **phi_args = calloc(block->pred_count, sizeof(char*));
        for (size_t k = 0; k < block->pred_count; k++) {
            BasicBlock *pred = block->preds[k];
            char base[64];
            strncpy(base, phi->result, sizeof(base));
            char *uscore = strrchr(base, '_');
            if (uscore) *uscore = 0;
            char *ssa_name = NULL;
            for (TAC *t = pred->tac_head; t != NULL; t = t->next) {
                if (t->result && strncmp(t->result, base, strlen(base)) == 0 && t->result[strlen(base)] == '_' && t->type == TAC_ASSIGN) {
                    ssa_name = t->result;
                }
            }
            if (ssa_name) phi_args[k] = strdup(ssa_name);
            else phi_args[k] = strdup(phi->result); // fallback
        }
        // Free any previous args
        if (phi->arg1) { free(phi->arg1); phi->arg1 = NULL; }
        if (phi->arg2) { free(phi->arg2); phi->arg2 = NULL; }
        // Concatenate all phi args into a comma-separated string for arg1
        size_t total_len = 0;
        for (size_t k = 0; k < block->pred_count; k++) total_len += strlen(phi_args[k]) + 2;
        char *all_args = malloc(total_len + 1);
        all_args[0] = 0;
        for (size_t k = 0; k < block->pred_count; k++) {
            strcat(all_args, phi_args[k]);
            if (k + 1 < block->pred_count) strcat(all_args, ",");
        }
        phi->arg1 = all_args;
        // Clean up
        for (size_t k = 0; k < block->pred_count; k++) free(phi_args[k]);
        free(phi_args);
    }
}

// Convert TAC to SSA
// SSA conversion for TAC: just do variable renaming for now
void convert_to_ssa(TAC *tac, CFG *cfg) {
    tac_rename_variables_ssa(tac);
    patch_phi_functions(tac, cfg);
}

// Print TACs for a single basic block, including the block label as in the test expectations
void print_tac_bb(BasicBlock *block, FILE *stream) {
    // Print the block label as in the test expectations
    const char *block_type_str = NULL;
    switch (block->type) {
        case BLOCK_ENTRY: block_type_str = "Entry Block"; break;
        case BLOCK_EXIT: block_type_str = "Exit Block"; break;
        case BLOCK_IF_THEN: block_type_str = "If-Then Block"; break;
        case BLOCK_IF_ELSE: block_type_str = "If-Else Block"; break;
        case BLOCK_LOOP_HEADER: block_type_str = "Loop Header Block"; break;
        case BLOCK_LOOP_BODY: block_type_str = "Loop Body Block"; break;
        case BLOCK_NORMAL: block_type_str = "Normal Block"; break;
        default: block_type_str = "Unknown Block Type"; break;
    }
    fprintf(stream, "# BasicBlock %zu (%s)\n", block->id, block_type_str);

    TAC *tac = block->tac_head;
    while (tac) {
        switch (tac->type) {
            case TAC_ASSIGN:
                if (tac->result && strcmp(tac->result, "param") == 0) {
                    fprintf(stream, "param %s\n", tac->arg1);
                } else if (tac->result && strcmp(tac->result, "halt") == 0) {
                    fprintf(stream, "halt\n");
                } else {
                    if (tac->int_value != 0) {
                        fprintf(stream, "%s = %d\n", tac->result, tac->int_value);
                    } else if (tac->ptr_value != NULL) {
                        fprintf(stream, "%s = %p\n", tac->result, tac->ptr_value);
                    } else {
                        fprintf(stream, "%s = %s\n", tac->result, tac->arg1);
                    }
                }
                break;
            case TAC_BINARY_OP:
                fprintf(stream, "%s = %s %s %s\n", tac->result, tac->arg1, tac->op, tac->arg2);
                break;
            case TAC_UNARY_OP:
                fprintf(stream, "%s = %s %s\n", tac->result, tac->op, tac->arg1);
                break;
            case TAC_LABEL:
                if (tac->str_label) {
                    fprintf(stream, "%s:\n", tac->str_label);
                } else if (tac->int_label) {
                    fprintf(stream, "L%d:\n", *(tac->int_label));
                }
                break;
            case TAC_GOTO:
                if (tac->int_label)
                    fprintf(stream, "goto L%d\n", *(tac->int_label));
                break;
            case TAC_IF_GOTO:
                if (tac->int_label)
                    fprintf(stream, "if not %s goto L%d\n", tac->arg1, *(tac->int_label));
                break;
            case TAC_RETURN:
                if (tac->result == NULL) {
                    fprintf(stream, "return %d\n", tac->int_value);
                } else if (tac->ptr_value != NULL) {
                    fprintf(stream, "return %p\n", tac->ptr_value);
                } else {
                    fprintf(stream, "return %s\n", tac->result);
                }
                break;
            case TAC_PHI:
                fprintf(stream, "%s = phi(%s)\n", tac->result, tac->arg1 ? tac->arg1 : "...");
                break;
            case TAC_CALL:
                fprintf(stream, "%s = call %s\n", tac->result ? tac->result : "_", tac->arg1);
                break;
            default:
                fprintf(stream, "Unknown TAC type\n");
                break;
        }
        tac = tac->next;
    }
}

// Print TACs for each basic block in the CFG
void print_tac(CFG *cfg, FILE *stream) {
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *block = cfg->blocks[i];
        print_tac_bb(block, stream);
        fprintf(stream, "\n");
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
        if (tac->int_label) free(tac->int_label);
        if (tac->str_label) free(tac->str_label);
        free(tac);
        tac = next;
    }
}