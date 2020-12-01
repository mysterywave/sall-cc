#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "parser.h"
#include "tokenizer.h"
#include "optimizer.h"

//#define PARSER_DEBUG
#define PRINT_AST

#define expect(...) _expect_1((sizeof((int[]){__VA_ARGS__})/sizeof(int)), __VA_ARGS__)
#define expect_peek(...) _expect_peek((sizeof((int[]){__VA_ARGS__})/sizeof(int)), __VA_ARGS__)

#ifdef PARSER_DEBUG
    #define macro_to_string_1(x) #x
    #define macro_to_string(x) macro_to_string_1(x)
    #define debug(...) do{printf("\x1b[93;1mparser.c:" macro_to_string(__LINE__) "\x1b[0m:\n");_debug(__VA_ARGS__);}while(0)
    #define error(...) do{printf("\x1b[93;1mparser.c:" macro_to_string(__LINE__) "\x1b[0m:\n");_error(__VA_ARGS__);}while(0)
#else
    #define debug(...)
    #define error(...) _error(__VA_ARGS__)
#endif

extern string_builder current_line;
extern char *filename;
extern int line_num;

varlist *all_vars;
varlist *local_vars;
int current_scope;

variable *parse_type();
void parse_arg_definition(varlist *arglist);
tree *parse_expression();
tree *parse_while();
tree *parse_if();
tree *parse_for();
tree *parse_switch();
tree *parse_code();
tree *parse_declaration();
tree *parse_declaration_list();
tree *parse();

// for stack trace printing in GDB
void cause_segfault() {
    int *i = 0;
    *i = 0;
}

extern toklist *tokens_to_free;

void _debug(int i, char *format, ...) {
    token *tok;
    // <hacky>
    if(i || tokens_to_free->length == 0) {
        tok = tokenizer_peek();
    } else {
        tok = tokens_to_free->list[tokens_to_free->length - 1];
    }
    // </hacky>
    printf("\x1b[1m%s:%d:12:\x1b[0m ", filename, line_num, tok->line_offset - 1);
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    if(tok->line_offset != -1) {
        char *line = string_builder_get(&current_line);
        int pos = tok->line_offset - 1;
        int len = tok->length;
        printf("%.*s", pos, line);
        printf("\x1b[92;1m%.*s\x1b[0m", tok->length, line + pos);
        printf("%.*s\n", current_line.length - (pos + len), line + pos + len);
        printf("\x1b[92;1m%*s^\x1b[0m\n\n", tok->line_offset - 1, "");
    }
}

void _error(token *tok, char *format, ...) {
    printf("\x1b[1m%s:%d:12:\x1b[0m \x1b[91;1merror:\x1b[0m ", filename, line_num, tok->line_offset - 1);
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    if(tok->line_offset != -1) {
        char *line = string_builder_get(&current_line);
        int pos = tok->line_offset - 1;
        int len = tok->length;
        printf("%.*s", pos, line);
        printf("\x1b[91;1m%.*s\x1b[0m", tok->length, line + pos);
        printf("%.*s\n", current_line.length - (pos + len), line + pos + len);
        printf("\x1b[91;1m%*s^\x1b[0m\n", tok->line_offset - 1, "");
    }
    //cause_segfault();
    exit(1);
}

void add_variable(variable *var) {
    //var->global_id = all_vars->length;
    var->scope_level = current_scope;
    varlist_add(all_vars, var);
    varlist_add(local_vars, var);
}

// tok should always be an identifier
variable *get_variable_noerror(token *tok) {
    char *s = tok->string;
    int i;
    for(i = local_vars->length - 1; i >= 0; i--) {
        if(strcmp(s, local_vars->list[i]->name) == 0) {
            return local_vars->list[i];
        }
    }
    return NULL;
}

variable *get_variable(token *tok) {
    variable *out = get_variable_noerror(tok);
    if(out == NULL) {
        error(tok, "Unknown identifier\n");
    }
    return out;
}

