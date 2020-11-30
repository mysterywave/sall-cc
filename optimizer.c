#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "optimizer.h"
#include "tokenizer.h"

tree *compute_16bit(token *operator, uint16_t a, uint16_t b) {
    uint16_t output;
    switch(operator->type) {
        case TOK_DIVIDE:
            output = a / b;
            break;
        case TOK_MOD:
            output = a % b;
            break;
        case TOK_BITWISE_OR:
            output = a | b;
            break;
        case TOK_XOR:
            output = a ^ b;
            break;
        case TOK_LSH:
            output = a << b;
            break;
        case TOK_RSH:
            output = a >> b;
            break;
        case TOK_AND:
            output = a && b;
            break;
        case TOK_OR:
            output = a || b;
            break;
        case TOK_EQUAL_TO:
            output = a == b;
            break;
        case TOK_NOT_EQUAL:
            output = a != b;
            break;
        case TOK_LESS:
            output = a < b;
            break;
        case TOK_MORE:
            output = a > b;
            break;
        case TOK_LESS_EQUAL:
            output = a <= b;
            break;
        case TOK_MORE_EQUAL:
            output = a >= b;
            break;
        case TOK_ADD:
            output = a + b;
            break;
        case TOK_SUBTRACT:
            output = a - b;
            break;
        case TOK_MULTIPLY:
            output = a * b;
            break;
        case TOK_BITWISE_AND:
            output = a & b;
            break;
    }
    tree *out = create_tree(TREETYPE_INTEGER);
    out->data.int_value = output;
    return out;
}

tree *compute_8bit(token *operator, uint8_t a, uint8_t b) {
    uint16_t output;
    switch(operator->type) {
        case TOK_DIVIDE:
            output = a / b;
            break;
        case TOK_MOD:
            output = a % b;
            break;
        case TOK_BITWISE_OR:
            output = a | b;
            break;
        case TOK_XOR:
            output = a ^ b;
            break;
        case TOK_LSH:
            output = a << b;
            break;
        case TOK_RSH:
            output = a >> b;
            break;
        case TOK_AND:
            output = a && b;
            break;
        case TOK_OR:
            output = a || b;
            break;
        case TOK_EQUAL_TO:
            output = a == b;
            break;
        case TOK_NOT_EQUAL:
            output = a != b;
            break;
        case TOK_LESS:
            output = a < b;
            break;
        case TOK_MORE:
            output = a > b;
            break;
        case TOK_LESS_EQUAL:
            output = a <= b;
            break;
        case TOK_MORE_EQUAL:
            output = a >= b;
            break;
        case TOK_ADD:
            output = a + b;
            break;
        case TOK_SUBTRACT:
            output = a - b;
            break;
        case TOK_MULTIPLY:
            output = a * b;
            break;
        case TOK_BITWISE_AND:
            output = a & b;
            break;
    }
    tree *out = create_tree(TREETYPE_CHAR);
    out->data.int_value = output;
    return out;
}

tree *optimize_expression(tree *in) {
    if(in->type == TREETYPE_OPERATOR) {
        if(is_single_argument(in->data.tok->type)) {
            optimize_expression(in->left);
            return in;
        } else {
            in->left = optimize_expression(in->left);
            in->right = optimize_expression(in->right);
            int a = in->left->type;
            int b = in->right->type;
            if((a == TREETYPE_INTEGER || a == TREETYPE_CHAR) && (b == TREETYPE_INTEGER || b == TREETYPE_CHAR)) {
                if(a == TREETYPE_INTEGER || b == TREETYPE_INTEGER) {
                    tree *out = compute_16bit(in->data.tok, in->left->data.int_value, in->right->data.int_value);
                    if(out == NULL) {
                        return in;
                    }
                    return out;
                } else {
                    tree *out = compute_8bit(in->data.tok, in->left->data.int_value, in->right->data.int_value);
                    if(out == NULL) {
                        return in;
                    }
                    return out;
                }
            } else {
                return in;
            }
        }
    } else if(in->type == TREETYPE_VARIABLE || in->type == TREETYPE_INTEGER || in->type == TREETYPE_STRING || in->type == TREETYPE_CHAR) {
        return in;
    } else {
        printf("weird tree type: %d\n", in->type);
        exit(1);
    }
}
