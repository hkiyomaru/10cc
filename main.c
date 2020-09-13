#include "9cc.h"

void usage() { error("Usage: ./9cc <path/to/file>"); }

int main(int argc, char **argv) {
    if (argc == 1) {
        usage();
    }
    filename = argv[1];
    user_input = read_file(filename);
    token = tokenize();
    Program *prog = parse();
    sema(prog);
    // draw_ast(prog);
    gen_x86(prog);
    return 0;
}
