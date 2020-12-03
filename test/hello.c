void print(char *string) {
    register char *string_pointer = string;
    register char character = *string_pointer;
    while(character) {
        asm out character 0;
        string_pointer++;
        character = *string_pointer;
    }
}

void main() {
    print("Hello, World!\n");
}
