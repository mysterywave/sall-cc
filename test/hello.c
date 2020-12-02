void print(char *string) {
    register char *string_pointer = string;
    register int character = *string_pointer;
    while(character) {
        register int test1 = 1337;
        register int test2 = 2674;
        asm out character 0;
        string_pointer++;
        character = *string_pointer;
    }
    register int test3 = 1337;
    register int test1 = 2674;
}

void main() {
    print("Hello, World!\n");
}