token *_expect(token *tok, int n, va_list list) {
    int i;
    for(i = 0; i < n; i++) {
        if(tok->type == va_arg(list, int)) {
            return tok;
        }
    }
    
    /*printf("Expecting ");
    va_list ap2;
    va_start(ap2, n);
    for(i = 0; i < n; i++) {
        int j = va_arg(ap2, int);
        if(i == n - 1) {
            printf(", or \"%s\"\n", get_string_from_toktype(j));
        } else if(i == 0) {
            printf("\"%s\"", get_string_from_toktype(j));
        } else {
            printf(", \"%s\"", get_string_from_toktype(j));
        }
    }
    va_end(ap2);*/
    
    error(tok, "Unexpected token\n");
}

token *_expect_1(int n, ...) {
    token *tok = tokenizer_get();
    va_list list;
    va_start(list, n);
    _expect(tok, n, list);
    va_end(list);
}

token *_expect_peek(int n, ...) {
    token *tok = tokenizer_peek();
    va_list list;
    va_start(list, n);
    _expect(tok, n, list);
    va_end(list);
}

variable *parse_type() {
    debug(0, "parse_type()\n");
    variable *output = create_variable();
    token *tok = tokenizer_peek();
    while(
        tok->type == TOK_UNSIGNED ||
        tok->type == TOK_CONST ||
        tok->type == TOK_REGISTER
    ) {
        tokenizer_get();
        switch(tok->type) {
            case TOK_UNSIGNED:
                output->is_unsigned = 1;
                break;
            case TOK_CONST:
                output->is_constant = 1;
                break;
            case TOK_REGISTER:
                output->is_register = 1;
                break;
        }
        tok = tokenizer_peek();
    }
    token *tok1 = expect(TOK_VOID, TOK_CHAR, TOK_INT, TOK_LONG, TOK_FLOAT, TOK_DOUBLE);
    switch(tok1->type) {
        case TOK_VOID:
            output->type = VARTYPE_VOID;
            break;
        case TOK_CHAR:
            output->type = VARTYPE_CHAR;
            break;
        case TOK_INT:
            output->type = VARTYPE_INT;
            break;
        case TOK_LONG:
            output->type = VARTYPE_LONG;
            break;
        case TOK_FLOAT:
            output->type = VARTYPE_FLOAT;
            break;
        case TOK_DOUBLE:
            output->type = VARTYPE_DOUBLE;
            break;
        default:
            error(tok1, "Unknown type\n");
    }
    token *tok2 = tokenizer_peek();
    while(tok2->type == TOK_STAR) {
        tokenizer_get();
        output->pointers++;
        tok2 = tokenizer_peek();
    }
    return output;
}

void parse_arg_definition(varlist *arglist) {
    debug(0, "parse_arg_definition()\n");
    variable *arg = parse_type();
    token *name = expect(TOK_IDENTIFIER);
    arg->name = name->string;
    arg->is_argument = 1;
    add_variable(arg);
    varlist_add(arglist, arg);
    token *next = expect(TOK_RPAREN, TOK_COMMA);
    if(next->type == TOK_RPAREN) {
        return;
    } else {
        parse_arg_definition(arglist);
    }
}

