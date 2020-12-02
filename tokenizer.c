#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "datastructs.h"

typedef struct string_len {
    char *string;
    int len;
} string_len;

struct {
    char *string;
    int value;
} convert_table[] = {
    // keywords
    {"auto", TOK_AUTO},
    {"break", TOK_BREAK},
    {"case", TOK_CASE},
    {"char", TOK_CHAR},
    {"const", TOK_CONST},
    {"continue", TOK_CONTINUE},
    {"default", TOK_DEFAULT},
    {"do", TOK_DO},
    {"double", TOK_DOUBLE},
    {"else", TOK_ELSE},
    {"enum", TOK_ENUM},
    {"extern", TOK_EXTERN},
    {"float", TOK_FLOAT},
    {"for", TOK_FOR},
    {"goto", TOK_GOTO},
    {"if", TOK_IF},
    {"int", TOK_INT},
    {"long", TOK_LONG},
    {"register", TOK_REGISTER},
    {"return", TOK_RETURN},
    {"short", TOK_SHORT},
    {"signed", TOK_SIGNED},
    {"sizeof", TOK_SIZEOF},
    {"static", TOK_STATIC},
    {"struct", TOK_STRUCT},
    {"switch", TOK_SWITCH},
    {"typedef", TOK_TYPEDEF},
    {"union", TOK_UNION},
    {"unsigned", TOK_UNSIGNED},
    {"void", TOK_VOID},
    {"volitile", TOK_VOLATILE},
    {"while", TOK_WHILE},
    {"asm", TOK_ASM},
    
    // operators
    {"=", TOK_EQUAL},
    {"+=", TOK_PLUS_EQUAL},
    {"-=", TOK_MINUS_EQUAL},
    {"*=", TOK_TIMES_EQUAL},
    {"/=", TOK_DIVIDE_EQUAL},
    {"%=", TOK_MOD_EQUAL},
    {"&=", TOK_AND_EQUAL},
    {"|=", TOK_OR_EQUAL},
    {"^=", TOK_XOR_EQUAL},
    {"<<=", TOK_LSHIFT_EQUAL},
    {">>=", TOK_RSHIFT_EQUAL},
    {"++", TOK_INCREMENT},
    {"--", TOK_DECREMENT},
    {"/", TOK_DIVIDE},
    {"%", TOK_MOD},
    {"~", TOK_BITWISE_NOT},
    {"|", TOK_BITWISE_OR},
    {"^", TOK_XOR},
    {"<<", TOK_LSH},
    {">>", TOK_RSH},
    {"!", TOK_NOT},
    {"&&", TOK_AND},
    {"||", TOK_OR},
    {"==", TOK_EQUAL_TO},
    {"!=", TOK_NOT_EQUAL},
    {"<", TOK_LESS},
    {">", TOK_MORE},
    {"<=", TOK_LESS_EQUAL},
    {">=", TOK_MORE_EQUAL},
    {"->", TOK_ARROW},
    {".", TOK_DOT},
    {"...", TOK_3DOT},
    {",", TOK_COMMA},
    {"?", TOK_QUESTION_MARK},
    {":", TOK_COLON},
    {"(", TOK_LPAREN},
    {")", TOK_RPAREN},
    {"[", TOK_LSQUARE},
    {"]", TOK_RSQUARE},
    {"{", TOK_LBRACE},
    {"}", TOK_RBRACE},
    {";", TOK_SEMICOLON},
    {"sizeof", TOK_SIZEOF},
    
    {"+", TOK_PLUS},
    {"-", TOK_MINUS},
    {"*", TOK_STAR},
    {"&", TOK_AMPERSAND},
    
    // these should go after the ambiguous operators
    {"+", TOK_ADD},
    {"-", TOK_SUBTRACT},
    {"*", TOK_MULTIPLY},
    {"+", TOK_POSITIVE},
    {"-", TOK_NEGATIVE},
    {"*", TOK_POINTER},
    {"&", TOK_ADDRESS},
    {"&", TOK_BITWISE_AND},
    {"++", TOK_PREFIX_INCREMENT},
    {"--", TOK_PREFIX_DECREMENT},
    {"++", TOK_POSTFIX_INCREMENT},
    {"--", TOK_POSTFIX_DECREMENT},
    
    {"[ INT CONST ]", TOK_INT_CONST},
    {"[ LONG CONST ]", TOK_LONG_CONST},
    {"[ CHAR CONST ]", TOK_CHAR_CONST},
    {"[ STRING CONST ]", TOK_STRING_CONST},
    {"[ FLOAT CONST ]", TOK_FLOAT_CONST},
    {"[ DOUBLE CONST ]", TOK_DOUBLE_CONST},
    {"[ IDENTIFIER ]", TOK_IDENTIFIER},
    
    {"[ EOF ]", TOK_EOF},
    {"[ ERROR ]", TOK_ERROR},
};

