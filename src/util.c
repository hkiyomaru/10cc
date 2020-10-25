#include "10cc.h"

// Return a formatted string.
char *format(char *fmt, ...) {
    size_t size = 2048;
    char *buff = calloc(size, sizeof(char));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buff, sizeof(char) * size, fmt, ap);
    va_end(ap);
    return buff;
}

// Show an debug message.
void debug(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
}

// Show an error message.
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}

// Show an error message with its location information.
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

// Return true if the first string starts with the second string.
bool startswith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

// Return true if a given character is an alphabet, number, or _.
bool isalnumus(char c) { return isalnum(c) || c == '_'; }

// Draw the abstract syntax tree of a node.
void draw_node(Node *node, int depth, char *role) {
    if (node != NULL) {
        for (int i = 0; i < depth; i++) {
            debug("  ");
        }
        if (strlen(role)) {
            debug("%s: ", role);
        }
        switch (node->kind) {
            case ND_ADD:
                debug("ADD\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_SUB:
                debug("SUB\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_MUL:
                debug("MUL\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_DIV:
                debug("DIV\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_EQ:
                debug("EQ\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_NE:
                debug("NE\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_LE:
                debug("LE\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_LT:
                debug("LT\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_ASSIGN:
                debug("ASSIGN\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_RETURN:
                debug("RETURN\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_IF:
                debug("IF\n");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->then, depth + 1, "then");
                draw_node(node->els, depth + 1, "else");
                break;
            case ND_WHILE:
                debug("WHILE\n");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->then, depth + 1, "then");
                break;
            case ND_FOR:
                debug("FOR\n");
                draw_node(node->init, depth + 1, "init");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->upd, depth + 1, "update");
                draw_node(node->then, depth + 1, "then");
                break;
            case ND_BLOCK:
                debug("BLOCK\n");
                for (int i = 0; i < node->stmts->len; i++) {
                    draw_node(node->stmts->data[i], depth + 1, "");
                }
                break;
            case ND_STMT_EXPR:
                debug("STMT_EXPR\n");
                for (int i = 0; i < node->stmts->len; i++) {
                    draw_node(node->stmts->data[i], depth + 1, "");
                }
                break;
            case ND_FUNC_CALL:
                debug("FUNC_CALL(name: %s)\n", node->funcname);
                for (int i = 0; i < node->args->len; i++) {
                    char *prefix = format("arg%d", i);
                    draw_node(node->args->data[i], depth + 1, prefix);
                }
                break;
            case ND_VARREF:
                if (node->var->is_local) {
                    debug("VARREF(name: %s, local)\n", node->var->name);
                } else {
                    debug("VARREF(name: %s, global)\n", node->var->name);
                }
                break;
            case ND_NUM:
                debug("NUM(%d)\n", node->val);
                break;
            case ND_ADDR:
                debug("ADDR\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_DEREF:
                debug("DEREF\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_EXPR_STMT:
                debug("EXPR_STMT\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_SIZEOF:
                debug("SIZEOF\n");
                draw_node(node->lhs, depth + 1, "");
            case ND_NULL:
                debug("NULL\n");
                break;
            default:
                break;
        }
    }
}

// Draw the abstract syntax tree of a program.
void draw_ast(Prog *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Func *fn = vec_at(prog->fns->vals, i);
        debug("%s(\n", fn->name);
        for (int j = 0; j < fn->args->len; j++) {
            draw_node(fn->args->data[j], 1, format("arg%d", j));
        }
        debug(")\n");
        draw_node(fn->body, 1, "");
        debug("\n");
    }
}
