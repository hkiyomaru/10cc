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
    fprintf(stderr, "\033[36mdebug\033[m: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

// Show an error message.
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "\033[31merror\033[m: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
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
    fprintf(stderr, "\033[31merror\033[m: ");
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
    if (node && node->kind != ND_NULL) {
        for (int i = 0; i < depth; i++) {
            fprintf(stderr, "  ");
        }
        if (strlen(role)) {
            fprintf(stderr, "%s: ", role);
        }
        switch (node->kind) {
            case ND_NOT:
                fprintf(stderr, "NOT\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_ADD:
                fprintf(stderr, "ADD\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_SUB:
                fprintf(stderr, "SUB\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_MUL:
                fprintf(stderr, "MUL\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_DIV:
                fprintf(stderr, "DIV\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_EQ:
                fprintf(stderr, "EQ\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_NE:
                fprintf(stderr, "NE\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_LE:
                fprintf(stderr, "LE\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_LT:
                fprintf(stderr, "LT\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_ASSIGN:
                fprintf(stderr, "ASSIGN\n");
                draw_node(node->lhs, depth + 1, "lhs");
                draw_node(node->rhs, depth + 1, "rhs");
                break;
            case ND_RETURN:
                fprintf(stderr, "RETURN\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_TERNARY:
                fprintf(stderr, "TERNARY\n");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->then, depth + 1, "then");
                draw_node(node->els, depth + 1, "else");
                break;
            case ND_IF:
                fprintf(stderr, "IF\n");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->then, depth + 1, "then");
                draw_node(node->els, depth + 1, "else");
                break;
            case ND_WHILE:
                fprintf(stderr, "WHILE\n");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->then, depth + 1, "then");
                break;
            case ND_FOR:
                fprintf(stderr, "FOR\n");
                draw_node(node->init, depth + 1, "init");
                draw_node(node->cond, depth + 1, "cond");
                draw_node(node->upd, depth + 1, "update");
                draw_node(node->then, depth + 1, "then");
                break;
            case ND_BREAK:
                fprintf(stderr, "BREAK\n");
                break;
            case ND_CONTINUE:
                fprintf(stderr, "CONTINUE\n");
                break;
            case ND_BLOCK:
                fprintf(stderr, "BLOCK\n");
                for (int i = 0; i < node->stmts->len; i++) {
                    draw_node(node->stmts->data[i], depth + 1, "");
                }
                break;
            case ND_STMT_EXPR:
                fprintf(stderr, "STMT_EXPR\n");
                for (int i = 0; i < node->stmts->len; i++) {
                    draw_node(node->stmts->data[i], depth + 1, "");
                }
                break;
            case ND_FUNC_CALL:
                fprintf(stderr, "FUNC_CALL(name: %s)\n", node->funcname);
                for (int i = 0; i < node->args->len; i++) {
                    char *prefix = format("arg%d", i);
                    draw_node(node->args->data[i], depth + 1, prefix);
                }
                break;
            case ND_VARREF:
                if (node->var->is_local) {
                    fprintf(stderr, "VARREF(name: %s, local)\n", node->var->name);
                } else {
                    fprintf(stderr, "VARREF(name: %s, global)\n", node->var->name);
                }
                break;
            case ND_NUM:
                fprintf(stderr, "NUM(%ld)\n", node->val);
                break;
            case ND_ADDR:
                fprintf(stderr, "ADDR\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_DEREF:
                fprintf(stderr, "DEREF\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_EXPR_STMT:
                fprintf(stderr, "EXPR_STMT\n");
                draw_node(node->lhs, depth + 1, "");
                break;
            case ND_SIZEOF:
                fprintf(stderr, "SIZEOF\n");
                draw_node(node->lhs, depth + 1, "");
            case ND_MEMBER:
                fprintf(stderr, "MEMBER(name: %s)\n", node->member_name);
                break;
            case ND_COMMA:
                fprintf(stderr, "COMMA\n");
                draw_node(node->lhs, depth + 1, "");
                draw_node(node->rhs, depth + 1, "");
            default:
                break;
        }
    }
}

// Draw the abstract syntax tree of a program.
void draw_ast(Prog *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Func *fn = vec_at(prog->fns->vals, i);
        fprintf(stderr, "%s()\n", fn->name);
        draw_node(fn->body, 1, "");
        fprintf(stderr, "\n");
    }
}