int precedence(token *tok, int prefix_postfix) {
    switch(tok->type) {
        case TOK_LPAREN:
        case TOK_RPAREN:
        case TOK_LSQUARE:
        case TOK_RSQUARE:
        case TOK_DOT:
        case TOK_ARROW:
            return 1;
        case TOK_NOT:
        case TOK_BITWISE_NOT:
        case TOK_SIZEOF:
            return 2;
        case TOK_DIVIDE:
        case TOK_MOD:
            return 3;
        case TOK_LSH:
        case TOK_RSH:
            return 5;
        case TOK_LESS:
        case TOK_MORE:
        case TOK_LESS_EQUAL:
        case TOK_MORE_EQUAL:
            return 6;
        case TOK_EQUAL_TO:
        case TOK_NOT_EQUAL:
            return 7;
        case TOK_XOR:
            return 9;
        case TOK_BITWISE_OR:
            return 10;
        case TOK_AND:
            return 11;
        case TOK_OR:
            return 12;
        case TOK_QUESTION_MARK:
        case TOK_COLON:
            return 13;
        case TOK_EQUAL:
        case TOK_PLUS_EQUAL:
        case TOK_MINUS_EQUAL:
        case TOK_TIMES_EQUAL:
        case TOK_DIVIDE_EQUAL:
        case TOK_MOD_EQUAL:
        case TOK_AND_EQUAL:
        case TOK_OR_EQUAL:
        case TOK_XOR_EQUAL:
        case TOK_LSHIFT_EQUAL:
        case TOK_RSHIFT_EQUAL:
            return 14;
        case TOK_COMMA:
            return 15;
        case TOK_POSTFIX_INCREMENT:
        case TOK_POSTFIX_DECREMENT:
            return 1;
        case TOK_PREFIX_INCREMENT:
        case TOK_PREFIX_DECREMENT:
            return 2;
        case TOK_MULTIPLY:
            return 3;
        case TOK_POINTER:
            return 2;
        case TOK_BITWISE_AND:
            return 8;
        case TOK_ADDRESS:
            return 2;
        case TOK_POSITIVE:
            return 4;
        case TOK_ADD:
            return 2;
        case TOK_NEGATIVE:
            return 4;
        case TOK_SUBTRACT:
            return 2;
        /*case TOK_INCREMENT:
        case TOK_DECREMENT:
            if(prefix_postfix == -1) {
                return 1;
            } else {
                return 2;
            }
        case TOK_STAR:
            if(prefix_postfix == 0) {
                return 3;
            } else {
                return 2;
            }
        case TOK_AMPERSAND:
            if(prefix_postfix == 0) {
                return 8;
            } else {
                return 2;
            }
        case TOK_PLUS:
        case TOK_MINUS:
            if(prefix_postfix == 0) {
                return 4;
            } else {
                return 2;
            }*/
    }
    // should never get here
    error(tok, "Strange operator: %s\n", get_string_from_toktype(tok->type));
}

int is_left_associative(token *tok, int prefix_postfix) {
    switch(tok->type) {
        case TOK_LPAREN:
        case TOK_RPAREN:
        case TOK_LSQUARE:
        case TOK_RSQUARE:
        case TOK_DOT:
        case TOK_ARROW:
            return 1;
        case TOK_NOT:
        case TOK_BITWISE_NOT:
        case TOK_SIZEOF:
            return 0;
        case TOK_DIVIDE:
        case TOK_MOD:
        case TOK_LSH:
        case TOK_RSH:
        case TOK_LESS:
        case TOK_MORE:
        case TOK_LESS_EQUAL:
        case TOK_MORE_EQUAL:
        case TOK_EQUAL_TO:
        case TOK_NOT_EQUAL:
        case TOK_XOR:
        case TOK_BITWISE_OR:
        case TOK_AND:
        case TOK_OR:
            return 1;
        case TOK_QUESTION_MARK:
        case TOK_COLON:
        case TOK_EQUAL:
        case TOK_PLUS_EQUAL:
        case TOK_MINUS_EQUAL:
        case TOK_TIMES_EQUAL:
        case TOK_DIVIDE_EQUAL:
        case TOK_MOD_EQUAL:
        case TOK_AND_EQUAL:
        case TOK_OR_EQUAL:
        case TOK_XOR_EQUAL:
        case TOK_LSHIFT_EQUAL:
        case TOK_RSHIFT_EQUAL:
            return 0;
        case TOK_COMMA:
            return 1;
        case TOK_POSTFIX_INCREMENT:
        case TOK_POSTFIX_DECREMENT:
            return 1;
        case TOK_PREFIX_INCREMENT:
        case TOK_PREFIX_DECREMENT:
            return 0;
        case TOK_MULTIPLY:
            return 1;
        case TOK_POINTER:
            return 0;
        case TOK_BITWISE_AND:
            return 1;
        case TOK_ADDRESS:
            return 0;
        case TOK_POSITIVE:
            return 1;
        case TOK_ADD:
            return 0;
        case TOK_NEGATIVE:
            return 1;
        case TOK_SUBTRACT:
            return 0;
        /*case TOK_INCREMENT:
        case TOK_DECREMENT:
            if(prefix_postfix == -1) {
                return 1;
            } else {
                return 0;
            }
        case TOK_STAR:
            if(prefix_postfix == 0) {
                return 1;
            } else {
                return 0;
            }
        case TOK_AMPERSAND:
            if(prefix_postfix == 0) {
                return 1;
            } else {
                return 0;
            }
        case TOK_PLUS:
        case TOK_MINUS:
            if(prefix_postfix == 0) {
                return 1;
            } else {
                return 0;
            }*/
    }
    // should never get here
    error(tok, "Strange operator: %s\n", get_string_from_toktype(tok->type));
}

