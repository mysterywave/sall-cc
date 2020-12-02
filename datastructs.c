#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "datastructs.h"
#include "tokenizer.h"

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
    out->address = -1;
    return out;
}

int get_variable_size(variable *in) {
    if(in->pointers > 0 || in->is_function || in->type != VARTYPE_CHAR) {
        return 2;
    } else {
        return 1;
    }
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
    //printf("  \x1b[94;1mglobal_id\x1b[0m = %d\n", var->global_id);
    if(var->pointers != 0) {
        printf("  \x1b[94;1mpointers\x1b[0m = %d\n", var->pointers);
    }
    if(var->address != -1) {
        printf("  \x1b[94;1maddress\x1b[0m = %d\n", var->address);
    }
    if(var->is_function) {
        printf("  \x1b[94;1mfunction\x1b[0m\n");
    }
    if(var->is_argument) {
        printf("  \x1b[94;1margument\x1b[0m\n");
    }
    if(var->is_constant) {
        printf("  \x1b[94;1mconstant\x1b[0m\n");
    }
    if(var->is_register) {
        printf("  \x1b[94;1mregister\x1b[0m\n");
    }
    if(var->is_unsigned) {
        printf("  \x1b[94;1munsigned\x1b[0m\n");
    }
    printf("}\n");
}

varlist *create_varlist() {
    varlist *list = malloc(sizeof(varlist));
    list->bytes_size = -1;
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

opstack_link *create_opstack_link(void *value, int prefix_postfix) {
    opstack_link *link = malloc(sizeof(opstack_link));
    link->value = value;
    link->prefix_postfix = prefix_postfix;
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

void opstack_push(opstack *stack, void *value, int prefix_postfix) {
    opstack_link *link = create_opstack_link(value, prefix_postfix);
    if(stack->start == NULL) {
        stack->start = link;
        stack->end = link;
    } else {
        stack->end->next = link;
        link->last = stack->end;
        stack->end = link;
    }
}

opstack_link opstack_peek(opstack *stack) {
    if(stack->end != NULL) {
        return *(stack->end);
    }
    printf("Error: cannot peek stack.\n");
    exit(1);
}

opstack_link opstack_pop(opstack *stack) {
    opstack_link out;
    if(stack->start == NULL) {
        printf("Error: cannot pop from stack.\n");
        exit(1);
    } else if(stack->start == stack->end) {
        out = *(stack->start);
        free(stack->start);
        stack->end = NULL;
        stack->start = NULL;
    } else {
        out = *(stack->end);
        stack->end = stack->end->last;
        free(stack->end->next);
        stack->end->next = NULL;
    }
    return out;
}

opstack_link opstack_peek_fifo(opstack *stack) {
    if(stack->start != NULL) {
        return *(stack->start);
    }
    printf("Error: cannot peek stack.\n");
    exit(1);
}

opstack_link opstack_pop_fifo(opstack *stack) {
    opstack_link out;
    if(stack->start == NULL) {
        printf("Error: cannot pop from stack.\n");
        exit(1);
    } else if(stack->start == stack->end) {
        out = *(stack->start);
        free(stack->start);
        stack->end = NULL;
        stack->start = NULL;
    } else {
        out = *(stack->start);
        stack->start = stack->start->next;
        free(stack->start->last);
        stack->start->last = NULL;
    }
    return out;
}

int opstack_empty(opstack *stack) {
    return stack->start == NULL;
}

void print_opstack_tokens(opstack *stack) {
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

void global_list_init(global_list *list) {
    list->start = NULL;
    list->end = NULL;
}

void global_list_end(global_list *list) {
    global_link *link = list->start;
    while(link) {
        if(link->type == GLOBAL_TYPE_STRING) {
            free(link->value.string_value);
        }
        free(link);
        link = link->next;
    }
    list->start = NULL;
    list->end = NULL;
}

void global_list_add_string(global_list *list, char *name, char *string) {
    global_link *link = malloc(sizeof(global_link));
    link->name = name;
    link->value.string_value = strdup(string);
    link->type = GLOBAL_TYPE_STRING;
    link->next = NULL;
    if(list->start == NULL) {
        list->start = link;
        list->end = link;
    } else {
        list->end->next = link;
        list->end = link;
    }
}