string_len multi_char_operators[] = {
    {"++"},
    {"--"},
    {"->"},
    {"<<"},
    {">>"},
    {"<="},
    {">="},
    {"=="},
    {"!="},
    {"&&"},
    {"||"},
    {"+="},
    {"-="},
    {"*="},
    {"/="},
    {"%="},
    {"<<="},
    {">>="},
    {"&="},
    {"^="},
    {"|="},
    {"..."}
};

toklist *tokens_to_free;

FILE *file;
char *filename;
int line_num;
int line_offset;
string_builder current_line;
token *output;
string_builder tok;
int type;
int ptype;
char c;
char ending_char;
int peeked;
int newline_found;

token *create_token(int type);

int get_char_value(char *string) {
    int len = strlen(string);
    if(len > 2 || (len == 2 && string[0] != '\\')) {
        printf("Error reading char: '%s'\n", string);
        exit(1);
    } else if(len == 2) {
        switch(string[1]) {
            case '0':
                return '\0';
            case 'a':
                return '\a';
            case 'b':
                return '\b';
            case 'e':
                return '\e';
            case 'f':
                return '\f';
            case 'n':
                return '\n';
            case 'r':
                return '\r';
            case 't':
                return '\t';
            case 'v':
                return '\v';
            case '\\':
                return '\\';
            case '\'':
                return '\'';
            case '"':
                return '"';
            case '?':
                return '\?';
            default:
                printf("Error: Unknown escape character \'\\%c\'", string[1]);
                exit(0);
        }
    }
    return string[0];
}

int to_integer(char *string) {
    int len = strlen(string);
    int output = 0;
    int i;
    if(len >= 3) {
        if(string[0] == '0' && string[1] == 'x') {
            for(i = len - 1; i >= 0; i--) {
                output <<= 4;
                if(string[len - 1 - i] >= '0' && string[len - 1 - i] <= '9') {
                    output |= string[len - 1 - i] - '0';
                } else if(string[len - 1 - i] >= 'a' && string[len - 1 - i] <= 'f') {
                    output |= string[len - 1 - i] - 'a' + 10;
                } else if(string[len - 1 - i] >= 'A' && string[len - 1 - i] <= 'F') {
                    output |= string[len - 1 - i] - 'A' + 10;
                } else {
                    return -1;
                }
            }
        } else if(string[0] == '0' && string[1] == 'b') {
            for(i = len - 1; i >= 0; i--) {
                output <<= 1;
                if(string[len - 1 - i] == '0' || string[len - 1 - i] == '1') {
                    output |= string[len - 1 - i] - '0';
                } else {
                    return -1;
                }
            }
        }
    }
    for(i = len - 1; i >= 0; i--) {
        output *= 10;
        if(string[len - 1 - i] >= '0' && string[len - 1 - i] <= '9') {
            output += string[len - 1 - i] - '0';
        } else {
            return -1;
        }
    }
    return output;
}

int can_combine_operator(char *string, int length, char new_char) {
    int i;
    for(i = 0; i < sizeof(multi_char_operators) / sizeof(multi_char_operators[0]); i++) {
        string_len operator = multi_char_operators[i];
        int len = operator.len > length ? length : operator.len;
        if(strncmp(operator.string, string, len) == 0) {
            if(operator.string[len] == new_char) {
                return 1;
            }
        }
    }
    return 0;
}

