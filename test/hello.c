void print(char *string) {
    char *string_pointer = string;
    int character = *string_pointer;
    while(character) {
        asm out character 0;
        string_pointer++;
        character = *string_pointer;
    }
}

void main() {
    print("Hello, World!\n");
}
