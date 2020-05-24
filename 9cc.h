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
bool isalnumus(char c);

extern char *user_input;

/**
 * container.c
 */
typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *create_vec();
void add_elem_to_vec(Vector *vec, void *elem);

typedef struct {
    Vector *keys;
    Vector *vals;
    int len;
} Map;

Map *create_map();
void add_elem_to_map(Map *map, char *key, void *val);
void *get_elem_from_map(Map *map, char *key);

/**
 * tokenize.c
 */
typedef enum {
    TK_RESERVED,  // indicates '+', '-', and so on
    TK_IDENT,     // indicates an identifier
    TK_NUM,       // indicates a number
    TK_RETURN,    // indicates 'return'
    TK_IF,        // indicates if'
    TK_ELSE,      // indicates 'else'
    TK_WHILE,     // indicates 'while'
    TK_FOR,       // indicates 'for'
    TK_EOF,       // indicates EOF
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    int val;
    char *str;
    char *loc;
};

Vector *tokenize();

/**
 * parse.c
 */
typedef enum {
    ND_ADD,        // indicates +
    ND_SUB,        // indicates -
    ND_MUL,        // indicates *
    ND_DIV,        // indicates 
    ND_EQ,         // indicates ==
    ND_NE,         // indicates ==
    ND_LE,         // indicates <=
    ND_LT,         // indicates <
    ND_ASSIGN,     // indicates =
    ND_NUM,        // indicates a number
    ND_LVAR,       // indicates a local variable
    ND_RETURN,     // indicates 'return'
    ND_IF,         // indicates 'if'
    ND_ELSE,       // indicates 'else'
    ND_WHILE,      // indicates 'while'
    ND_FOR,        // indicates 'for'
    ND_FUNC_CALL,  // indicates a function call
    ND_BLOCK,      // indicates a block
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    
    int val;

    Node *lhs;
    Node *rhs;

    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *upd;

    Vector *stmts;
    Vector *args;

    int offset;
};

typedef struct LVar LVar;

struct LVar {
    int offset;
};

Vector *parse();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

extern Map *locals;

/**
 * codegen.c
 */
void translate();
