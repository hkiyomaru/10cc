#include "9cc.h"

/**
 * Returns a formatted string.
 * @param fmt The format of a string.
 * @param arg Arguments which will be filled in the message.
 */
char *format(char *fmt, ...) {
    char *buff = calloc(512, sizeof(char));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buff, sizeof(buff), fmt, ap);
    va_end(ap);
    return buff;
}

/**
 * Shows an error message.
 * @param fmt The format of an error message.
 * @param arg Arguments which will be filled in the message.
 */
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/**
 * Shows an error message with its location information.
 * @param loc The location of an error.
 * @param fmt The format of an error message.
 * @param arg Arguments which will be filled in the message.
 */
void error_at(char *loc, char *fmt, ...) {
    // Find the start/end positions of the line.
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }
    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    // Investigate the line number.
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    // Report the line number with the file name.
    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // Display a pointer to the error location.
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");

    // Display an error message.
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/**
 * Returns true if the first string starts with the second string.
 * @param p A string.
 * @param q A string.
 * @return True if the first string starts with the second string.
 */
bool startswith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

/**
 * Returns true if the given character is an alphabet, or a number, or _.
 * @param c A character.
 * @return True if the given character is an alphabet, or a number, or _.
 */
bool isalnumus(char c) { return isalnum(c) || c == '_'; }

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

/**
 * Draws the abstract syntax tree of a node.
 * @param node A node.
 * @param depth The depth of the node.
 * @param role The role of the node.
 */
void draw_node_tree(Node *node, int depth, char *role) {
    if (node != NULL) {
        for (int i = 0; i < depth; i++) {
            fprintf(stderr, "  ");
        }
        if (strlen(role)) {
            fprintf(stderr, "%s: ", role);
        }
        switch (node->kind) {
            case ND_ADD:
                fprintf(stderr, "ADD\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_SUB:
                fprintf(stderr, "SUB\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_MUL:
                fprintf(stderr, "MUL\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_DIV:
                fprintf(stderr, "DIV\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_EQ:
                fprintf(stderr, "EQ\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_NE:
                fprintf(stderr, "NE\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_LE:
                fprintf(stderr, "LE\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_LT:
                fprintf(stderr, "LT\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_ASSIGN:
                fprintf(stderr, "ASSIGN\n");
                draw_node_tree(node->lhs, depth + 1, "lhs");
                draw_node_tree(node->rhs, depth + 1, "rhs");
                break;
            case ND_RETURN:
                fprintf(stderr, "RETURN\n");
                draw_node_tree(node->lhs, depth + 1, "");
                break;
            case ND_IF:
                fprintf(stderr, "IF\n");
                draw_node_tree(node->cond, depth + 1, "cond");
                draw_node_tree(node->then, depth + 1, "then");
                draw_node_tree(node->els, depth + 1, "else");
                break;
            case ND_WHILE:
                fprintf(stderr, "WHILE\n");
                draw_node_tree(node->cond, depth + 1, "cond");
                draw_node_tree(node->then, depth + 1, "then");
                break;
            case ND_FOR:
                fprintf(stderr, "FOR\n");
                draw_node_tree(node->init, depth + 1, "init");
                draw_node_tree(node->cond, depth + 1, "cond");
                draw_node_tree(node->upd, depth + 1, "update");
                draw_node_tree(node->then, depth + 1, "then");
                break;
            case ND_BLOCK:
                fprintf(stderr, "BLOCK\n");
                for (int i = 0; i < node->stmts->len; i++) {
                    draw_node_tree(node->stmts->data[i], depth + 1, "");
                }
                break;
            case ND_FUNC_CALL:
                fprintf(stderr, "FUNC_CALL(name: %s)\n", node->name);
                for (int i = 0; i < node->args->len; i++) {
                    char *prefix = format("arg%d", i);
                    draw_node_tree(node->args->data[i], depth + 1, prefix);
                }
                break;
            case ND_GVAR:
                fprintf(stderr, "GVAR(name: %s)\n", node->name);
                break;
            case ND_LVAR:
                fprintf(stderr, "LVAR(name: %s)\n", node->name);
                break;
            case ND_NUM:
                fprintf(stderr, "NUM(%d)\n", node->val);
                break;
            case ND_ADDR:
                fprintf(stderr, "ADDR\n");
                draw_node_tree(node->lhs, depth + 1, "");
                break;
            case ND_DEREF:
                fprintf(stderr, "DEREF\n");
                draw_node_tree(node->lhs, depth + 1, "");
                break;
            default:
                break;
        }
    }
}

/**
 * Draws the abstract syntax tree of a program.
 * @param prog A program.
 */
void draw_ast(Program *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Function *fn = vec_get(prog->fns->vals, i);
        fprintf(stderr, "%s(\n", fn->name);
        for (int j = 0; j < fn->args->len; j++) {
            char *prefix = format("arg%d", j);
            draw_node_tree(fn->args->data[j], 1, prefix);
        }
        fprintf(stderr, ")\n");
        draw_node_tree(fn->body, 1, "");
        fprintf(stderr, "\n");
    }
}