tree *parse_reverse_polish(opstack *stack) {
    if(stack->start == NULL) {
        error(tokenizer_peek(), "Bad expression\n");
    }
    token *first = opstack_peek_fifo(stack).value;
    opstack *tmpstack = create_opstack();
    while(!opstack_empty(stack)) {
        token *tok = (token *)opstack_pop_fifo(stack).value;
        if(is_operator(tok->type)) {
            if(is_single_argument(tok->type)) {
                tree *t = create_tree(TREETYPE_OPERATOR);
                t->data.tok = tok;
                t->left = opstack_pop(tmpstack).value;
                opstack_push(tmpstack, t, 0);
            } else {
                tree *t = create_tree(TREETYPE_OPERATOR);
                t->data.tok = tok;
                t->right = opstack_pop(tmpstack).value;
                t->left = opstack_pop(tmpstack).value;
                opstack_push(tmpstack, t, 0);
            }
        } else if(tok->type == TOK_INT_CONST) {
            tree *t = create_tree(TREETYPE_INTEGER);
            t->data.int_value = tok->value;
            opstack_push(tmpstack, t, 0);
        } else if(tok->type == TOK_CHAR_CONST) {
            tree *t = create_tree(TREETYPE_CHAR);
            t->data.int_value = tok->value;
            opstack_push(tmpstack, t, 0);
        } else if(tok->type == TOK_STRING_CONST) {
            tree *t = create_tree(TREETYPE_STRING);
            t->data.string_value = strdup(tok->string);
            opstack_push(tmpstack, t, 0);
        } else if(tok->type == TOK_IDENTIFIER) {
            tree *t = create_tree(TREETYPE_VARIABLE);
            t->data.var = get_variable(tok);
            opstack_push(tmpstack, t, 0);
        } else {
            error(tok, "Weird value: %d\n", tok->type);
        }
    }
    if(tmpstack->start != NULL && tmpstack->start == tmpstack->end) {
        tree *t = (tree *)opstack_pop(tmpstack).value;
        delete_opstack(tmpstack);
        return t;
    }
    error(first, "Bad expression\n");
}

void opmod(token *tok, int prefix_postfix) {
    switch(tok->type) {
        case TOK_STAR:
            if(prefix_postfix == 0) {
                tok->type = TOK_MULTIPLY;
            } else if(prefix_postfix == 1) {
                tok->type = TOK_POINTER;
            } else {
                error(tok, "Error parsing token\n");
            }
            break;
        case TOK_AMPERSAND:
            if(prefix_postfix == 0) {
                tok->type = TOK_BITWISE_AND;
            } else if(prefix_postfix == 1) {
                tok->type = TOK_ADDRESS;
            } else {
                error(tok, "Error parsing token\n");
            }
            break;
        case TOK_PLUS:
            if(prefix_postfix == 0) {
                tok->type = TOK_ADD;
            } else if(prefix_postfix == 1) {
                tok->type = TOK_POSITIVE;
            } else {
                error(tok, "Error parsing token\n");
            }
            break;
        case TOK_MINUS:
            if(prefix_postfix == 0) {
                tok->type = TOK_SUBTRACT;
            } else if(prefix_postfix == 1) {
                tok->type = TOK_NEGATIVE;
            } else {
                error(tok, "Error parsing token\n");
            }
            break;
        case TOK_INCREMENT:
            if(prefix_postfix == -1) {
                tok->type = TOK_POSTFIX_INCREMENT;
            } else if(prefix_postfix == 1) {
                tok->type = TOK_PREFIX_INCREMENT;
            } else {
                error(tok, "Error parsing token\n");
            }
            break;
        case TOK_DECREMENT:
            if(prefix_postfix == -1) {
                tok->type = TOK_POSTFIX_DECREMENT;
            } else if(prefix_postfix == 1) {
                tok->type = TOK_PREFIX_DECREMENT;
            } else {
                error(tok, "Error parsing token\n");
            }
            break;
    }
}

