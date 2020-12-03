#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "datastructs.h"
#include "tokenizer.h"

//#define PARSER_DEBUG

#ifdef PARSER_DEBUG
    #define parser_debug(...) _parser_debug(__VA_ARGS__);
#else
    #define parser_debug(...)
#endif

int indent_level;
FILE *output_file;
char *reg_names[] = {"%r1", "%r2", "%r3", "%r4", "%r5"};
variable *used_regs[] = {NULL, NULL, NULL, NULL, NULL};
const int reg_count = sizeof(used_regs) / sizeof(used_regs[0]);
int scope_level;

void gen_code(tree *t);

int whiles;
int call_returns;

global_list globals_list;
int globals;

#ifdef linux
#include <execinfo.h>

int backtrace_start;

int get_backtrace_level() {
    void *buffer[100];
    return backtrace(buffer, 100);
}
#endif

void _parser_debug(char *format, ...) {
    int ilevel;
    #ifdef linux
        ilevel = get_backtrace_level() - backtrace_start;
    #else
        ilevel = indent_level;
    #endif
    printf("%*s", ilevel * 2, "");
    va_list list;
    va_start(list, format);
    vprintf(format, list);
    va_end(list);
}

void print_noindent(char *format, ...) {
    va_list list;
    va_start(list, format);
    vfprintf(output_file, format, list);
    va_end(list);
}

void print(char *format, ...) {
    fprintf(output_file, "%*s", indent_level * 4, "");
    va_list list;
    va_start(list, format);
    vfprintf(output_file, format, list);
    va_end(list);
}

char *variable_to_reg(variable *var) {
    if(var->is_argument) {
        if(get_variable_size(var) == 1) {
            print("peek8 %d\n", var->address);
        } else {
            print("peek %d\n", var->address);
        }
        return "%oo";
    }
    int i;
    for(i = 0; i < reg_count; i++) {
        if(used_regs[i] == var) {
            return reg_names[i];
        }
    }
    printf("Error: Cannot find variable: %s\n", var->name);
    exit(1);
}

void use_register(variable *var) {
    int i;
    for(i = 0; i < reg_count; i++) {
        if(used_regs[i] == NULL) {
            used_regs[i] = var;
            return;
        }
    }
    printf("Error: There's not enough registers to hold variable: %s\n", var->name);
    exit(1);
}

void clear_used_registers_scope() {
    int i;
    for(i = 0; i < reg_count; i++) {
        if(used_regs[i] != NULL && used_regs[i]->scope_level == scope_level) {
            used_regs[i] = NULL;
        }
    }
}

void clear_used_registers() {
    int i;
    for(i = 0; i < reg_count; i++) {
        used_regs[i] = NULL;
    }
}

void push_used_registers() {
    int i;
    for(i = 0; i < reg_count; i++) {
        if(used_regs[i] != NULL) {
            print("push %s\n", reg_names[i]);
        }
    }
}

void pop_used_registers() {
    int i;
    for(i = reg_count - 1; i >= 0; i--) {
        if(used_regs[i] != NULL) {
            print("pop %s\n", reg_names[i]);
        }
    }
}

char string_output[16];

// only use this for small output strings
char *format(char *format, ...) {
    va_list list;
    va_start(list, format);
    vsnprintf(string_output, sizeof(string_output), format, list);
    va_end(list);
    return string_output;
}

char *global_add_string(char *string) {
    char *s = format("STRING_%d", globals);
    globals++;
    global_list_add_string(&globals_list, s, string);
    return string_output;
}

typedef struct type {
    char *value;
    int pointers;
    int base_size;
} type;

int get_size(type t) {
    if(t.pointers > 0) {
        return 2;
    } else {
        return t.base_size;
    }
}

type gen_expression(tree *t, char *output1, int expected_size) {
    parser_debug("gen_expression()\n");
    type output = {NULL, -1, -1};
    output.value = output1;
    if(t->type == TREETYPE_OPERATOR) {
        switch(t->data.tok->type) {
            case TOK_POINTER: {
                type out = gen_expression(t->left, "%oo", -1);
                if(out.pointers > 0) {
                    output.pointers = out.pointers - 1;
                    output.base_size = out.base_size;
                } else {
                    printf("Warning: Dereferencing a non-pointer type.\n", get_string_from_toktype(t->data.tok->type));
                }
                if(expected_size == 1) {
                    print("mov8 [%s] %s\n", out.value, output1);
                } else {
                    print("mov [%s] %s\n", out.value, output1);
                }
                break;
            }
            default: {
                printf("Error: operator '%s' is unsupported in this version of the compiler.\n", get_string_from_toktype(t->data.tok->type));
                exit(1);
            }
        }
    } else if(t->type == TREETYPE_VARIABLE) {
        variable *var = t->data.var;
        output.base_size = get_variable_size_nopointer(var);
        output.value = variable_to_reg(var);
        output.pointers = var->pointers;
    } else if(t->type == TREETYPE_INTEGER) {
        output.value = format("%d", t->data.int_value);
        output.pointers = 0;
        output.base_size = 2;
    } else if(t->type == TREETYPE_CHAR) {
        output.value = format("%d", t->data.int_value);
        output.pointers = 0;
        output.base_size = 1;
    } else if(t->type == TREETYPE_STRING) {
        output.value = global_add_string(t->data.string_value);
        output.pointers = 1;
        output.base_size = 1;
    } else {
        printf("Error: weird expression: %d\n", t->type);
        exit(1);
    }
    if(expected_size != -1 && get_size(output) > expected_size) {
        printf("Error: Loss of precision\n");
        exit(1);
    }
    return output;
}

