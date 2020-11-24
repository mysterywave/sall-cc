#pragma once

#include "tokenizer.h"

#define STRING_BUILDER_GROW_EXPONENTIAL 0
#define STRING_BUILDER_DEALLOC_ON_CLEAR 0

#define VARLIST_GROW_EXPONENTIAL 0

typedef struct varlist varlist;

typedef struct string_builder {
    int length;
    int buffer_length;
    char *buffer;
} string_builder;

void string_builder_init(string_builder *builder);
void string_builder_end(string_builder *builder);

void string_builder_add(string_builder *builder, char c);
char *string_builder_get();
char string_builder_get_char(string_builder *builder, int offset);
void string_builder_clear(string_builder *builder);

enum {
    VARTYPE_VOID,
    VARTYPE_CHAR,
    VARTYPE_INT,
    VARTYPE_LONG,
    VARTYPE_FLOAT,
    VARTYPE_DOUBLE,
    VARTYPE_STRUCT,
    VARTYPE_UNION
};

typedef struct variable {
    int global_id;
    
    char *name;
    int type;
    int pointers;
    int address;
    int is_argument;
    int is_function;
    int is_constant;
    
    varlist *arguments;
} variable;

variable *create_variable();
void delete_variable();

struct varlist {
    int length;
    int buffer_length;
    variable **list;
};

varlist *create_varlist();
void delete_varlist(varlist *list);

void varlist_add(varlist *list, variable *var);
variable *varlist_get(varlist *list, int id);
void print_varlist(varlist *list);

typedef struct toklist {
    int length;
    int buffer_length;
    token **list;
} toklist;

toklist *create_toklist();
void delete_toklist(toklist *list);

void toklist_add(toklist *list, token *var);
token *toklist_get(toklist *list, int id);

typedef struct opstack_link opstack_link;

struct opstack_link {
    token *value;
    opstack_link *last;
    opstack_link *next;
};

typedef struct opstack {
    opstack_link *start;
    opstack_link *end;
} opstack;

opstack *create_opstack();
void delete_opstack(opstack *stack);

void opstack_push(opstack *stack, token *tok);
token *opstack_peek(opstack *stack);
token *opstack_pop(opstack *stack);
int opstack_empty(opstack *stack);
void print_opstack(opstack *stack);
