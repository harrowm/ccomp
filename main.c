#include "lexer.h"
#include "ast.h"
#include "cfg.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Forward declaration of parse function from parser.c
ASTNode* parse(Lexer *lexer);

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
        
    // Clean up
    printf("\nCleaning up ...\n");
    free_cfg(cfg);
    free_ast(ast);
    free(code);

    printf("\nCompilation completed successfully.\n");
    return 0;
}