void gen_assign(tree *t) {
    parser_debug("gen_assign()\n");
    variable *var = t->left->data.var;
    if(var->is_register) {
        char *output = variable_to_reg(var);
        type exp = gen_expression(t->right, output, get_variable_size(var));
        if(strcmp(exp.value, output) != 0) {
            if(get_variable_size(var) == 1) {
                print("mov8 %s %s\n", exp.value, output);
            } else {
                print("mov %s %s\n", exp.value, output);
            }
        }
    } else if(var->is_argument) {
        printf("Error: Arguments are all const in this version of the compiler.\n");
        exit(1);
    } else {
        printf("Error: There is currently no support for non-register variables.\n");
        exit(1);
    }
}

void gen_define(tree *t) {
    parser_debug("gen_define()\n");
    variable *var = t->left->data.var;
    if(var->is_register) {
        use_register(var);
    } else if(var->is_argument) {
        printf("Error: Arguments are all const in this version of the compiler.\n");
        exit(1);
    } else {
        printf("Error: There is currently no support for non-register variables.\n");
        exit(1);
    }
    if(t->right != NULL) {
        gen_assign(t->right);
    }
}

void gen_func_call(tree *t) {
    parser_debug("gen_func_call()\n");
    variable *var = t->left->data.var;
    varlist *args = var->arguments;
    push_used_registers();
    print("push call_return_%d\n", call_returns);
    tree *arg = t->right;
    int i = 0;
    while(arg != NULL) {
        if(i >= args->length) {
            printf("Error: too many arguments supplied for function \"%s\"", var->name);
            exit(1);
        }
        type exp = gen_expression(arg->left, "%oo", get_variable_size(args->list[i]));
        print("push %s\n", exp.value);
        arg = arg->right;
        i++;
    }
    print("mov func_%s %%ip\n", var->name);
    print("call_return_%d:\n", call_returns);
    pop_used_registers();
    int arg_size = 0;
    for(i = args->length - 1; i >= 0; i--) {
        arg_size += get_variable_size(args->list[i]);
    }
    print("sub %%sp %d\n", arg_size + 2);
    print("mov %%oo %%sp\n");
    call_returns++;
}

void gen_while(tree *t) {
    parser_debug("gen_while()\n");
    if(t->left->type == TREETYPE_INTEGER) {
        if(t->left->data.int_value != 0) {
            print("while_%d:\n", whiles);
            print("mov while_%d %%ip\n", whiles);
            whiles++;
        }
    } else {
        print("while_%d:\n", whiles);
        type exp = gen_expression(t->left, "%oo", 1);
        print("if %s while_%d_end\n", exp.value, whiles);
        gen_code(t->right);
        print("mov while_%d %%ip\n", whiles);
        print("while_%d_end:\n", whiles);
        whiles++;
    }
}

void _gen_asm(tree *t) {
    switch(t->left->type) {
        case TREETYPE_INTEGER: {
            print_noindent("%d", t->left->data.int_value);
            break;
        }
        case TREETYPE_CHAR: {
            print_noindent("%d", t->left->data.int_value);
            break;
        }
        case TREETYPE_IDENTIFIER: {
            print_noindent("%s", t->left->data.string_value);
            break;
        }
        case TREETYPE_VARIABLE: {
            char *reg = variable_to_reg(t->left->data.var);
            print_noindent("%s", reg);
            break;
        }
    }
    if(t->right != NULL) {
        if(strcmp(t->left->data.string_value, "%") != 0) {
            print_noindent(" ");
        }
        _gen_asm(t->right);
    }
}

void gen_asm(tree *t) {
    parser_debug("gen_asm()\n");
    print("");
    _gen_asm(t);
    print_noindent("\n");
}

