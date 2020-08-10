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
    Vector *tokens = tokenize();

    // construct an abstract syntax tree
    Vector *code = parse(tokens);

#if LOGLEVEL <= 2
    // draw the abstract syntax tree
    draw_ast(code);
#endif

    // generate code
    translate(code);
    return 0;
}
