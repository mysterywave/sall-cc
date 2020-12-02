void main() {
    asm out 'H' 0;
    asm out 'e' 0;/*
        Multi-line comment test!
    */
    asm out 'l' 0;
    asm out 'l' 0;// single line comment test
    asm out 'o' 0;
    asm out/*
        multi-line comment test #2
    */ ',' 0;
    asm out ' ' 0;// single line comment test #2
    asm out 'W' 0;
    asm out 'o' 0;
    asm out 'r' 0;
    asm out 'l' 0;
    asm out 'd' 0;
    asm out '!' 0;
    asm out '\n' 0;
}
