#include "9cc.h"

void usage() { error("Usage: ./9cc <program>"); }

int main(int argc, char **argv) {
    // Show help when there is no argument.
    if (argc == 1) {
        usage();
    }

    // Make user input acceessible.
    user_input = argv[1];

    // Tokenize user input.
    Vector *tokens = tokenize();

    // Construct an abstract syntax tree.
    Vector *code = parse(tokens);

#if LOGLEVEL <= 2
    // Draw the abstract syntax tree.
    draw_ast(code);
#endif

    // Generate code.
    gen_x86(code);

    return 0;
}
