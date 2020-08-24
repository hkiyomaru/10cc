#include "9cc.h"

char *user_input;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

bool startswith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

bool isalnumus(char c) { return isalnum(c) || c == '_'; }

void draw_node_tree(Node *node, int depth, char *prefix) {
    if (node != NULL) {
        for (int i = 0; i < depth; i++) {
            fprintf(stderr, "  ");
        }
        if (strlen(prefix)) {
            fprintf(stderr, "%s: ", prefix);
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
                fprintf(stderr, "FUNC_CALL(name: %s, ty: %d)\n", node->name, node->ty->ty);
                for (int i = 0; i < node->args->len; i++) {
                    char prefix[16] = {'\0'};
                    sprintf(prefix, "arg%d", i);
                    draw_node_tree(node->args->data[i], depth + 1, prefix);
                }
                break;
            case ND_GVAR:
                fprintf(stderr, "GVAR(name: %s, ty: %d)\n", node->name, node->ty->ty);
                break;
            case ND_LVAR:
                fprintf(stderr, "LVAR(name: %s, ty: %d)\n", node->name, node->ty->ty);
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

void draw_ast(Program *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Function *fn = vec_get(prog->fns->vals, i);
        fprintf(stderr, "%s(\n", fn->name);
        for (int j = 0; j < fn->args->len; j++) {
            char prefix[256] = {'\0'};
            sprintf(prefix, "arg%d", j);
            draw_node_tree(fn->args->data[j], 1, prefix);
        }
        fprintf(stderr, ")\n");
        for (int j = 0; j < fn->body->len; j++) {
            draw_node_tree(fn->body->data[j], 1, "");
        }
        fprintf(stderr, "\n");
    }
}
