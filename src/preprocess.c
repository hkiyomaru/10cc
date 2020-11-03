#include "10cc.h"

bool is_hash(Token *tok) { return tok->is_bol && !strcmp(tok->str, "#"); }

Token *preprocess(Token *tok) {
    Token head = {};
    Token *cur = &head;

    while (tok->kind != TK_EOF) {
        if (!is_hash(tok)) {
            cur = cur->next = tok;
            tok = tok->next;
            continue;
        }

        tok = tok->next;

        if (tok->is_bol) {
            continue;
        }

        error_at(tok->loc, "invalid preprocessor directive '#%s'\n", tok->str);
    }

    cur->next = tok;
    return head.next;
}
