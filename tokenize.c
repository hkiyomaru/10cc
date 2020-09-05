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
bool at_typename() { return token->kind == TK_INT; }

/**
 * Creates a token.
 *
 * @param kind The kind of a token.
 * @param str The string of a token.
 * @param len The length of the string of a token.
 *
 * @return A token.
 */
Token *new_token(Token *cur, TokenKind kind, char *p, int len) {
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

        if (strncmp(p, "return", 6) == 0 && !isalnumus(p[6])) {
            cur = new_token(cur, TK_RETURN, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "if", 2) == 0 && !isalnumus(p[2])) {
            cur = new_token(cur, TK_IF, p, 2);
            p += 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !isalnumus(p[4])) {
            cur = new_token(cur, TK_ELSE, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !isalnumus(p[5])) {
            cur = new_token(cur, TK_WHILE, p, 5);
            p += 5;
            continue;
        }

        if (strncmp(p, "for", 3) == 0 && !isalnumus(p[3])) {
            cur = new_token(cur, TK_FOR, p, 3);
            p += 3;
            continue;
        }

        if (strncmp(p, "int", 3) == 0 && !isalnumus(p[3])) {
            cur = new_token(cur, TK_INT, p, 3);
            p += 3;
            continue;
        }

        if (strncmp(p, "sizeof", 6) == 0 && !isalnumus(p[6])) {
            cur = new_token(cur, TK_SIZEOF, p, 6);
            p += 6;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalnumus(p[len])) len++;
            cur = new_token(cur, TK_IDENT, p, len);
            p += len;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(cur, TK_RESERVED, p, 2);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>=;{}[],&", *p)) {
            cur = new_token(cur, TK_RESERVED, p, 1);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(cur, TK_NUM, p, 0);
            cur->val = strtol(p, &p, 10); /**< strtol increments `p` */
            continue;
        }
        error_at(p, "Failed to tokenize user input");
    }
    cur = new_token(cur, TK_EOF, p, 0);
    return head.next;
}
