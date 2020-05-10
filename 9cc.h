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
bool is_alnum(char c);

/**
 * tokenize.c
 */
typedef enum {
    TK_RESERVED,  // indicates '+', '-', and so on
    TK_IDENT,     // indicates an identifier
    TK_NUM,       // indicates a number
    TK_EOF,       // indicates EOF
    TK_RETURN,    // indicates 'return'
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

Token *tokenize(char *p);
bool consume_op(char *op);
bool consume_stmt(TokenKind kind);
Token *consume_ident();
int expect_number();
bool at_eof();

/**
 * parse.c
 */
typedef enum {
    ND_ADD,     // indicates +
    ND_SUB,     // indicates -
    ND_MUL,     // indicates *
    ND_DIV,     // indicates 
    ND_EQ,      // indicates ==
    ND_NE,      // indicates ==
    ND_LE,      // indicates <=
    ND_LT,      // indicates <
    ND_ASSIGN,  // indicates =
    ND_NUM,     // indicates a number
    ND_LVAR,    // indicates a local variable
    ND_RETURN,  // indicates 'return'
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;
    int offset;
};

typedef struct LVar LVar;

struct LVar {
    LVar *next;
    char *name;
    int len;
    int offset;
};

void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

LVar *find_lvar(Token *tok);

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
extern Node *code[];
extern LVar *locals;
