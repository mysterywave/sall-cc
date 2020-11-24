#pragma once

enum {
    TOK_EOF = 0,
    TOK_ERROR,
    
    // keywords
    TOK_AUTO,
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
    
    // operators
    TOK_EQUAL,
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
    TOK_INCREMENT,
    TOK_DECREMENT,
    TOK_DIVIDE,
    TOK_MOD,
    TOK_BITWISE_NOT,
    TOK_BITWISE_OR,
    TOK_XOR,
    TOK_LSH,
    TOK_RSH,
    TOK_NOT,
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
    
    // ambiguous
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_AMPERSAND,
    
    TOK_ADD,
    TOK_POSITIVE,
    TOK_SUBTRACT,
    TOK_NEGATIVE,
    TOK_MULTIPLY,
    TOK_POINTER,
    TOK_ADDRESS,
    TOK_BITWISE_AND,
    
    // const
    TOK_INT_CONST,
    TOK_LONG_CONST,
    TOK_CHAR_CONST,
    TOK_STRING_CONST,
    TOK_FLOAT_CONST,
    TOK_DOUBLE_CONST,
    
    TOK_IDENTIFIER
    
    // internal
};

typedef struct token {
    int type;
    char *string;
    int value;
    
    // for error printing
    int line_offset;
    int length;
} token;

void tokenizer_init();
void tokenizer_end();
token *tokenizer_get();
token *tokenizer_peek();
void delete_token(token *t);

token *string_to_token(char *string);
char *get_string_from_toketype(int type);
void free_tokens();
int get_toktype_from_string();
char *token_to_string(token *tok);
void print_token(token *tok);
