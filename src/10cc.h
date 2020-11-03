#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token Token;
typedef struct Type Type;
typedef struct Node Node;
typedef struct Var Var;
typedef struct Member Member;
typedef struct Func Func;
typedef struct Prog Prog;
typedef struct Vector Vector;
typedef struct Map Map;

// main.c
extern char *filename;
extern char *user_input;

// tokenize.c
extern Token *ctok;

typedef enum {
    TK_RESERVED,  // '+', '-', and so on
    TK_IDENT,     // identifier
    TK_NUM,       // number
    TK_STR,       // string literal
    TK_EOF        // EOF
} TokenKind;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    char *loc;
};

Token *tokenize();

Token *peek(TokenKind kind, char *str);
Token *consume(TokenKind kind, char *str);
Token *expect(TokenKind kind, char *str);

bool at_eof();

// preprocess.c
Token *preprocess(Token *tok);

// parse.c
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_EQ,
    ND_NE,
    ND_LE,
    ND_LT,
    ND_PRE_INC,
    ND_POST_INC,
    ND_PRE_DEC,
    ND_POST_DEC,
    ND_ASSIGN,
    ND_TERNARY,
    ND_EXPR_STMT,
    ND_STMT_EXPR,
    ND_NUM,
    ND_VARREF,
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_WHILE,
    ND_FOR,
    ND_BREAK,
    ND_CONTINUE,
    ND_FUNC_CALL,
    ND_BLOCK,
    ND_ADDR,
    ND_DEREF,
    ND_SIZEOF,
    ND_MEMBER,
    ND_NULL
} NodeKind;  // AST nodes

struct Prog {
    Map *fns;
    Vector *gvars;  // Vector<Var *>
};

struct Func {
    Type *rtype;
    char *name;
    Vector *lvars;  // Vector<Var *>
    Vector *args;   // Vector<Var *>
    Node *body;
    Token *tok;
};

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

    // Struct member
    char *member_name;
    Member *member;
};

struct Var {
    char *name;
    Type *type;
    bool is_local;

    // Local variables
    int offset;

    // Global variables
    char *data;
};

struct Member {
    char *name;
    Type *type;
    int offset;
};

Prog *parse();

Node *new_node(NodeKind kind, Token *tok);
Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_unary(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);

// type.c
typedef enum { TY_LONG, TY_INT, TY_SHORT, TY_PTR, TY_ARY, TY_CHAR, TY_VOID, TY_STRUCT } TypeKind;

struct Type {
    TypeKind kind;
    int size;
    struct Type *base;  // pointer or array
    int array_size;     // array
    Map *members;       // struct
};

Prog *assign_type(Prog *prog);

Type *long_type();
Type *int_type();
Type *short_type();
Type *char_type();
Type *void_type();
Type *struct_type();
Type *ptr_to(Type *base);
Type *ary_of(Type *base, int len);
bool is_same_type(Type *x, Type *y);

// container.c
struct Vector {
    void **data;
    int capacity;
    int len;
};

struct Map {
    Vector *keys;
    Vector *vals;
    int len;
};

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
bool map_contains(Map *map, char *key);

// codegen.c
void codegen();

// util.c
char *format(char *fmt, ...);
void debug(char *fmt, ...);
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool startswith(char *p, char *q);
bool isalnumus(char c);
void draw_ast(Prog *prog);