// TODO: actually get this working correctly...
tree *parse_expression2(int end_type, int end_type2) {
    debug(0, "parse_expression()\n");
    // shunting yard
    
    opstack *out = create_opstack();
    opstack *tmp = create_opstack();
    
    int prefix_postfix = 1;
    
    token *tok = tokenizer_peek();
    while(tok->type != TOK_EOF && tok->type != end_type && tok->type != end_type2) {
        tok = expect(
            TOK_INT_CONST, TOK_LONG_CONST, TOK_CHAR_CONST, TOK_STRING_CONST, TOK_FLOAT_CONST, TOK_DOUBLE_CONST,
            TOK_IDENTIFIER, TOK_EQUAL, TOK_PLUS_EQUAL, TOK_MINUS_EQUAL, TOK_TIMES_EQUAL, TOK_DIVIDE_EQUAL,
            TOK_MOD_EQUAL, TOK_AND_EQUAL, TOK_OR_EQUAL, TOK_XOR_EQUAL, TOK_LSHIFT_EQUAL, TOK_RSHIFT_EQUAL,
            TOK_INCREMENT, TOK_DECREMENT, TOK_DIVIDE, TOK_MOD, TOK_BITWISE_NOT, TOK_BITWISE_OR, TOK_XOR,
            TOK_LSH, TOK_RSH, TOK_NOT, TOK_AND, TOK_OR, TOK_EQUAL_TO, TOK_NOT_EQUAL, TOK_LESS, TOK_MORE,
            TOK_LESS_EQUAL, TOK_MORE_EQUAL, TOK_ARROW, TOK_DOT, TOK_COMMA, TOK_QUESTION_MARK,TOK_COLON,
            TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_AMPERSAND, TOK_SIZEOF, TOK_LPAREN, TOK_RPAREN,TOK_LSQUARE,
            TOK_RSQUARE
        );
        if(
            tok->type == TOK_INT_CONST || tok->type == TOK_LONG_CONST || tok->type == TOK_CHAR_CONST ||
            tok->type == TOK_STRING_CONST || tok->type == TOK_FLOAT_CONST || tok->type == TOK_DOUBLE_CONST ||
            tok->type == TOK_IDENTIFIER
        ) {
            opstack_push(out, tok, 0);
            token *peeked = tokenizer_peek();
            if(peeked->type == TOK_INCREMENT || peeked->type == TOK_DECREMENT) {
                prefix_postfix = -1;
            } else {
                prefix_postfix = 0;
            }
        } else if(tok->type == TOK_LPAREN) {
            opstack_push(tmp, tok, prefix_postfix);
        } else if(tok->type == TOK_RPAREN) {
            while(!opstack_empty(tmp) && ((token *)opstack_peek(tmp).value)->type != TOK_LPAREN) {
                opstack_link popped = opstack_pop(tmp);
                opstack_push(out, popped.value, popped.prefix_postfix);
            }
            if(opstack_empty(out)) {
                error(tok, "Mismatched parenthesis");
            }
            opstack_pop(tmp);
        } else {
            if(prefix_postfix > 1) {
                error(tok, "Error parsing token\n");
            }
            opmod(tok, prefix_postfix);
            while(
                !opstack_empty(tmp) &&
                ((token *)opstack_peek(tmp).value)->type != TOK_LPAREN && (
                    precedence(opstack_peek(tmp).value, opstack_peek(tmp).prefix_postfix) < precedence(tok, prefix_postfix) || (
                        precedence(opstack_peek(tmp).value, opstack_peek(tmp).prefix_postfix) == precedence(tok, prefix_postfix) &&
                        is_left_associative(tok, prefix_postfix)
                    )
                )
            ) {
                opstack_push(out, opstack_pop(tmp).value, 0);
            }
            opstack_push(tmp, tok, prefix_postfix);
            prefix_postfix++;
        }
        tok = tokenizer_peek();
    }
    if(tok->type == TOK_EOF) {
        error(tokenizer_peek(), "Reached EOF while parsing expression\n");
    }
    while(!opstack_empty(tmp)) {
        opstack_push(out, opstack_pop(tmp).value, 0);
    }
    //tokenizer_get();
    #ifdef PARSER_DEBUG
        printf("Reverse Polish: ");
        print_opstack_tokens(out);
    #endif
    tree *expression = parse_reverse_polish(out);
    return optimize_expression(expression);
}

tree *parse_expression(int end_type) {
    tree *output = parse_expression2(end_type, 0);
    // hacky
    tokenizer_get();
    return output;
}