void gen_increment(tree *t) {
    parser_debug("gen_increment()\n");
    variable *var = t->left->data.var;
    char *reg = variable_to_reg(var);
    print("add %s 1\n", reg);
    print("mov %%oo %s\n", reg);
}

void gen_decrement(tree *t) {
    parser_debug("gen_decrement()\n");
    variable *var = t->left->data.var;
    char *reg = variable_to_reg(var);
    print("sub %s 1\n", reg);
    print("mov %%oo %s\n", reg);
}

void gen_statement_list(tree *t) {
    if(t != NULL) {
        parser_debug("gen_statement_list()\n");
        gen_code(t->left);
        gen_statement_list(t->right);
    }
}

void gen_block(tree *t) {
    parser_debug("gen_block()\n");
    indent_level++;
    scope_level++;
    gen_statement_list(t);
    clear_used_registers_scope();
    scope_level--;
    indent_level--;
}

void gen_code(tree *t) {
    if(t != NULL) {
        parser_debug("gen_code()\n");
        if(t->type == TREETYPE_DEFINE) {
            gen_define(t);
        } else if(t->type == TREETYPE_ASSIGN) {
            gen_assign(t);
        } else if(t->type == TREETYPE_FUNC_CALL) {
            gen_func_call(t);
        } else if(t->type == TREETYPE_WHILE) {
            gen_while(t);
        } else if(t->type == TREETYPE_ASM) {
            gen_asm(t);
        } else if(t->type == TREETYPE_OPERATOR) {
            if(t->data.tok->type == TOK_INCREMENT) {
                gen_increment(t);
            } else if(t->data.tok->type == TOK_DECREMENT) {
                gen_decrement(t);
            }
        } else if(t->type == TREETYPE_BLOCK) {
            gen_block(t);
        } else if(t->type == TREETYPE_STATEMENT_LIST) {
            gen_statement_list(t);
        }
    }
}

void gen_function(tree *t) {
    parser_debug("gen_function()\n");
    scope_level++;
    variable *var = t->left->data.var;
    print("func_%s:\n", var->name);
    varlist *arguments = var->arguments;
    int i;
    int arg_size = 0;
    int id = 0;
    for(i = arguments->length - 1; i >= 0; i--) {
        int size = get_variable_size(arguments->list[i]);
        id -= size;
        arguments->list[i]->address = id;
        arg_size += size;
    }
    gen_code(t->right);
    print("return_%s:\n", var->name);
    if(strcmp(var->name, "main") == 0) {
        print("end\n");
    } else if(strcmp(var->name, "interrupt") == 0) {
        //print("end\n");
        print("pop %%r5\n");
        print("pop %%r4\n");
        print("pop %%r3\n");
        print("pop %%r2\n");
        print("pop %%r1\n");
        print("pop %%oo\n");
        print("pop %%ip\n");
    } else {
        print("peek %d\n", -arg_size - 2);
        print("mov %%oo %%ip\n");
    }
    print("\n");
    clear_used_registers();
    scope_level--;
}

void gen_declaration(tree *t) {
    parser_debug("gen_declaration()\n");
    if(t->type == TREETYPE_FUNCTION_DEFINITION) {
        gen_function(t);
    }
}

void gen_declaration_list(tree *t) {
    if(t != NULL) {
        parser_debug("gen_declaration_list()\n");
        gen_declaration(t->left);
        gen_declaration_list(t->right);
    }
}

void gen_start() {
    parser_debug("gen_start()\n");
    print("mov stack %%sp\n");
    print("mov func_main %%ip\n");
    print("func_interrupt\n");
    print("\n");
}

void gen_end() {
    parser_debug("gen_end()\n");
    global_link *link = globals_list.start;
    while(link) {
        if(link->type == GLOBAL_TYPE_STRING) {
            print("%s: \"%s\"\n", link->name, link->value.string_value);
        }
        link = link->next;
    }
    if(globals_list.start != NULL) {
        print("\n");
    }
    print("stack:\n");
}

// AST should be a valid program tree since it's generated by
// the parser, so there's no need to check for errors in it 
void generate(FILE *output, tree *AST) {
    #ifdef linux
        backtrace_start = get_backtrace_level() + 1;
    #endif
    parser_debug("generate()\n");
    whiles = 0;
    call_returns = 0;
    globals = 0;
    
    indent_level = 0;
    int i;
    for(i = 0; i < reg_count; i++) {
        used_regs[i] = 0;
    }
    output_file = output;
    global_list_init(&globals_list);
    gen_start();
    gen_declaration_list(AST);
    for(i = 0; i < reg_count; i++) {
        if(used_regs[i] != 0) {
            printf("STILL USED: %d\n", i);
        }
    }
    gen_end();
    global_list_end(&globals_list);
}