void tokenizer_init(FILE *in, char *filename_in) {
    file = in;
    filename = filename_in;
    string_builder_init(&tok);
    string_builder_init(&current_line);
    type = -1;
    ptype = -1;
    c = ' ';
    line_num = 0;
    peeked = 0;
    newline_found = 1;
    line_offset = 0;
    output = NULL;
    int i;
    for(i = 0; i < sizeof(multi_char_operators) / sizeof(multi_char_operators[0]); i++) {
        multi_char_operators[i].len = strlen(multi_char_operators[i].string);
    }
    tokens_to_free = create_toklist();
}

token *tokenizer_get2() {
    if(peeked) {
        peeked = 0;
        return output;
    }
    if(output != NULL && output->type == TOK_EOF) {
        delete_token(output);
    }
    while(c != EOF) {
        if(newline_found) {
            line_num++;
            string_builder_clear(&current_line);
            newline_found = 0;
            line_offset = 0;
            while((c = fgetc(file)) != '\n' && c != EOF) {
                string_builder_add(&current_line, c);
            }
            if(c == EOF) {
                break;
            }
            string_builder_add(&current_line, c);
            //char *s = string_builder_get(&current_line);
            //printf("\"%s\"\n", s);
        }
        while(c != EOF) {
            if(string_builder_get_char(&current_line, line_offset) == '/' && string_builder_get_char(&current_line, line_offset + 1) == '/') {
                while(string_builder_get_char(&current_line, line_offset) != '\n') {
                    line_offset++;
                }
                newline_found = 1;
                break;
            } else if(string_builder_get_char(&current_line, line_offset) == '/' && string_builder_get_char(&current_line, line_offset + 1) == '*') {
                line_offset++;
                while(!(string_builder_get_char(&current_line, line_offset - 1) == '*' && string_builder_get_char(&current_line, line_offset) == '/')) {
                    if(string_builder_get_char(&current_line, line_offset) == '\n') {
                        line_num++;
                        string_builder_clear(&current_line);
                        line_offset = 0;
                        while((c = fgetc(file)) != '\n' && c != EOF) {
                            string_builder_add(&current_line, c);
                        }
                        if(c == EOF) {
                            goto outer; // break out of both loops
                        }
                        string_builder_add(&current_line, c);
                    }
                    line_offset++;
                }
                line_offset++;
                break;
            }
            c = string_builder_get_char(&current_line, line_offset++);
            if(type == 2) {
                while(c != ending_char && !(c == EOF || line_offset > current_line.length)) {
                    string_builder_add(&tok, c);
                    c = string_builder_get_char(&current_line, line_offset++);
                }
                if(line_offset > current_line.length) {
                    if(ending_char == '"') {
                        printf("error: String is split over newline.\n");
                    } else {
                        printf("error: Char is split over newline.\n");
                    }
                    exit(1);
                }
                if(c == EOF) {
                    printf("Reached EOF while parsing string or char.\n");
                    exit(1);
                }
                c = string_builder_get_char(&current_line, line_offset++);
            }
            if(c == '\n') {
                newline_found = 1;
                if(tok.length != 0) {
                    char *string = string_builder_get(&tok);
                    token *output = string_to_token(string);
                    string_builder_clear(&tok);
                    return output;
                } else {
                    string_builder_clear(&tok);
                    break;
                }
            }
            if(c == ' ' || c == '\t' || c == '\n') {
                type = 0;
            } else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                type = 1;
            } else if(c == '"') {
                type = 2;
                ending_char = '"';
            } else if(c == '\'') {
                type = 2;
                ending_char = '\'';
            } else {
                type = 3;
            }
            if(tok.length != 0 && (type != ptype || ptype == 2 || (type == 3 && !can_combine_operator(tok.buffer, tok.length, c)))) {
                char *string = string_builder_get(&tok);
                token *output = string_to_token(string);
                string_builder_clear(&tok);
                if(type != 0) {
                    string_builder_add(&tok, c);
                    ptype = type;
                }
                return output;
            }
            if(type != 0) {
                string_builder_add(&tok, c);
            }
            if(type != 0) {
                ptype = type;
            }
        }
    }
    outer:;
    char *string = string_builder_get(&tok);
    if(strlen(string) != 0) {
        token *out = string_to_token(string);
        string_builder_clear(&tok);
        return out;
    }
    output = create_token(TOK_EOF);
    return output;
}

token *tokenizer_peek() {
    token *out = tokenizer_get2(0);
    peeked = 1;
    return out;
}

