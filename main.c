#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "parser.h"
#include "codegen.h"

void debug_tokens() {
    while(tokenizer_peek()->type != TOK_EOF) {
        token *tok = tokenizer_get();
        char *s1 = get_string_from_toktype(tok->type);
        char *s2 = token_to_string(tok);
        if(strcmp(s1, s2) == 0) {
            printf("TOKEN: %s\n", s2);
        } else {
            printf("TOKEN: %s, %s\n", s2, s1);
        }
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    char *input_filename = "stdin";
    char *output_filename = "stdout";
    FILE *input = stdin;
    FILE *output = stdout;
    if(argc == 2) {
        input_filename = argv[1];
        input = fopen(input_filename, "r");
        if(input == NULL) {
            fprintf(stderr, "%s\n", strerror(errno));
            return 1;
        }
    } else if(argc == 3) {
        input_filename = argv[1];
        output_filename = argv[2];
        input = fopen(input_filename, "r");
        output = fopen(output_filename, "w");
        if(input == NULL || output == NULL) {
            fprintf(stderr, "%s\n", strerror(errno));
            return 1;
        }
    } else if(argc > 3) {
        fprintf(stderr, "Usage: %s [input] [output]\n", argv[0]);
        return 1;
    }
    tokenizer_init(input, input_filename);
    
    //debug_tokens();
    
    tree *AST = parse();
    
    FILE *outputf = fopen("test/output.sall", "w");
    
    if(outputf == NULL) {
        printf("Cannot open file for writing.");
        return 1;
    }
    
    generate(outputf, AST);
    
    return 0;
}
