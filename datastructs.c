#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "datastructs.h"

void string_builder_init(string_builder *builder) {
    builder->length = 0;
    builder->buffer_length = 16;
    builder->buffer = malloc(16);
}

void string_builder_end(string_builder *builder) {
    free(builder->buffer);
}

void string_builder_add(string_builder *builder, char c) {
    if(builder->length == builder->buffer_length) {
        #if STRING_GROW_EXPONENTIAL
            builder->buffer_length <<= 1;
        #else
            builder->buffer_length += 16;
        #endif
        builder->buffer = realloc(builder->buffer, builder->buffer_length);
    }
    builder->buffer[builder->length] = c;
    builder->length++;
}

char *string_builder_get(string_builder *builder) {
    if(builder->length == builder->buffer_length) {
        #if STRING_GROW_EXPONENTIAL
            builder->buffer_length <<= 1;
        #else
            builder->buffer_length += 16;
        #endif
        builder->buffer = realloc(builder->buffer, builder->buffer_length);
    }
    builder->buffer[builder->length] = '\0';
    return builder->buffer;
}

char string_builder_get_char(string_builder *builder, int offset) {
    return builder->buffer[offset];
}

void string_builder_clear(string_builder *builder) {
    #if STRING_BUILDER_DEALLOC_ON_CLEAR
        builder->buffer = realloc(builder->buffer, 16);
    #endif
    builder->length = 0;
}

variable *create_variable() {
    variable *out = malloc(sizeof(variable));
    memset(out, 0, sizeof(variable));
    return out;
}

void print_variable(variable *var) {
    printf("\x1b[93;1m%s\x1b[0m {\n", var->name);
    switch(var->type) {
        case VARTYPE_VOID:
            printf("  \x1b[94;1mtype\x1b[0m = void\n");
            break;
        case VARTYPE_CHAR:
            printf("  \x1b[94;1mtype\x1b[0m = char\n");
            break;
        case VARTYPE_INT:
            printf("  \x1b[94;1mtype\x1b[0m = int\n");
            break;
        case VARTYPE_LONG:
            printf("  \x1b[94;1mtype\x1b[0m = long\n");
            break;
        case VARTYPE_FLOAT:
            printf("  \x1b[94;1mtype\x1b[0m = float\n");
            break;
        case VARTYPE_DOUBLE:
            printf("  \x1b[94;1mtype\x1b[0m = double\n");
            break;
        case VARTYPE_STRUCT:
            printf("  \x1b[94;1mtype\x1b[0m = struct\n");
            break;
        case VARTYPE_UNION:
            printf("  \x1b[94;1mtype\x1b[0m = union\n");
            break;
    }
    printf("  \x1b[94;1mglobal_id\x1b[0m = %d\n", var->global_id);
    if(var->pointers != 0) {
        printf("  \x1b[94;1mpointers\x1b[0m = %d\n", var->pointers);
    }
    if(var->address != -1) {
        printf("  \x1b[94;1maddress\x1b[0m = %d\n", var->address);
    }
    if(var->is_function) {
        printf("  \x1b[94;1mis_function\x1b[0m = true\n", var->is_function);
    }
    if(var->is_argument) {
        printf("  \x1b[94;1mis_argument\x1b[0m = true\n", var->is_argument);
    }
    if(var->is_constant) {
        printf("  \x1b[94;1mis_constant\x1b[0m = true\n", var->is_constant);
    }
    printf("}\n");
}

varlist *create_varlist() {
    varlist *list = malloc(sizeof(varlist));
    list->length = 0;
    list->buffer_length = 16;
    list->list = malloc(16 * sizeof(variable *));
    return list;
}

void delete_varlist(varlist *list) {
    free(list->list);
    free(list);
}

void varlist_add(varlist *list, variable *v) {
    if(list->length == list->buffer_length) {
        #if VARLIST_GROW_EXPONENTIAL
            list->buffer_length <<= 1;
        #else
            list->buffer_length += 16;
        #endif
        list->list = realloc(list->list, list->buffer_length * sizeof(variable *));
    }
    list->list[list->length] = v;
    list->length++;
}

variable *varlist_get(varlist *list, int id) {
    return list->list[id];
}

void print_varlist(varlist *list) {
    int i;
    printf("VARIABLE LIST:\n");
    for(i = 0; i < list->length; i++) {
        print_variable(list->list[i]);
    }
}

toklist *create_toklist() {
    toklist *list = malloc(sizeof(toklist));
    list->length = 0;
    list->buffer_length = 16;
    list->list = malloc(16 * sizeof(token *));
    return list;
}

void delete_toklist(toklist *list) {
    free(list->list);
    free(list);
}

void toklist_add(toklist *list, token *v) {
    if(list->length == list->buffer_length) {
        #if toklist_GROW_EXPONENTIAL
            list->buffer_length <<= 1;
        #else
            list->buffer_length += 16;
        #endif
        list->list = realloc(list->list, list->buffer_length * sizeof(token *));
    }
    list->list[list->length] = v;
    list->length++;
}

token *toklist_get(toklist *list, int id) {
    return list->list[id];
}

opstack_link *create_opstack_link(token *tok) {
    opstack_link *link = malloc(sizeof(opstack_link));
    link->value = tok;
    link->last = NULL;
    link->next = NULL;
    return link;
}

opstack *create_opstack() {
    opstack *stack = malloc(sizeof(opstack));
    stack->start = NULL;
    stack->end = NULL;
    return stack;
}

void delete_opstack(opstack *stack) {
    opstack_link *link = stack->start;
    while(link != NULL) {
        opstack_link *next = link->next;
        free(link);
        link = next;
    }
    free(stack);
}

void opstack_push(opstack *stack, token *tok) {
    opstack_link *link = create_opstack_link(tok);
    if(stack->start == NULL) {
        stack->start = link;
        stack->end = link;
    } else {
        stack->end->next = link;
        link->last = stack->end;
        stack->end = link;
    }
}

token *opstack_peek(opstack *stack) {
    return stack->end->value;
}

token *opstack_pop(opstack *stack) {
    token *out;
    if(stack->start == NULL) {
        return NULL;
    } else if(stack->start == stack->end) {
        out = stack->start->value;
        stack->end = NULL;
        free(stack->start);
        stack->start = NULL;
    } else {
        out = stack->end->value;
        stack->end = stack->end->last;
        free(stack->end->next);
        stack->end->next = NULL;
    }
    return out;
}

int opstack_empty(opstack *stack) {
    return stack->start == NULL;
}

void print_opstack(opstack *stack) {
    opstack_link *item = stack->start;
    int i = 0;
    printf("[");
    while(item != NULL) {
        if(i == 0) {
            printf("%s", token_to_string(item->value));
        } else {
            printf(", %s", token_to_string(item->value));
        }
        item = item->next;
        i++;
    }
    printf("]\n");
}
