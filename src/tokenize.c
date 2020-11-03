#include "10cc.h"

Token *ctok;

bool is_bol;

// Return the current token if it satisfies given conditions. Otherwise, NULL will be returned.
Token *peek(TokenKind kind, char *str) {
    if (ctok->kind != kind || (str && strcmp(ctok->str, str))) {
        return NULL;
    }
    return ctok;
}

// Pop the current token if it satisfies given conditions. Otherwise, NULL will be returned.
Token *consume(TokenKind kind, char *str) {
    Token *ret = peek(kind, str);
    if (ret) {
        ctok = ctok->next;
    }
    return ret;
}

// Pop the current token if it satisfies given conditions. Otherwise, raise an error.
Token *expect(TokenKind kind, char *str) {
    Token *ret = peek(kind, str);
    if (!ret) {
        if (!str) {
            switch (kind) {
                case TK_RESERVED:
                    str = "reserved token";
                    break;
                case TK_IDENT:
                    str = "identifier";
                    break;
                case TK_NUM:
                    str = "number";
                    break;
                case TK_STR:
                    str = "string literal";
                    break;
                case TK_EOF:
                    str = "EOF";
                    break;
            }
        }
        error_at(ctok->loc, "error: expected '%s' before '%s' token\n", str, ctok->str);
    }
    ctok = ctok->next;
    return ret;
}

// Return true if the kind of the current token is EOF.
bool at_eof() { return peek(TK_EOF, NULL); }

// Read an reserved keyword.
char *read_reserved(char *p) {
    char *kws[] = {"return", "if",   "else",  "while", "for",     "sizeof", "struct",  "int",
                   "char",   "void", "short", "long",  "typedef", "break",  "continue"};
    for (int i = 0; i < sizeof(kws) / sizeof(kws[0]); i++) {
        int len = strlen(kws[i]);
        if (startswith(p, kws[i]) && !isalnumus(p[len])) {
            return kws[i];
        }
    }
    char *multi_ops[] = {"<=", ">=", "==", "!=", "++", "--", "+=", "-=", "*=", "/=", "->"};
    for (int i = 0; i < sizeof(multi_ops) / sizeof(multi_ops[0]); i++) {
        if (startswith(p, multi_ops[i])) {
            return multi_ops[i];
        }
    }
    char *single_ops[] = {"+", "-", "*", "/", "(", ")", "<", ">", "=", ";", "{",
                          "}", ",", "[", "]", "&", ".", ",", ":", "!", "?", "#"};
    for (int i = 0; i < sizeof(single_ops) / sizeof(single_ops[0]); i++) {
        if (startswith(p, single_ops[i])) {
            return single_ops[i];
        }
    }
    return NULL;
}

// Create a token.
Token *new_token(TokenKind kind, Token *cur, char *p, int len) {
    char *str = calloc(len + 1, sizeof(char));
    strncpy(str, p, len);
    str[len] = '\0';
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->loc = p;
    tok->is_bol = is_bol;
    cur->next = tok;
    is_bol = false;
    return tok;
}

// Tokenize a given file.
Token *tokenize() {
    Token head = {};
    Token *cur = &head;

    char *p = user_input;

    is_bol = true;

    while (*p) {
        // Skip a line comment.
        if (strncmp(p, "//", 2) == 0) {
            p += 2;
            while (*p != '\n') {
                p++;
            }
            continue;
        }

        // Skip a block comment.
        if (strncmp(p, "/*", 2) == 0) {
            char *q = strstr(p + 2, "*/");
            if (!q) {
                error_at(p, "error: unterminated comment\n");
            }
            p = q + 2;
            continue;
        }

        // Skip newline.
        if (*p == '\n') {
            p++;
            is_bol = true;
            continue;
        }

        // Skip white space.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Read a reserved keyword.
        char *kw = read_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        // Read a string literal.
        if (*p == '"') {
            p++;  // skip "
            int len = 0;
            while (p[len] && p[len] != '"') {
                if (p[len] == '\\') {
                    len++;
                }
                len++;
            }
            cur = new_token(TK_STR, cur, p, len);
            p += len;
            p++;  // skip "
            continue;
        }

        // Read an identifier.
        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalnumus(p[len])) {
                len++;
            }
            cur = new_token(TK_IDENT, cur, p, len);
            p += len;
            continue;
        }

        // Read a number.
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);  // strtol increments `p`
            continue;
        }
        error_at(p, "error: stray '%c' in program\n", *p);
    }
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
