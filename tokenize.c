#include "9cc.h"

char *user_input;
Token *token;

/**
 * Returns the current token if it satisfies the given conditions.
 * Otherwise, NULL will be returned.
 *
 * @param kind The kind of a token.
 * @param str The string expression of a token.
 *
 * @return The current token.
 */
Token *consume(TokenKind kind, char *str) {
    if (token->kind != kind || (str && strcmp(token->str, str) != 0)) {
        return NULL;
    }
    Token *ret = token;
    token = token->next;
    return ret;
}

/**
 * Returns the current token if it satisfies the given conditions.
 * Otherwise, stops the program with an error message.
 *
 * @param kind The kind of a token.
 * @param str The string expression of a token.
 *
 * @return The current token.
 */
Token *expect(TokenKind kind, char *str) {
    if (token->kind != kind || (str && strcmp(token->str, str) != 0)) {
        error_at(token->loc, "Unexpected token");
    }
    Token *ret = token;
    token = token->next;
    return ret;
}

/**
 * Returns True if the kind of the current token is EOF.
 *
 * @return True if the kind of the current token is EOF.
 */
bool at_eof() { return token->kind == TK_EOF; }

/**
 * Returns True if the kind of the current token is a type.
 *
 * @return True if the kind of the current token is a type.
 */
bool at_typename() { return token->kind == TK_RESERVED && !strcmp(token->str, "int"); }

/**
 * Reads reserved keywords consisting of multiple characters.
 * @param p The pointer to the current position.
 * @return A reserved keyword.
 */
char *read_reserved(char *p) {
    char *kws[] = {"return", "if", "else", "while", "for", "int", "sizeof"};
    for (int i = 0; i < sizeof(kws) / sizeof(kws[0]); i++) {
        int len = strlen(kws[i]);
        if (startswith(p, kws[i]) && !isalnumus(p[len])) {
            return kws[i];
        }
    }

    char *multi_ops[] = {"<=", ">=", "==", "!="};
    for (int i = 0; i < sizeof(multi_ops) / sizeof(multi_ops[0]); i++) {
        int len = strlen(multi_ops[i]);
        if (startswith(p, multi_ops[i])) {
            return multi_ops[i];
        }
    }

    char *single_ops[] = {"+", "-", "*", "/", "(", ")", "<", ">", "=", ";", "{", "}", ",", "[", "]", "&"};
    for (int i = 0; i < sizeof(single_ops) / sizeof(single_ops[0]); i++) {
        int len = strlen(single_ops[i]);
        if (startswith(p, single_ops[i])) {
            return single_ops[i];
        }
    }
    return NULL;
}

/**
 * Creates a token.
 *
 * @param kind The kind of a token.
 * @param str The string of a token.
 * @param len The length of the string of a token.
 *
 * @return A token.
 */
Token *new_token(TokenKind kind, Token *cur, char *p, int len) {
    Token *tok = calloc(1, sizeof(Token));

    char *str_sliced = calloc(len + 1, sizeof(char));
    strncpy(str_sliced, p, len);
    str_sliced[len] = '\0';

    tok->kind = kind;
    tok->str = str_sliced;
    tok->loc = p;
    cur->next = tok;
    return tok;
}

/**
 * Tokenize a program.
 *
 * @return The head of a linked list of tokens.
 */
Token *tokenize() {
    Token head;
    head.next = NULL;

    Token *cur = &head;

    char *p = user_input;
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        char *kw = read_reserved(p);
        if (kw) {
            int len = strlen(kw);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalnumus(p[len])) {
                len++;
            }
            cur = new_token(TK_IDENT, cur, p, len);
            p += len;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);  // strtol increments `p`
            continue;
        }
        error_at(p, "Failed to tokenize user input");
    }
    cur = new_token(TK_EOF, cur, p, 0);
    return head.next;
}
