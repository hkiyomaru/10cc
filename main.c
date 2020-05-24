#include "9cc.h"

void usage() {
    error("Usage: ./9cc <program>");
}

int main(int argc, char **argv) {
    if(argc == 1) {
        usage();
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
    printf("  sub rsp, %d\n", (locals->len) * 8);  // offset
    
    // generated program code
    translate();

    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
