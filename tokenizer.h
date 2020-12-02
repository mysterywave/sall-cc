#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "datastructs.h"

enum {
    TOK_EOF = 0x0000,
    TOK_ERROR,
    
    // keywords
    TOK_AUTO = 0x1000,
    TOK_BREAK,
    TOK_CASE,
    TOK_CHAR,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_DEFAULT,
    TOK_DO,
    TOK_DOUBLE,
    TOK_ELSE,
    TOK_ENUM,
    TOK_EXTERN,
    TOK_FLOAT,
    TOK_FOR,
    TOK_GOTO,
    TOK_IF,
    TOK_INT,
    TOK_LONG,
    TOK_REGISTER,
    TOK_RETURN,
    TOK_SHORT,
    TOK_SIGNED,
    TOK_SIZEOF,
    TOK_STATIC,
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_TYPEDEF,
    TOK_UNION,
    TOK_UNSIGNED,
    TOK_VOID,
    TOK_VOLATILE,
    TOK_WHILE,
    TOK_ASM,
    
    // operators
    TOK_EQUAL = 0x2000,
    TOK_PLUS_EQUAL,
    TOK_MINUS_EQUAL,
    TOK_TIMES_EQUAL,
    TOK_DIVIDE_EQUAL,
    TOK_MOD_EQUAL,
    TOK_AND_EQUAL,
    TOK_OR_EQUAL,
    TOK_XOR_EQUAL,
    TOK_LSHIFT_EQUAL,
    TOK_RSHIFT_EQUAL,
    TOK_DIVIDE,
    TOK_MOD,
    TOK_BITWISE_OR,
    TOK_XOR,
    TOK_LSH,
    TOK_RSH,
    TOK_AND,
    TOK_OR,
    TOK_EQUAL_TO,
    TOK_NOT_EQUAL,
    TOK_LESS,
    TOK_MORE,
    TOK_LESS_EQUAL,
    TOK_MORE_EQUAL,
    TOK_ARROW,
    TOK_DOT,
    TOK_3DOT,
    TOK_COMMA,
    TOK_QUESTION_MARK,
    TOK_COLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LSQUARE,
    TOK_RSQUARE,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    
    TOK_ADD,
    TOK_SUBTRACT,
    TOK_MULTIPLY,
    TOK_BITWISE_AND,
    
    // single argument operators
    
    TOK_POSITIVE = 0x3000,
    TOK_NEGATIVE,
    TOK_POINTER,
    TOK_ADDRESS,
    TOK_PREFIX_INCREMENT,
    TOK_PREFIX_DECREMENT,
    TOK_POSTFIX_INCREMENT,
    TOK_POSTFIX_DECREMENT,
    TOK_BITWISE_NOT,
    TOK_NOT,
    
    // ambiguous operators
    
    TOK_PLUS = 0x4000,
    TOK_MINUS,
    TOK_STAR,
    TOK_AMPERSAND,
    TOK_INCREMENT,
    TOK_DECREMENT,
    
    // const
    TOK_INT_CONST = 0x5000,
    TOK_LONG_CONST,
    TOK_CHAR_CONST,
    TOK_STRING_CONST,
    TOK_FLOAT_CONST,
    TOK_DOUBLE_CONST,
    
    TOK_IDENTIFIER = 0x6000
};

typedef struct tokenizer tokenizer;

struct tokenizer {
    tokenizer *parent;
    
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
};

typedef struct token {
    int type;
    char *string;
    int value;
    int prefix_postfix;
    
    // for error printing
    int line_offset;
    int length;
} token;

void init_tokenizer();
void end_tokenizer();
tokenizer *tokenizer_create(tokenizer *parent, FILE *in, char *filename_in);
void tokenizer_delete(tokenizer *reader);
token *tokenizer_get_f(tokenizer *reader);
token *tokenizer_peek_f(tokenizer *reader);
token *tokenizer_get();
token *tokenizer_peek();

void delete_token(token *t);
char *get_string_from_toktype(int type);
void free_tokens();
int get_toktype_from_string();
char *token_to_string(token *tok);
void print_token(token *tok);

int is_operator(int toktype);
int is_single_argument(int type);
int is_constant(int toktype);

#endif
