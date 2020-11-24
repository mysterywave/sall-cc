#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "parser.h"
#include "tokenizer.h"

//#define PARSER_DEBUG

#define expect(...) _expect((sizeof((int[]){__VA_ARGS__})/sizeof(int)), __VA_ARGS__)

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

void add_variable(variable *var) {
    var->global_id = all_vars->length;
    varlist_add(all_vars, var);
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

token *_expect(int n, ...) {
    token *tok = tokenizer_get();
    
    va_list ap;
    va_start(ap, n);
    int i;
    for(i = 0; i < n; i++) {
        if(tok->type == va_arg(ap, int)) {
            return tok;
        }
    }
    va_end(ap);
    printf("Expecting ");
    va_list ap2;
    va_start(ap2, n);
    for(i = 0; i < n; i++) {
        int j = va_arg(ap2, int);
        if(i == n - 1) {
            printf(", or \"%s\"\n", get_string_from_toketype(j));
        } else if(i == 0) {
            printf("\"%s\"", get_string_from_toketype(j));
        } else {
            printf(", \"%s\"", get_string_from_toketype(j));
        }
    }
    va_end(ap2);
    error(tok, "Unexpected token\n");
}

variable *parse_type2(token *tok1) {
    debug(0, "parse_type()\n");
    variable *output = create_variable();
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
    }
    token *tok2 = tokenizer_peek();
    while(tok2->type == TOK_STAR) {
        tokenizer_get();
        output->pointers++;
        tok2 = tokenizer_peek();
    }
    return output;
}

variable *parse_type() {
    token *tok1 = expect(TOK_VOID, TOK_CHAR, TOK_INT, TOK_LONG, TOK_FLOAT, TOK_DOUBLE);
    return parse_type2(tok1);
}

void parse_arg_definition(varlist *arglist) {
    debug(0, "parse_arg_definition()\n");
    variable *arg = parse_type();
    token *name = expect(TOK_IDENTIFIER);
    arg->name = name->string;
    arg->is_argument = 1;
    add_variable(arg);
    token *next = expect(TOK_RPAREN, TOK_COMMA);
    if(next->type == TOK_RPAREN) {
        return;
    } else {
        parse_arg_definition(arglist);
    }
}

tree *parse_reverse_polish(opstack *stack) {
    tree *out = NULL;
    return out;
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
        case TOK_INCREMENT:
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
            }
    }
    // should never get here
    error(tok, "Strange operator: %s\n", get_string_from_toketype(tok->type));
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
        case TOK_INCREMENT:
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
            }
    }
    // should never get here
    error(tok, "Strange operator: %s\n", get_string_from_toketype(tok->type));
}

void opmod(token *tok, int prefix_postfix) {
    switch(tok->type) {
        case TOK_STAR:
            if(prefix_postfix == 0) {
                tok->type = TOK_MULTIPLY;
            } else {
                tok->type = TOK_POINTER;
            }
        case TOK_AMPERSAND:
            if(prefix_postfix == 0) {
                tok->type = TOK_BITWISE_AND;
            } else {
                tok->type = TOK_ADDRESS;
            }
        case TOK_PLUS:
            if(prefix_postfix == 0) {
                tok->type = TOK_POSITIVE;
            } else {
                tok->type = TOK_ADD;
            }
        case TOK_MINUS:
            if(prefix_postfix == 0) {
                tok->type = TOK_NEGATIVE;
            } else {
                tok->type = TOK_SUBTRACT;
            }
    }
}

tree *parse_expression(int end_type) {
    debug(0, "parse_expression()\n");
    // shunting yard
    
    opstack *out = create_opstack();
    opstack *tmp = create_opstack();
    
    int prefix_postfix = -1;
    
    token *tok = tokenizer_peek();
    while(tok->type != TOK_EOF && tok->type != end_type) {
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
            opstack_push(out, tok);
            token *peeked = tokenizer_peek();
            if(peeked->type == TOK_INCREMENT || peeked->type == TOK_DECREMENT) {
                prefix_postfix = -1;
            } else {
                prefix_postfix = 0;
            }
        } else if(tok->type == TOK_LPAREN) {
            opstack_push(tmp, tok);
        } else if(tok->type == TOK_RPAREN) {
            while(!opstack_empty(tmp) && opstack_peek(tmp)->type != TOK_LPAREN) {
                opstack_push(out, opstack_pop(tmp));
            }
            if(opstack_empty(out)) {
                error(tok, "Mismatched parenthesis");
            }
            opstack_pop(tmp);
        } else {
            if(prefix_postfix > 1) {
                error(tok, "Error parsing token\n");
            }
            while(!opstack_empty(tmp) && opstack_peek(tmp)->type != TOK_LPAREN &&
                  (precedence(opstack_peek(tmp), prefix_postfix) < precedence(tok, prefix_postfix)
                      || (precedence(opstack_peek(tmp), prefix_postfix) == precedence(tok, prefix_postfix) && is_left_associative(tok, prefix_postfix)))) {
                opstack_push(out, opstack_pop(tmp));
            }
            opstack_push(tmp, tok);
            prefix_postfix++;
        }
        tok = tokenizer_peek();
    }
    if(tok->type == TOK_EOF) {
        error(tokenizer_peek(), "Reached EOF while parsing expression\n");
    }
    while(!opstack_empty(tmp)) {
        opstack_push(out, opstack_pop(tmp));
    }
    tokenizer_get();
    #ifdef PARSER_DEBUG
        printf("Reverse Polish: ");
        print_opstack(out);
    #endif
    return parse_reverse_polish(tmp);
}

