#include "9cc.h"

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    // make user input accessible
    user_input = argv[1];

    // tokenize user input
    tokenize();

    // construct an abstract syntax tree
    parse();

    // header
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", locals->offset);
    
    // generated program code
    gen();

    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
