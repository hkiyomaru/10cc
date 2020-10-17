#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
void vec_push(Vector *vec, void *item);
void vec_pushi(Vector *vec, int item);
void *vec_at(Vector *vec, int key);
int vec_ati(Vector *vec, int index);
void *vec_set(Vector *vec, int index, void *item);
void *vec_back(Vector *vec);
Map *map_create();
void map_insert(Map *map, char *key, void *val);
void *map_at(Map *map, char *key);
int map_count(Map *map, char *key);

/**
 * tokenize.c
 */
typedef enum {
    TK_RESERVED,  // '+', '-', and so on
    TK_IDENT,     // identifier
    TK_NUM,       // number
    TK_STR,       // string literal
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

    struct Type *base;  // used when type is TY_PTR or TY_ARY
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
    ND_EXPR_STMT,
    ND_STMT_EXPR,
    ND_NUM,
    ND_VARREF,
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_WHILE,
    ND_FOR,
    ND_FUNC_CALL,
    ND_BLOCK,
    ND_ADDR,
    ND_DEREF,
    ND_SIZEOF,
    ND_NULL
} NodeKind;  // AST nodes

typedef struct Var {
    char *name;
    Type *type;
    bool is_local;

    // Local variables
    int offset;

    // Global variables
    char *data;
} Var;

typedef struct Node Node;
struct Node {
    NodeKind kind;  // Node kind
    Type *type;     // Type
    Token *tok;     // Representative token

    Node *lhs;  // Left-hand side
    Node *rhs;  // Right-hand side

    // "if", "while", or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *upd;

    // Block statement
    Vector *stmts;

    // Function call
    char *funcname;
    Vector *args;

    // Variable reference
    Var *var;

    // Number literal
    int val;
};

typedef struct {
    Type *rtype;
    char *name;
    Vector *lvars;
    Vector *args;
    Node *body;
} Function;

typedef struct {
    Map *fns;
    Vector *gvars;
} Program;

Program *parse();

Type *int_type();
Type *ptr_to(Type *base);
Node *new_node(NodeKind kind, Token *tok);
Node *new_node_bin_op(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_unary_op(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);

/**
 * sema.c
 */
void sema(Program *prog);

/**
 * codegen.c
 */
void codegen();

/**
 * util.c
 */
char *format(char *fmt, ...);
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool startswith(char *p, char *q);
bool isalnumus(char c);
void draw_ast(Program *prog);
