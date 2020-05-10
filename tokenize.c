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

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, "return", 6);
            p += 6;
            continue;
        }

        if (isalpha(*p)) {
            int len = 0;
            while(is_alnum(p[len]))
                len++;
            cur = new_token(TK_IDENT, cur, p, len);
            p+=len;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
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
        error("Tokenization failed");
    }
    new_token(TK_EOF, cur, "EOF", 0);
    return head.next;
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

bool consume_stmt(int kind) {
    if (token->kind != kind) {
        return false;
    }
    token = token->next;
    return true;
}

Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *current_token = token;
    token = token->next;
    return current_token;
}

int expect_number() {
    if (token->kind !=TK_NUM)
        error("Expected a number, but got %s", token->str);
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}