tree *parse_while() {
    debug(0, "parse_while()\n");
    tree *out = create_tree(TREETYPE_WHILE);
    expect(TOK_LPAREN);
    out->left = parse_expression(TOK_RPAREN);
    out->right = parse_code();
    return out;
}

tree *parse_if() {
    debug(0, "parse_if()\n");
    tree *out = create_tree(TREETYPE_IF);
    expect(TOK_LPAREN);
    out->left = parse_expression(TOK_RPAREN);
    out->right = parse_code();
    return out;
}

tree *parse_for() {
    debug(0, "parse_for()\n");
    tree *out = NULL;
    return out;
}

tree *parse_switch() {
    debug(0, "parse_switch()\n");
    tree *out = NULL;
    return out;
}

// TODO
tree *parse_asm(token *asmtok) {
    debug(0, "parse_asm()\n");
    if(tokenizer_peek()->type == TOK_SEMICOLON) {
        return NULL;
    }
    tree *out = create_tree(TREETYPE_ASM);
    tree *tmp = out;
    while(1) {
        token *tok = tokenizer_get();
        tree *t;
        if(tok->type == TOK_INT_CONST) {
            t = create_tree(TREETYPE_INTEGER);
            t->data.int_value = tok->value;
        } else if(tok->type == TOK_IDENTIFIER) {
            variable *var = get_variable_noerror(tok);
            if(var == NULL) {
                t = create_tree(TREETYPE_IDENTIFIER);
                t->data.string_value = strdup(tok->string);
            } else {
                t = create_tree(TREETYPE_VARIABLE);
                t->data.var = var;
            }
        } else {
            error(tok, "Unexpected token\n");
        }
        tmp->left = t;
        if(tokenizer_peek()->type == TOK_SEMICOLON) {
            break;
        }
        if(tokenizer_peek()->type == TOK_EOF) {
            error(asmtok, "Reached EOF while parsing asm\n");
        }
        tree *new = create_tree(TREETYPE_ASM);
        tmp->right = new;
        tmp = tmp->right;
    }
    tokenizer_get(); // ;
    return out;
}

