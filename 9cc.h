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
void error_at(char *loc, char *fmt, ...);
bool startswith(char *p, char *q);
bool is_alnum(char c);

extern char *user_input;

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

void tokenize();
bool consume_op(char *op);
bool consume_stmt(TokenKind kind);
Token *consume_ident();
void expect_op(char *op);
int expect_number();
bool at_eof();

extern Token *token;

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

void parse();
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

extern Node *code[];
extern LVar *locals;

/**
 * codegen.c
 */
void gen();

/**
 * container.c
 */
