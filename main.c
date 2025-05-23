/*
 * File: main.c
 * Description: Entry point for the compiler program.
 * Purpose: Orchestrates the compilation process by invoking various components.
 */

#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "cfg.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Forward declaration of parse function from parser.c
// ASTNode* parse(Lexer *lexer);
// now in parser.h

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Open the file
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        LOG_ERROR("Error opening source file");
        return 1;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for file contents (+1 for null terminator)
    char *code = malloc(file_size + 1);
    if (!code) {
        LOG_ERROR("Memory allocation failed for source code buffer");
        fclose(file);
        return 1;
    }

    // Read file contents
    size_t bytes_read = fread(code, 1, file_size, file);
    if (bytes_read != file_size) {
        LOG_ERROR("Error reading source code file");
        free(code);
        fclose(file);
        return 1;
    }

    code[file_size] = '\0'; // Null-terminate the string

    fclose(file);

    // Initialize the lexer
    Lexer lexer;
    lexer_init(&lexer, code);

    // Parse the input into an AST
    ASTNode *ast = parse(&lexer);
    if (!ast) {
        LOG_ERROR("Error parsing input");
        free(code);
        return 1;
    }

    // Print original AST
    printf("Original Abstract Syntax Tree:\n");
    printf("==============================\n");
    print_ast(ast, 0);


    // Convert AST to CFG
    printf("\nConverting to Control Flow Graph...\n");
    CFG *cfg = ast_to_cfg(ast);
    if (!cfg) {
        LOG_ERROR("Error creating CFG");
        free_ast(ast);
        free(code);
        return 1;
    }
    print_cfg(cfg, stdout);

    generate_dot_file(cfg, "cfg.dot");
    printf("Control Flow Graph saved to cfg.dot\n");

    // Compute the dominator tree
    printf("\nComputing Dominator Tree...\n");
    compute_dominator_tree(cfg);
    printf("Dominator Tree computed successfully.\n");

    // Compute dominance frontiers
    printf("\nComputing Dominance Frontiers...\n");
    compute_dominance_frontiers(cfg);
    generate_dominance_frontiers_dot(cfg, "df.dot");
    printf("Dominance Frontiers saved to df.dot\n");

    // Insert φ-functions into the CFG
    printf("\nInserting φ-functions into the CFG...\n");
    insert_phi_functions(cfg);
    printf("φ-functions inserted successfully.\n");

    // Generate DOT file for modified CFG
    generate_dot_file(cfg, "cfg_with_phi.dot");
    printf("Modified Control Flow Graph with φ-functions saved to cfg_with_phi.dot\n");
        
    // Clean up
    printf("\nCleaning up ...\n");
    free_cfg(cfg);
    free_ast(ast);
    free(code);

    printf("\nCompilation completed successfully.\n");
    return 0;
}
