#include "9cc.h"

void usage() { error("Usage: ./9cc <program>"); }

int main(int argc, char **argv) {
    if (argc == 1) {
        usage();
    }

    user_input = argv[1];
    token = tokenize();
    Program *prog = parse();
    sema(prog);

#if LOGLEVEL <= 2
    draw_ast(prog);
#endif

    gen_x86(prog);

    return 0;
}