tree *parse_code() {
    debug(1, "parse_code()\n");
    tree *out = NULL;
    int type = tokenizer_peek()->type;
    /*token *next = expect_peek(
        TOK_WHILE, TOK_IF, TOK_FOR,
        TOK_IDENTIFIER,
        TOK_LBRACE,
        TOK_ASM,
        TOK_SEMICOLON,
        TOK_VOID, TOK_CHAR, TOK_INT, TOK_LONG, TOK_FLOAT, TOK_DOUBLE, TOK_STRUCT, TOK_UNION
    );*/
    token *next = tokenizer_peek();
    if(next->type == TOK_WHILE) {
        tokenizer_get();
        out = parse_while();
    } else if(next->type == TOK_IF) {
        tokenizer_get();
        out = parse_if();
    } else if(next->type == TOK_FOR) {
        tokenizer_get();
        out = parse_for();
    } else if(next->type == TOK_SWITCH) {
        tokenizer_get();
        out = parse_switch();
    } else if(next->type == TOK_IDENTIFIER) {
        tokenizer_get();
        variable *var = get_variable(next);
        token *next2 = expect(
            TOK_LPAREN,
            TOK_INCREMENT, TOK_DECREMENT,
            TOK_EQUAL, TOK_PLUS_EQUAL, TOK_MINUS_EQUAL, TOK_TIMES_EQUAL, TOK_DIVIDE_EQUAL,
            TOK_MOD_EQUAL, TOK_AND_EQUAL, TOK_OR_EQUAL, TOK_XOR_EQUAL, TOK_LSHIFT_EQUAL,
            TOK_RSHIFT_EQUAL
        );
        if(next2->type == TOK_LPAREN) {
            out = create_tree(TREETYPE_FUNC_CALL);
            out->left = create_tree(TREETYPE_VARIABLE);
            out->left->data.var = var;
            tree *tmp = out;
            if(tokenizer_peek()->type != TOK_RPAREN) {
                token *next3;
                while(1) {
                    tree *expression = parse_expression2(TOK_COMMA, TOK_RPAREN);
                    tmp->right = create_tree(TREETYPE_ARG_LIST);
                    tmp = tmp->right;
                    tmp->left = expression;
                    next3 = tokenizer_get();
                    if(next3->type == TOK_RPAREN || next3->type == TOK_EOF) {
                        break;
                    }
                }
                if(next3->type == TOK_EOF) {
                    error(next2, "Reached EOF while parsing function arguments\n");
                }
            }
            expect(TOK_SEMICOLON);
        } else if(next2->type == TOK_INCREMENT) {
            out = create_tree(TREETYPE_OPERATOR);
            out->data.tok = next2;
            out->left = create_tree(TREETYPE_VARIABLE);
            out->left->data.var = var;
            expect(TOK_SEMICOLON);
        } else if(next2->type == TOK_DECREMENT) {
            out = create_tree(TREETYPE_OPERATOR);
            out->data.tok = next2;
            out->left = create_tree(TREETYPE_VARIABLE);
            out->left->data.var = var;
            expect(TOK_SEMICOLON);
        } else {
            out = create_tree(TREETYPE_ASSIGN);
            out->data.tok = next2;
            out->left = create_tree(TREETYPE_VARIABLE);
            out->left->data.var = var;
            out->right = parse_expression(TOK_SEMICOLON);
        }
    } else if(next->type == TOK_LBRACE) {
        tokenizer_get();
        out = create_tree(TREETYPE_BLOCK);
        tree *tmp = out;
        token *peeked = tokenizer_peek();
        while(1) {
            tree *t = parse_code();
            if(t != NULL) {
                tmp->left = t;
                peeked = tokenizer_peek();
                if(peeked->type == TOK_RBRACE || peeked->type == TOK_EOF) {
                    break;
                }
                tmp->right = create_tree(TREETYPE_STATEMENT_LIST);
                tmp = tmp->right;
            } else {
                peeked = tokenizer_peek();
                if(peeked->type == TOK_RBRACE || peeked->type == TOK_EOF) {
                    break;
                }
            }
        }
        if(peeked->type == TOK_EOF) {
            error(peeked, "Reached EOF while parsing block.\n");
        }
        tokenizer_get();
    } else if(next->type == TOK_ASM) {
        tokenizer_get();
        out = parse_asm(next);
    } else if(next->type == TOK_SEMICOLON) {
        tokenizer_get();
    } else {
        variable *var = parse_type(next);
        token *name = expect(TOK_IDENTIFIER);
        if(get_variable_noerror(name) != NULL) {
            error(name, "Variable name already defined.\n");
        }
        token *t = expect(
            TOK_EQUAL, TOK_SEMICOLON
        );
        if(t->type == TOK_SEMICOLON) {
            var->name = name->string;
            add_variable(var);
            out = create_tree(TREETYPE_DEFINE);
            out->left = create_tree(TREETYPE_VARIABLE);
            out->left->data.var = var;
        } else {
            var->name = name->string;
            add_variable(var);
            out = create_tree(TREETYPE_DEFINE);
            out->left = create_tree(TREETYPE_VARIABLE);
            out->left->data.var = var;
            out->right = create_tree(TREETYPE_ASSIGN);
            out->right->left = create_tree(TREETYPE_VARIABLE);
            out->right->left->data.var = var;
            out->right->right = parse_expression(TOK_SEMICOLON);
        }
    }
    return out;
}

tree *parse_declaration() {
    debug(0, "parse_declaration()\n");
    tree *out = NULL;
    variable *var = parse_type();
    token *name = expect(TOK_IDENTIFIER);
    var->name = name->string;
    add_variable(var);
    token *peeked = expect(TOK_LPAREN, TOK_SEMICOLON, TOK_EQUAL);
    if(peeked->type == TOK_LPAREN) { // function definition
        out = create_tree(TREETYPE_FUNCTION_DEFINITION);
        var->is_function = 1;
        var->is_constant = 1;
        varlist *arglist = create_varlist();
        token *next = tokenizer_peek();
        if(next->type == TOK_RPAREN) {
            tokenizer_get();
        } else {
            parse_arg_definition(arglist);
        }
        var->arguments = arglist;
        tree *var_tree = create_tree(TREETYPE_VARIABLE);
        var_tree->data.var = var;
        out->left = var_tree;
        out->right = parse_code();
    } else if(peeked->type == TOK_SEMICOLON) { // definition
        out = create_tree(TREETYPE_NULL); // TODO
    } else { // asignment definition
        out = create_tree(TREETYPE_NULL); // TODO
    }
    return out;
}

