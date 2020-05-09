#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * util.c
 */
void error(char *fmt, ...);
bool startswith(char *p, char *q);

/**
 * parse.c
 */
typedef enum {
    TK_RESERVED,  // indicates '+' or '-'
    TK_NUM,       // indicates a number
    TK_EOF,       // indicates EOF
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

typedef enum {
    ND_ADD,  // indicates +
    ND_SUB,  // indicates -
    ND_MUL,  // indicates *
    ND_DIV,  // indicates /
    ND_NUM,  // indicates a number
    ND_EQ,   // indicates ==
    ND_NE,   // indicates ==
    ND_LE,   // indicates <=
    ND_LT,   // indicates <
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;
};

Token *tokenize(char *p);

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

/**
 * codegen.c
 */
void gen(Node *node);

/**
 * container.c
 */

/**
 * Global variables
 */
extern Token *token;
