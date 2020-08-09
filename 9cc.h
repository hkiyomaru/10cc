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

typedef struct {
    Vector *keys;
    Vector *vals;
    int len;
} Map;

Vector *create_vec();
void add_elem_to_vec(Vector *vec, void *elem);
void *get_elem_from_vec(Vector *vec, int key);
Map *create_map();
void add_elem_to_map(Map *map, char *key, void *val);
void *get_elem_from_map(Map *map, char *key);

/**
 * tokenize.c
 */
typedef enum {
    TK_RESERVED,  // '+', '-', and so on
    TK_IDENT,     // identifier
    TK_NUM,       // number
    TK_RETURN,    // return
    TK_IF,        // if
    TK_ELSE,      // else
    TK_WHILE,     // while
    TK_FOR,       // for
    TK_EOF,       // EOF
    TK_INT,       // int
    TK_SIZEOF     // sizeof
} TokenKind;

typedef struct {
    TokenKind kind;
    int val;
    char *str;
    char *loc;
} Token;

struct Type {
    enum {INT, PTR, ARY} ty;
    int size;
    int align;
    int array_size;
    struct Type *ptr_to;
    struct Type *ary_of;
};

typedef struct Type Type;

Vector *tokenize();

/**
 * parse.c
 */
typedef enum {
    ND_ADD,        // +
    ND_SUB,        // -
    ND_MUL,        // *
    ND_DIV,        // /
    ND_EQ,         // ==
    ND_NE,         // ==
    ND_LE,         // <=
    ND_LT,         // <
    ND_ASSIGN,     // =
    ND_NUM,        // number
    ND_LVAR,       // local variable
    ND_RETURN,     // return
    ND_IF,         // if
    ND_ELSE,       // else
    ND_WHILE,      // while
    ND_FOR,        // for
    ND_FUNC_CALL,  // function call
    ND_BLOCK,      // block
    ND_ADDR,       // &
    ND_DEREF,      // *
    ND_VARREF      // reference to variable
} NodeKind;

typedef struct Node Node;

struct Node {
    char *name;
    NodeKind kind;
    
    Type *ty;
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

    Node *ref;
};

typedef struct {
    char *name;
    Map *lvars;
    Vector *args;
    Vector *body;
    Type *rty;
} Function;

Vector *parse();

/**
 * codegen.c
 */
void translate();
