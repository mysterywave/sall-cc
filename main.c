#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "parser.h"

void debug_tokens() {
    while(tokenizer_peek()->type != TOK_EOF) {
        printf("TOKEN: %s\n", token_to_string(tokenizer_get()));
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    char *input_filename;
    char *output_filename;
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
    
    parse();
    
    printf("done.\n");
    return 0;
}
