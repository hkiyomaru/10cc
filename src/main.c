#include "10cc.h"

char *filename;
char *user_input;

void usage() { error("error: no input files\n"); }

// Read a file.
char *read_file(char *path) {
    // Open the file.
    FILE *fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s\n", path, strerror(errno));
    }

    // Investigate the file size.
    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s\n", path, strerror(errno));
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error("%s: fseek: %s\n", path, strerror(errno));
    }

    // Read the file.
    char *buff = calloc(1, size + 2);
    fread(buff, size, 1, fp);
    fclose(fp);

    // Make sure that the file ends with a new line.
    if (size == 0 || buff[size - 1] != '\n') {
        buff[size++] = '\n';
    }
    buff[size] = '\0';
    return buff;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        usage();
    }
    filename = argv[1];
    user_input = read_file(filename);
    ctok = tokenize();
    ctok = preprocess(ctok);
    Prog *prog = parse();
    prog = assign_type(prog);
    // draw_ast(prog);
    codegen(prog);
    return 0;
}
