#pragma once

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
};

typedef struct tree tree;

struct tree {
    int type;
    void *data;
    tree *left;
    tree *right;
};

tree *parse();
tree *create_tree(int type);
void print_tree(tree *t);
