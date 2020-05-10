#include "9cc.h"

Token *token;

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

void tokenize() {
    Token head;
    head.next = NULL;
    Token *cur = &head;
    char *p = user_input;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while(is_alnum(p[len]))
                len++;
            cur = new_token(TK_IDENT, cur, p, len);
            p += len;
            continue;
        }

        if (startswith(p, "==") ||
            startswith(p, "!=") ||
            startswith(p, "<=") ||
            startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>=;", *p)) {
            cur = new_token(TK_RESERVED, cur, p, 1);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        error_at(p, "Failed to tokenize user input");
    }
    new_token(TK_EOF, cur, p, 0);
    token = head.next;
}

bool consume_op(char *op) {
    if (token->kind != TK_RESERVED || 
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0) {
        return false;
    }
    token = token->next;
    return true;
}

bool consume_stmt(TokenKind kind) {
    if (token->kind != kind)
        return false;
    token = token->next;
    return true;
}

Token *consume_ident() {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *current_token = token;
    token = token->next;
    return current_token;
}

void expect_op(char *op) {
    if(token->kind != TK_RESERVED ||
       strlen(op) != token->len ||
       memcmp(token->str, op, token->len) != 0) {
        error_at(token->str, "Expected '%s'");
    }
    token = token->next;
}

int expect_number() {
    if (token->kind !=TK_NUM)
        error_at(token->str, "Expected a number");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}
