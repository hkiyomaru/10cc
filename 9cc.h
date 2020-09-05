#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGLEVEL 6

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

Vector *vec_create();
void vec_push(Vector *vec, void *elem);
void *vec_get(Vector *vec, int key);
Map *map_create();
void map_insert(Map *map, char *key, void *val);
void *map_at(Map *map, char *key);

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

typedef enum { TY_INT, TY_PTR, TY_ARY } TypeKind;

struct Type {
    TypeKind ty;
    int size;
    int align;

    struct Type *base; /**< used when ty is TY_PTR or TY_ARY */
    int array_size;
};

typedef struct Type Type;

Vector *tokenize();

/**
 * parse.c
 */

/**< AST nodes */
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
    ND_GVAR,       // global variable
    ND_RETURN,     // return
    ND_IF,         // if
    ND_ELSE,       // else
    ND_WHILE,      // while
    ND_FOR,        // for
    ND_FUNC_CALL,  // function call
    ND_BLOCK,      // block
    ND_ADDR,       // &
    ND_DEREF       // *
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
    Type *rty;
    char *name;
    Map *lvars;
    Vector *args;
    Vector *body;
} Function;

typedef struct {
    Map *fns;
    Map *gvars;
} Program;

Program *parse(Vector *tokens_);

Type *int_ty();
Type *ptr_to(Type *base);
Node *new_node(NodeKind kind);
Node *new_node_num(int val);

/**
 * sema.c
 */
void sema(Program *prog);

/**
 * codegen.c
 */
void gen_x86();

/**
 * util.c
 */
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool startswith(char *p, char *q);
bool isalnumus(char c);
void draw_ast(Program *prog);
