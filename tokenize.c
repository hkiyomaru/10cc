#include "9cc.h"

Vector *tokens;

Token *new_token(TokenKind kind, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    
    char *str_sliced = calloc(len + 1, sizeof(char));
    strncpy(str_sliced, str, len);
    str_sliced[len] = '\0';
    
    tok->kind = kind;
    tok->str = str_sliced;
    tok->loc = str;
    return tok;
}

Vector *tokenize() {
    tokens = create_vec();
    char *p = user_input;
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !isalnumus(p[6])) {
            add_elem_to_vec(tokens, new_token(TK_RETURN, p, 6));
            p += 6;
            continue;
        }

        if(strncmp(p, "if", 2) == 0 && !isalnumus(p[2])) {
            add_elem_to_vec(tokens, new_token(TK_IF, p, 2));
            p += 2;
            continue;
        }

        if(strncmp(p, "else", 4) == 0 && !isalnumus(p[4])) {
            add_elem_to_vec(tokens, new_token(TK_ELSE, p, 4));
            p += 4;
            continue;
        }

        if(strncmp(p, "while", 5) == 0 && !isalnumus(p[5])) {
            add_elem_to_vec(tokens, new_token(TK_WHILE, p, 5));
            p += 5;
            continue;
        }

        if(strncmp(p, "for", 3) == 0 && !isalnumus(p[3])) {
            add_elem_to_vec(tokens, new_token(TK_FOR, p, 3));
            p += 3;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while(isalnumus(p[len])) {
                len++;
            }
            add_elem_to_vec(tokens, new_token(TK_IDENT, p, len));
            p += len;
            continue;
        }

        if (startswith(p, "==") ||
            startswith(p, "!=") ||
            startswith(p, "<=") ||
            startswith(p, ">=")) {
            add_elem_to_vec(tokens, new_token(TK_RESERVED, p, 2));
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>=;{},", *p)) {
            add_elem_to_vec(tokens, new_token(TK_RESERVED, p, 1));
            p++;
            continue;
        }

        if (isdigit(*p)) {
            Token *token = new_token(TK_NUM, p, 0);
            token->val = strtol(p, &p, 10);
            add_elem_to_vec(tokens, token);
            continue;
        }
        error_at(p, "Failed to tokenize user input");
    }
    add_elem_to_vec(tokens, new_token(TK_EOF, p, 0));
    return tokens;
}
