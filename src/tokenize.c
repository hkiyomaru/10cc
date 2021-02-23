#include "10cc.h"

Token *ctok;

bool is_bol;  // true if `cur` is at beginning of a line

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
        error_at(ctok->loc, "expected '%s' before '%s' token", str, ctok->str);
    }
    ctok = ctok->next;
    return ret;
}

// Return true if the kind of the current token is EOF.
bool at_eof() { return peek(TK_EOF, NULL); }

// Read an reserved keyword.
char *read_reserved(char *p) {
    char *kws[] = {"return",  "if",     "else", "while", "for",  "break", "continue", "struct",
                   "typedef", "sizeof", "void", "_Bool", "char", "short", "int",      "long"};
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

// Skip a line comment.
bool skip_line_comment(char **p) {
    if (strncmp(*p, "//", 2) != 0) {
        return false;
    }
    *p += 2;
    while (**p != '\n') {
        *p += 1;
    }
    return true;
}

// Skip a block comment.
bool skip_block_comment(char **p) {
    if (strncmp(*p, "/*", 2) != 0) {
        return false;
    }
    char *q = strstr(*p + 2, "*/");
    if (!q) {
        error_at(*p, "unterminated comment");
    }
    *p = q + 2;
    return true;
}

// Skip a new line.
bool skip_newline(char **p) {
    if (**p != '\n') {
        return false;
    }
    *p += 1;
    is_bol = true;
    return true;
}

// Skip a white space.
bool skip_space(char **p) {
    if (!isspace(**p)) {
        return false;
    }
    *p += 1;
    return true;
}

// Get a string.
char *get_str(TokenKind kind, char **p) {
    if (kind == TK_STR) {
        (*p)++;
    }

    char *str = calloc(256, sizeof(char));
    int len = 0;
    switch (kind) {
        case TK_RESERVED: {
            len = strlen(read_reserved(*p));
            break;
        }
        case TK_STR: {
            while ((*p)[len] && (*p)[len] != '"') {
                if ((*p)[len] == '\\') {
                    len++;
                }
                len++;
            }
            break;
        }
        case TK_IDENT: {
            len = 1;
            while (isalnumus((*p)[len])) {
                len++;
            }
            break;
        }
        default:
            break;
    }
    strncpy(str, *p, len);
    str[len] = '\0';
    *p += len;

    if (kind == TK_STR) {
        (*p)++;
    }
    return str;
}

// Get a value.
int get_val(TokenKind kind, char **p) {
    int val;
    switch (kind) {
        case TK_NUM:
            val = strtol(*p, p, 10);
            break;
        default:
            break;
    }
    return val;
}

// Create a token.
Token *new_token(TokenKind kind, Token *cur, char **p) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = *p;
    tok->str = get_str(kind, p);
    tok->val = get_val(kind, p);
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
        if (skip_line_comment(&p)) {
            continue;
        }
        if (skip_block_comment(&p)) {
            continue;
        }
        if (skip_newline(&p)) {
            continue;
        }
        if (skip_space(&p)) {
            continue;
        }
        if (read_reserved(p)) {
            cur = new_token(TK_RESERVED, cur, &p);
            continue;
        }
        if (*p == '"') {
            cur = new_token(TK_STR, cur, &p);
            continue;
        }
        if (isalpha(*p) || *p == '_') {
            cur = new_token(TK_IDENT, cur, &p);
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, &p);
            continue;
        }
        error_at(p, "stray '%c' in program", *p);
    }
    new_token(TK_EOF, cur, &p);
    return head.next;
}
