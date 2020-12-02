#ifndef PARSER_H
#define PARSER_H

#include "datastructs.h"

enum {
    TREETYPE_NULL = 0,
    
    // lists
    TREETYPE_DECLARATION_LIST,
    TREETYPE_STATEMENT_LIST,
    
    TREETYPE_FUNCTION_DEFINITION,
    TREETYPE_BLOCK,
    TREETYPE_ASSIGN,
    TREETYPE_IF,
    TREETYPE_WHILE,
    TREETYPE_FOR,
    TREETYPE_VARIABLE,
    TREETYPE_OPERATOR,
    TREETYPE_ASM,
    TREETYPE_IDENTIFIER,
    TREETYPE_FUNC_CALL,
    TREETYPE_ARG_LIST,
    TREETYPE_DEFINE,
    
    // constants
    TREETYPE_INTEGER,
    TREETYPE_CHAR,
    TREETYPE_STRING,
};

typedef struct tree tree;

struct tree {
    int type;
    union {
        token *tok;
        variable *var;
        int int_value;
        char *string_value;
    } data;
    tree *left;
    tree *right;
};

tree *parse();
tree *create_tree(int type);
void free_tree(tree *t);
void print_tree(tree *t);

#endif