token *tokenizer_get() {
    return tokenizer_get2(0);
}

void tokenizer_end() {
    free_tokens();
    delete_toklist(tokens_to_free);
    string_builder_end(&tok);
    string_builder_end(&current_line);
}

token *create_token(int type) {
    token *out = malloc(sizeof(token));
    out->type = type;
    out->line_offset = -1;
    return out;
}

void delete_token(token *t) {
    printf("FREEING TOKEN\n");
    if(t->string != NULL) {
        free(t->string);
    }
    free(t);
}

token *string_to_token(char *string) {
    if(ptype == 2) {
        if(string[0] == '"') {
            output = create_token(TOK_STRING_CONST);
            output->string = strdup(string + 1);
        } else {
            char value = get_char_value(string + 1);
            output = create_token(TOK_CHAR_CONST);
            output->value = value;
        }
    } else if(ptype == 3) {
        int i = get_toktype_from_string(string);
        if(i == -1) {
            printf("Unknown token: \"%s\"\n", string);
            exit(1);
        }
        output = create_token(i);
    } else if(ptype == 1) {
        int number = to_integer(string);
        if(number != -1) {
            output = create_token(TOK_INT_CONST);
            output->value = number;
        } else {
            int type = get_toktype_from_string(string);
            if(type != -1) {
                output = create_token(type);
            } else {
                output = create_token(TOK_IDENTIFIER);
                output->string = strdup(string);
            }
        }
    } else {
        printf("Unknown token: \"%s\"\n", string);
        exit(1);
    }
    toklist_add(tokens_to_free, output);
    output->length = strlen(string);
    output->line_offset = line_offset - output->length;
    return output;
}

void free_tokens() {
    int i;
    for(i = 0; i < tokens_to_free->length; i++) {
        delete_token(toklist_get(tokens_to_free, i));
    }
}

int get_toktype_from_string(char *string) {
    int i;
    for(i = 0; i < sizeof(convert_table) / sizeof(convert_table[0]); i++) {
        if(convert_table[i].string != NULL && strcmp(string, convert_table[i].string) == 0) {
            return convert_table[i].value;
        }
    }
    return -1;
}

char *get_string_from_toktype(int value) {
    int i;
    for(i = 0; i < sizeof(convert_table) / sizeof(convert_table[0]); i++) {
        if(convert_table[i].value == value) {
            return convert_table[i].string;
        }
    }
    return "(unknown)";
}

char token_to_string_output[256];

char *token_to_string(token *tok) {
    if(tok->type == TOK_IDENTIFIER) {
        return tok->string;
    } else if(tok->type == TOK_STRING_CONST) {
        snprintf(token_to_string_output, sizeof(token_to_string_output), "\"%s\"", tok->string);
        return token_to_string_output;
    } else if(tok->type == TOK_INT_CONST) {
        snprintf(token_to_string_output, sizeof(token_to_string_output), "%d", tok->value);
        return token_to_string_output;
    } else if(tok->type == TOK_CHAR_CONST) {
        snprintf(token_to_string_output, sizeof(token_to_string_output), "'%c'", tok->value);
        return token_to_string_output;
    }
    int i;
    for(i = 0; i < sizeof(convert_table) / sizeof(convert_table[0]); i++) {
        if(tok->type == convert_table[i].value && convert_table[i].string != NULL) {
            return convert_table[i].string;
        }
    }
}

void print_token(token *tok) {
    printf("TOKEN: %s\n", token_to_string(tok));
    if(tok->line_offset != -1) {
        char *line = string_builder_get(&current_line);
        int pos = tok->line_offset - 1;
        int len = tok->length;
        printf("%.*s", pos, line);
        printf("\x1b[92;1m%.*s\x1b[0m", tok->length, line + pos);
        printf("%.*s\n", current_line.length - (pos + len), line + pos + len);
        printf("\x1b[92;1m%*s^\x1b[0m\n", tok->line_offset - 1, "");
    }
}

int is_operator(int type) {
    int t = (type & 0xF000);
    return t == 0x2000 || t == 0x3000;
}

int is_single_argument(int type) {
    return (type & 0xF000) == 0x3000;
}

int is_constant(int type) {
    return (type & 0xF000) == 0x5000;
}