tree *parse_declaration_list() {
    debug(0, "parse_declaration_list()\n");
    tree *out = create_tree(TREETYPE_DECLARATION_LIST);
    out->left = parse_declaration();
    token *tok = tokenizer_peek();
    if(tok->type != TOK_EOF) {
        out->right = parse_declaration_list();
        tok = tokenizer_peek();
    }
    return out;
}

tree *parse() {
    debug(0, "parse()\n");
    all_vars = create_varlist();
    local_vars = create_varlist();
    current_scope = 0;
    tree *AST = parse_declaration_list();
    #ifdef PARSER_DEBUG
        printf("ALL VARIABLES:\n");
        print_varlist(all_vars);
    #endif
    #ifdef PRINT_AST
        print_tree(AST);
    #endif
    return AST;
}

tree *create_tree(int type) {
    tree *out = malloc(sizeof(tree));
    out->type = type;
    out->left = NULL;
    out->right = NULL;
    return out;
}

void free_tree(tree *t) {
    if(t->left != NULL) {
        free_tree(t->left);
    }
    if(t->right != NULL) {
        free_tree(t->right);
    }
    free(t);
}

char *get_treetype_name(int type) {
    switch(type) {
        case TREETYPE_NULL:
            return "NULL";
        case TREETYPE_DECLARATION_LIST:
            return "DECLARATION_LIST";
        case TREETYPE_STATEMENT_LIST:
            return "STATEMENT_LIST";
        case TREETYPE_FUNCTION_DEFINITION:
            return "FUNCTION_DEFINITION";
        case TREETYPE_BLOCK:
            return "BLOCK";
        case TREETYPE_ASSIGN:
            return "ASSIGN";
        case TREETYPE_IF:
            return "IF";
        case TREETYPE_WHILE:
            return "WHILE";
        case TREETYPE_FOR:
            return "FOR";
        case TREETYPE_VARIABLE:
            return "VARIABLE";
        case TREETYPE_OPERATOR:
            return "OPERATOR";
        case TREETYPE_ASM:
            return "ASM";
        case TREETYPE_IDENTIFIER:
            return "IDENTIFIER";
        case TREETYPE_FUNC_CALL:
            return "FUNC_CALL";
        case TREETYPE_ARG_LIST:
            return "ARG_LIST";
        case TREETYPE_DEFINE:
            return "DEFINE";
        case TREETYPE_INTEGER:
            return "INTEGER";
        case TREETYPE_CHAR:
            return "CHAR";
        case TREETYPE_STRING:
            return "STRING";
    }
    return "(unknown)";
}

void print_treetype(tree *t) {
    if(t->type == TREETYPE_INTEGER) {
        printf("\x1b[94;1m%d\x1b[0m", t->data.int_value);
    } else if(t->type == TREETYPE_STRING) {
        printf("\x1b[94;1m\"%s\"\x1b[0m", t->data.string_value);
    } else if(t->type == TREETYPE_VARIABLE) {
        printf("\x1b[95;1m%s\x1b[0m", t->data.var->name);
    } else if(t->type == TREETYPE_IDENTIFIER) {
        printf("\x1b[94;1m\"%s\"\x1b[0m", t->data.string_value);
    } else if(t->type == TREETYPE_OPERATOR) {
        printf("\x1b[94;1m%s\x1b[0m", get_string_from_toktype(t->data.tok->type));
    } else {
        printf("\x1b[92;1m%s\x1b[0m", get_treetype_name(t->type));
    }
}

void print_tree2(tree *t, int indent) {
    if(!t->left && !t->right) {
        print_treetype(t);
        printf("\n");
    } else {
        print_treetype(t);
        printf(" {\n");
        if(t->left == NULL) {
            printf("%*sleft = \x1b[91;1mNULL\x1b[0m\n", indent + 2, "");
        } else {
            printf("%*sleft = ", indent + 2, "");
            print_tree2(t->left, indent + 2);
        }
        if(t->right != NULL) {
            printf("%*sright = ", indent + 2, "");
            print_tree2(t->right, indent + 2);
        }
        printf("%*s}\n", indent, "");
    }
}

void print_tree(tree *t) {
    print_tree2(t, 0);
}
