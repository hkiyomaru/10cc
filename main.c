#include "10cc.h"

void usage() { error("error: no input files"); }

/**
 * Returns the contents of a given file.
 * @param path Path to a file.
 * @return Contents.
 */
char *read_file(char *path) {
    // Open the file.
    FILE *fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s", path, strerror(errno));
    }

    // Investigate the file size.
    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
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
    token = tokenize();
    Program *prog = parse();
    sema(prog);
    // draw_ast(prog);
    codegen(prog);
    return 0;
}
