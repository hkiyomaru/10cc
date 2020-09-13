#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGLEVEL 6

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
    TK_EOF        // EOF
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    char *loc;
};

typedef enum { TY_INT, TY_PTR, TY_ARY, TY_CHAR } TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    int size;
    int align;

    struct Type *base;  // used when ty is TY_PTR or TY_ARY
    int array_size;
};

extern char *filename;
extern char *user_input;
extern Token *token;

Token *peek(TokenKind kind, char *str);
Token *consume(TokenKind kind, char *str);
Token *expect(TokenKind kind, char *str);
bool at_eof();
bool at_typename();
Token *tokenize();

/**
 * parse.c
 */
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_EQ,
    ND_NE,
    ND_LE,
    ND_LT,
    ND_ASSIGN,
    ND_NUM,
    ND_LVAR,
    ND_GVAR,
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_WHILE,
    ND_FOR,
    ND_FUNC_CALL,
    ND_BLOCK,
    ND_ADDR,
    ND_DEREF
} NodeKind;  // AST nodes

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
};

typedef struct {
    Type *rty;
    char *name;
    Map *lvars;
    Vector *args;
    Node *body;
} Function;

typedef struct {
    Map *fns;
    Map *gvars;
} Program;

Program *parse();

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
char *read_file(char *path);
void draw_ast(Program *prog);
