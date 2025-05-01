#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

void print_ast(ASTNode *node, int indent);
const char *node_type_to_string(NodeType type);
void free_ast(ASTNode *node);
ASTNode* parse(Lexer *lexer);

#endif // PARSER_H

