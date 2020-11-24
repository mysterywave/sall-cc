void print(char *s) {
    char *i = s;
    int j = *i;
    while(j) {
        asm out j 0;
        i++;
        j = *i;
    }
}

void main() {
    print("Hello, World!\n");
}
