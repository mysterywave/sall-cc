#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "datastructs.h"
#include "tokenizer.h"

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
    globals++;
    char *s = format("STRING_%d", globals);
    global_list_add_string(&globals_list, s, string);
    return string_output;
}

char *gen_expression(tree *t, char *output1) {
    char *output = output1;
    if(t->type == TREETYPE_OPERATOR) {
        switch(t->data.tok->type) {
            case TOK_POINTER: {
                variable *var = t->left->data.var;
                char *reg = variable_to_reg(var);
                print_variable(var);
                if(get_variable_size_deref(var, 1) == 1) {
                    print("mov8 [%s] %s\n", reg, output);
                } else {
                    print("mov [%s] %s\n", reg, output);
                }
                return output;
            }
            default: {
                printf("Error: operator '%s' is unsupported in this version of the compiler.\n", get_string_from_toktype(t->data.tok->type));
                exit(1);
            }
        }
    } else if(t->type == TREETYPE_VARIABLE) {
        return variable_to_reg(t->data.var);
    } else if(t->type == TREETYPE_INTEGER) {
        char *s = format("%d", t->data.int_value);
        return s;
    } else if(t->type == TREETYPE_CHAR) {
        char *s = format("%d", t->data.int_value);
        return s;
    } else if(t->type == TREETYPE_STRING) {
        char *s = global_add_string(t->data.string_value);
        return s;
    }
    printf("Error: weird expression: %d\n", t->type);
    exit(1);
}

void gen_assign(tree *t) {
    variable *var = t->left->data.var;
    if(var->is_register) {
        char *output = variable_to_reg(var);
        char *exp = gen_expression(t->right, output);
        if(strcmp(exp, output) != 0) {
            if(get_variable_size(var) == 1) {
                print("mov8 %s %s\n", exp, output);
            } else {
                print("mov %s %s\n", exp, output);
            }
        }
    } else if(var->is_argument) {
        printf("Error: Arguments are all const in this version of the compiler.\n");
        exit(0);
    } else {
        printf("Error: There is currently no support for non-register variables.\n");
        exit(0);
    }
}

void gen_define(tree *t) {
    variable *var = t->left->data.var;
    if(var->is_register) {
        use_register(var);
    } else if(var->is_argument) {
        printf("Error: Arguments are all const in this version of the compiler.\n");
        exit(0);
    } else {
        printf("Error: There is currently no support for non-register variables.\n");
        exit(0);
    }
    if(t->right != NULL) {
        gen_assign(t->right);
    }
}

void gen_call_arg(tree *t) {
    char *exp = gen_expression(t->left, "%oo");
    print("push %s\n", exp);
    if(t->right != NULL) {
        gen_call_arg(t->right);
    }
}

void gen_func_call(tree *t) {
    variable *var = t->left->data.var;
    varlist *args = var->arguments;
    push_used_registers();
    print("push call_return_%d\n", call_returns);
    int i;
    gen_call_arg(t->right);
    print("mov func_%s %%ip\n", var->name);
    print("call_return_%d:\n", call_returns);
    pop_used_registers();
    call_returns++;
}

void gen_while(tree *t) {
    print("while_%d:\n", whiles);
    char *exp = gen_expression(t->left, "%oo");
    print("if %s while_%d_end\n", exp, whiles);
    gen_code(t->right);
    print("mov while_%d %%ip\n", whiles);
    print("while_%d_end:\n", whiles);
    whiles++;
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
        print_noindent(" ");
        _gen_asm(t->right);
    }
}

void gen_asm(tree *t) {
    print("");
    _gen_asm(t);
    print_noindent("\n");
}

void gen_increment(tree *t) {
    variable *var = t->left->data.var;
    char *reg = variable_to_reg(var);
    print("add %s 1\n", reg);
    print("mov %%oo %s\n", reg);
}

void gen_decrement(tree *t) {
    variable *var = t->left->data.var;
    char *reg = variable_to_reg(var);
    print("sub %s 1\n", reg);
    print("mov %%oo %s\n", reg);
}

void gen_statement_list(tree *t) {
    if(t != NULL) {
        gen_code(t->left);
        gen_statement_list(t->right);
    }
}

void gen_block(tree *t) {
    indent_level++;
    scope_level++;
    gen_statement_list(t);
    clear_used_registers_scope();
    scope_level--;
    indent_level--;
}

void gen_code(tree *t) {
    if(t != NULL) {
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
    } else {
        print("peek %d\n", -arg_size - 2);
        print("mov %%oo ip\n");
    }
    print("\n");
    clear_used_registers();
    scope_level--;
}

void gen_declaration(tree *t) {
    if(t->type == TREETYPE_FUNCTION_DEFINITION) {
        gen_function(t);
    }
}

void gen_declaration_list(tree *t) {
    if(t != NULL) {
        gen_declaration(t->left);
        gen_declaration_list(t->right);
    }
}

void gen_start() {
    print("mov stack %%sp\n");
    print("mov func_main %%ip\n");
    print("\n");
}

void gen_end() {
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
