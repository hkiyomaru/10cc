#include "10cc.h"

void usage() { error("error: no input files"); }

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
    codegen(prog);
    return 0;
}