tree *parse_while() {
    debug(0, "parse_while()\n");
    tree *out;
    return out;
}

tree *parse_if() {
    debug(0, "parse_if()\n");
    tree *out;
    expect(TOK_LPAREN); // expect a (
    tree *expression = parse_expression(TOK_RPAREN); // parse expression until it finds )
    out->left = expression;
    out->right = parse_code();
    return out;
}

tree *parse_for() {
    debug(0, "parse_for()\n");
    tree *out;
    return out;
}

tree *parse_switch() {
    debug(0, "parse_switch()\n");
    tree *out;
    return out;
}

tree *parse_code() {
    debug(1, "parse_code()\n");
    tree *out;
    int type = tokenizer_peek()->type;
    token *next = expect(
        TOK_WHILE, TOK_IF, TOK_FOR,
        TOK_IDENTIFIER,
        TOK_LBRACE,
        TOK_VOID, TOK_CHAR, TOK_INT, TOK_LONG, TOK_FLOAT, TOK_DOUBLE, TOK_STRUCT, TOK_UNION
    );
    if(next->type == TOK_WHILE) {
        out = parse_while();
    } else if(next->type == TOK_IF) {
        out = parse_if();
    } else if(next->type == TOK_FOR) {
        out = parse_for();
    } else if(next->type == TOK_SWITCH) {
        out = parse_switch();
    } else if(next->type == TOK_IDENTIFIER) {
        ;
    } else if(next->type == TOK_LBRACE) {
        out = create_tree(TREETYPE_BLOCK);
        tree *tmp = out;
        token *peeked;
        while((peeked = tokenizer_peek())->type != TOK_EOF && peeked->type != TOK_RBRACE) {
            tmp->right = create_tree(TREETYPE_STATEMENT_LIST);
            tree *code;
            while((code = parse_code()) == NULL) {}
            tmp->left = code;
            tmp = tmp->right;
        }
        if(peeked->type == TOK_EOF) {
            error(peeked, "Reached EOF while parsing block.\n");
        }
    } else {
        variable *type = parse_type2(next);
        token *name = expect(TOK_IDENTIFIER);
        token *t = expect(
            TOK_EQUAL, TOK_PLUS_EQUAL, TOK_MINUS_EQUAL, TOK_TIMES_EQUAL, TOK_DIVIDE_EQUAL, TOK_MOD_EQUAL, TOK_AND_EQUAL, TOK_OR_EQUAL, TOK_XOR_EQUAL, TOK_LSHIFT_EQUAL, TOK_RSHIFT_EQUAL,
            TOK_SEMICOLON
        );
        if(t->type == TOK_SEMICOLON) {
            return NULL;
        } else {
            parse_expression(TOK_SEMICOLON);
        }
    }
    token *t = tokenizer_peek();
    out->right = parse_code();
    return out;
}

tree *parse_declaration() {
    debug(0, "parse_declaration()\n");
    tree *out = create_tree(TREETYPE_NULL);
    variable *var = parse_type();
    token *name = expect(TOK_IDENTIFIER);
    var->name = name->string;
    add_variable(var);
    token *peeked = expect(TOK_LPAREN, TOK_SEMICOLON, TOK_EQUAL);
    if(peeked->type == TOK_LPAREN) { // function definition
        out = create_tree(TREETYPE_FUNCTION_DEFINITION);
        var->is_function = 1;
        varlist *arglist = create_varlist();
        parse_arg_definition(arglist);
        var->arguments = arglist;
        parse_code();
    } else if(peeked->type == TOK_SEMICOLON) { // definition
        ;
    } else { // asignment definition
        ;
    }
    return out;
}

tree *parse_declaration_list() {
    debug(0, "parse_declaration_list()\n");
    tree *out = create_tree(TREETYPE_DECLARATION_LIST);
    out->left = parse_declaration();
    out->right = parse_declaration_list();
    return out;
}

tree *parse() {
    debug(0, "parse()\n");
    all_vars = create_varlist();
    local_vars = create_varlist();
    tree *AST = parse_declaration_list();
    printf("ALL VARIABLES:\n");
    print_varlist(all_vars);
    print_tree(AST);
    return AST;
}

tree *create_tree(int type) {
    tree *out = malloc(sizeof(tree));
    out->type = type;
    out->left = NULL;
    out->right = NULL;
    return out;
}

void print_tree2(tree *t, int indent) {
    if(t->left || t->right) {
        printf("tree\n", indent, "");
    } else {
        printf("tree {\n", indent, "");
        if(t->left) {
            printf("%*sleft = ", indent + 2, "");
            print_tree2(t->left, indent + 2);
        }
        if(t->right) {
            printf("%*sright = ", indent + 2, "");
            print_tree2(t->right, indent + 2);
        }
        printf("%*s}", indent, "");
    }
}

void print_tree(tree *t) {
    print_tree2(t, 0);
}
