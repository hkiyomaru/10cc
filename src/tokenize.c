#include "10cc.h"

Token *ctok;

bool is_bol = true;  // true if `cur` is at beginning of a line

// Return the current token if it satisfies the given conditions. Otherwise, NULL will be returned.
Token *peek(TokenKind kind, char *str) {
    if (ctok->kind != kind || (str && strcmp(ctok->str, str))) {
        return NULL;
    }
    return ctok;
}

// Pop the current token if it satisfies the given conditions. Otherwise, NULL will be returned.
Token *consume(TokenKind kind, char *str) {
    Token *tok = peek(kind, str);
    if (tok) {
        ctok = ctok->next;
    }
    return tok;
}

// Pop the current token if it satisfies the given conditions. Otherwise, raise an error.
Token *expect(TokenKind kind, char *str) {
    Token *tok = peek(kind, str);
    if (!tok) {
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
                case TK_CHAR:
                    str = "char literal";
                    break;
                case TK_EOF:
                    str = "EOF";
                    break;
            }
        }
        error_at(ctok->loc, "expected '%s' before '%s' token", str, ctok->str);
    }
    ctok = ctok->next;
    return tok;
}

// Return true if the kind of the current token is EOF.
bool at_eof() { return peek(TK_EOF, NULL); }

// Read an reserved keyword.
char *read_reserved(char *p) {
    // Keywords.
    char *kws[] = {"return",  "if",     "else", "while", "for",  "break", "continue", "struct", "enum",
                   "typedef", "sizeof", "void", "_Bool", "char", "short", "int",      "long"};
    for (int i = 0; i < sizeof(kws) / sizeof(kws[0]); i++) {
        int len = strlen(kws[i]);
        if (startswith(p, kws[i]) && !(isalnum(p[len]) || p[len] == '_')) {
            return kws[i];
        }
    }
    // Multi-character operations
    char *multi_ops[] = {"<=", ">=", "==", "!=", "++", "--", "+=", "-=", "*=", "/=", "->"};
    for (int i = 0; i < sizeof(multi_ops) / sizeof(multi_ops[0]); i++) {
        if (startswith(p, multi_ops[i])) {
            return multi_ops[i];
        }
    }
    // Single-character operations
    char *single_ops[] = {"+", "-", "*", "/", "(", ")", "<", ">", "=", ";", "{", "}",
                          ",", "[", "]", "&", ".", ",", ":", "!", "?", "~", "#"};
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
        (*p)++;
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
    (*p)++;
    is_bol = true;
    return true;
}

// Skip a white space.
bool skip_space(char **p) {
    if (!isspace(**p)) {
        return false;
    }
    (*p)++;
    return true;
}

// Get an escaped character.
char get_escape_char(char c) {
    switch (c) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 't':
            return '\t';
        case 'n':
            return '\n';
        case 'v':
            return '\v';
        case 'f':
            return '\f';
        case 'r':
            return '\r';
        case 'e':
            return 27;
        case '0':
            return 0;
        default:
            return c;
    }
}

char *get_string_literal(char **p) {
    char *buf = calloc(256, sizeof(char));
    int len = 0;
    (*p)++;  // "
    while (**p != '"') {
        if (**p == '\\') {
            (*p)++;
            buf[len++] = get_escape_char(*((*p)++));
        } else {
            buf[len++] = *((*p)++);
        }
    }
    buf[len] = '\0';
    (*p)++;  // "
    return buf;
}

char get_char_literal(char **p) {
    char c;
    (*p)++;  // '
    if (**p == '\\') {
        (*p)++;
        c = get_escape_char(*((*p)++));
    } else {
        c = *((*p)++);
    }
    if (**p != '\'') {
        error_at(*p, "multi-character character constant");
    }
    (*p)++;  // '
    return c;
}

// Get a string.
char *get_str(TokenKind kind, char **p) {
    int len;
    switch (kind) {
        case TK_STR:
            return get_string_literal(p);
        case TK_RESERVED: {
            len = strlen(read_reserved(*p));
            break;
        }
        case TK_IDENT: {
            len = 1;  // the first character has been verified in tokenize().
            while (isalnum((*p)[len]) || (*p)[len] == '_') {
                len++;
            }
            break;
        }
        default:
            len = 0;
            break;
    }
    char *buf = calloc(256, sizeof(char));
    strncpy(buf, *p, len);
    buf[len] = '\0';
    *p += len;
    return buf;
}

// Get a value.
long get_val(TokenKind kind, char **p) {
    switch (kind) {
        case TK_NUM:
            return strtol(*p, p, 10);
        case TK_CHAR:
            return get_char_literal(p);
        default:
            return 0;
    }
}

// Create a token.
Token *new_token(TokenKind kind, Token *cur, char **p) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind == TK_CHAR ? TK_NUM : kind;  // TK_CHAR is treated as TK_NUM during parsing.
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

    while (*p) {
        if (skip_line_comment(&p) || skip_block_comment(&p) || skip_newline(&p) || skip_space(&p)) {
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
        if (*p == '\'') {
            cur = new_token(TK_CHAR, cur, &p);
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
