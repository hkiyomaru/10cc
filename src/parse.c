#include "10cc.h"

typedef struct Scope Scope;
typedef struct VarScope VarScope;
typedef struct TagScope TagScope;
typedef struct InitVal InitVal;

struct Scope {
    VarScope *var_scope;
    TagScope *tag_scope;
};

struct VarScope {
    VarScope *next;
    char *name;
    int depth;

    Var *var;
    Type *type_def;
    Type *enum_type;
    int enum_val;
};

struct TagScope {
    TagScope *next;
    char *name;
    int depth;

    Type *type;
};

struct InitVal {
    Vector *vals;  // Vector<InitValue *>
    Node *val;
};

Prog *prog;  // The program
Func *fn;    // The function being parsed

VarScope *var_scope;
TagScope *tag_scope;
int scope_depth;

int str_label_cnt;

Func *find_func(char *name);

Var *new_var(Type *type, char *name, bool is_local, Token *tok);
Var *new_gvar(Type *type, char *name, Token *tok);
Var *new_lvar(Type *type, char *name, Token *tok);
Var *new_strl(char *str, Token *tok);

VarScope *push_var_scope(char *name);
TagScope *push_tag_scope(char *name);

Var *push_var(char *name, Var *var);
VarScope *find_var(char *name);

Type *push_typedef(char *name, Type *type);
Type *find_typedef(char *name);

Type *push_tag(char *name, Type *type);
Type *find_tag(char *name);

Scope *enter_scope();
void leave_scope(Scope *sc);

bool at_typename();

Node *new_node_binop(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_uniop(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(long val, Token *tok);
Node *new_node_varref(Var *var, Token *tok);

Type *read_base_type();
Type *read_type_postfix(Type *type);
Type *struct_decl();
Type *enum_specifier();
Node *decl();

InitVal *read_lvar_init_val(Type *type);
Node *lvar_init(Type *type, Node *node, InitVal *iv, Token *tok);

Node *stmt();
Node *expr();
Node *assign();
Node *conditional();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();

Node *inc();
Node *dec();

// Find a function by name.
Func *find_func(char *name) { return map_contains(prog->fns, name) ? map_at(prog->fns, name) : NULL; }

// Create a variable.
Var *new_var(Type *type, char *name, bool is_local, Token *tok) {
    if (type->kind == TY_VOID) {
        error_at(tok->loc, "variable or field '%s' declared void", tok->str);
    }
    if (type->kind == TY_ARY) {
        Type *base = type->base;
        while (base->kind == TY_ARY) {
            base = base->base;
        }
        if (base->kind == TY_VOID) {
            error_at(tok->loc, "declaration of '%s' as array of voids", tok->str);
        }
    }
    Var *var = calloc(1, sizeof(Var));
    var->type = type;
    var->name = name;
    var->is_local = is_local;
    return var;
}

// Create a global variable, and registers it to the program.
Var *new_gvar(Type *type, char *name, Token *tok) {
    VarScope *sc = find_var(name);
    if (sc) {
        Var *var = sc->var;
        if (!is_same_type(type, var->type)) {
            error_at(tok->loc, "conflicting types for '%s'", name);
        }
        return var;
    }
    return push_var(name, new_var(type, name, false, tok));
}

// Create a local variable, and registers it to the function.
Var *new_lvar(Type *type, char *name, Token *tok) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (sc->depth != scope_depth) {
            break;
        }
        if (!strcmp(name, sc->name)) {
            error_at(tok->loc, "redeclaration of '%s'", tok->str);
        }
    }
    return push_var(name, new_var(type, name, true, tok));
}

// Create a string literal.
Var *new_strl(char *str, Token *tok) {
    Type *type = ary_of(char_type(), strlen(str) + 1);
    char *name = format(".Lstr%d", str_label_cnt++);
    Var *var = new_var(type, name, false, tok);
    var->data = tok->str;
    return push_var(name, var);
}

// Push a variable scope.
VarScope *push_var_scope(char *name) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->next = var_scope;
    sc->name = name;
    sc->depth = scope_depth;
    var_scope = sc;
    return sc;
}

// Push a tag cope.
TagScope *push_tag_scope(char *name) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->next = tag_scope;
    sc->name = name;
    sc->depth = scope_depth;
    tag_scope = sc;
    return sc;
}

// Push a variable to the current scope.
Var *push_var(char *name, Var *var) {
    push_var_scope(name)->var = var;
    vec_push(var->is_local ? fn->lvars : prog->gvars, var);
    return var;
}

// Find a variable by name.
VarScope *find_var(char *name) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (!strcmp(name, sc->name)) {
            if (!sc->var && !sc->enum_type) {
                error_at(ctok->loc, "unexpected variable name '%s'", name);
            }
            return sc;
        }
    }
    return NULL;
}

// Push a typedef to the current scope.
Type *push_typedef(char *name, Type *type) {
    push_var_scope(name)->type_def = type;
    return type;
}

// Find a typedef by name.
Type *find_typedef(char *name) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (!strcmp(name, sc->name)) {
            return sc->type_def;
        }
    }
    return NULL;
}

// Push a tag to the current scope.
Type *push_tag(char *name, Type *type) {
    push_tag_scope(name)->type = type;
    return type;
}

// Find a type by name.
Type *find_tag(char *name) {
    for (TagScope *sc = tag_scope; sc; sc = sc->next) {
        if (!strcmp(name, sc->name)) {
            return sc->type;
        }
    }
    return NULL;
}

// Enter a new scope.
Scope *enter_scope() {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->var_scope = var_scope;
    sc->tag_scope = tag_scope;
    scope_depth++;
    return sc;
}

// Leave the current scope.
void leave_scope(Scope *sc) {
    var_scope = sc->var_scope;
    tag_scope = sc->tag_scope;
    scope_depth--;
}

// Return true if the kind of the current token is a type name.
bool at_typename() {
    char *typenames[] = {"void", "_Bool", "char", "short", "int", "long", "struct", "typedef", "enum"};
    for (int i = 0; i < sizeof(typenames) / sizeof(typenames[0]); i++) {
        if (peek(TK_RESERVED, typenames[i])) {
            return true;
        }
    }
    Token *tok = peek(TK_IDENT, NULL);
    if (tok && find_typedef(tok->str)) {
        return true;
    }
    return false;
}

// Create a node.
Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

// Create a node to represent a binary operation.
Node *new_node_binop(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// Create a node to represent a unary operation.
Node *new_node_uniop(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

// Create a node to represent a number.
Node *new_node_num(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->type = int_type();
    node->val = val;
    return node;
}

// Create a node to refer a variable.
Node *new_node_varref(Var *var, Token *tok) {
    Node *node = new_node(ND_VARREF, NULL);
    node->type = var->type;
    node->var = var;
    return node;
}

// T = ("int" | "char" | "void" | struct-decl | typedef-name) "*"*
Type *read_base_type() {
    Type *type;
    if (consume(TK_RESERVED, "void")) {
        type = void_type();
    } else if (consume(TK_RESERVED, "_Bool")) {
        type = bool_type();
    } else if (consume(TK_RESERVED, "char")) {
        type = char_type();
    } else if (consume(TK_RESERVED, "short")) {
        type = short_type();
    } else if (consume(TK_RESERVED, "int")) {
        type = int_type();
    } else if (consume(TK_RESERVED, "long")) {
        type = long_type();
    } else if (peek(TK_RESERVED, "struct")) {
        type = struct_decl();
    } else if (peek(TK_RESERVED, "enum")) {
        type = enum_specifier();
    } else {
        type = find_typedef(expect(TK_IDENT, NULL)->str);
    }
    if (!type) {
        error_at(ctok->loc, "unknown type name '%s'", ctok->str);
    }
    while ((consume(TK_RESERVED, "*"))) {
        type = ptr_to(type);
    }
    return type;
}

// type-postfix = ("[" num? "]")*
Type *read_type_postfix(Type *base) {
    if (!consume(TK_RESERVED, "[")) {
        return base;
    }
    Token *tok = consume(TK_NUM, NULL);
    int size = tok ? tok->val : -1;
    expect(TK_RESERVED, "]");
    base = read_type_postfix(base);
    return ary_of(base, size);
}

// struct-member = type ident ("[" num "]")* ";"
Member *struct_member() {
    Member *mem = calloc(1, sizeof(Member));
    mem->type = read_base_type();
    mem->name = expect(TK_IDENT, NULL)->str;
    mem->type = read_type_postfix(mem->type);
    expect(TK_RESERVED, ";");
    return mem;
}

// struct-decl = "struct" ident
//             | "struct" ident? "{" struct-member* "}"
Type *struct_decl() {
    expect(TK_RESERVED, "struct");

    Token *tok = consume(TK_IDENT, NULL);
    if (tok && !peek(TK_RESERVED, "{")) {
        Type *type = find_tag(tok->str);
        if (!type) {
            error_at(tok->loc, "undefined struct '%s'", tok->str);
        }
        return type;
    }

    Map *members = map_create();
    int offset = 0;
    expect(TK_RESERVED, "{");
    while (!consume(TK_RESERVED, "}")) {
        Member *mem = struct_member();
        if (map_contains(members, mem->name)) {
            error_at(ctok->loc, "duplicate member '%s'", mem->name);
        }
        mem->offset = offset;
        offset += mem->type->size;
        map_insert(members, mem->name, mem);
    }

    Type *type = struct_type(members);

    if (tok) {
        push_tag(tok->str, type);
    }

    return type;
}

// enum-decl = "enum" ident
//           | "enum" ident? "{" enum-list? "}"
// enum-list = ident ("=" num)? ("," ident ("=" num)?)* ","?
Type *enum_specifier() {
    expect(TK_RESERVED, "enum");

    Type *type = enum_type();

    Token *tok = consume(TK_IDENT, NULL);
    char *tag_name = tok ? tok->str : NULL;
    if (tok && !peek(TK_RESERVED, "{")) {
        type = find_tag(tag_name);
        if (!type || type->kind != TY_ENUM) {
            error_at(tok->loc, "undefined enum '%s'", tok->str);
        }
        return type;
    }

    int enum_val = 0;
    expect(TK_RESERVED, "{");
    while (!consume(TK_RESERVED, "}")) {
        tok = consume(TK_IDENT, NULL);
        if ((consume(TK_RESERVED, "="))) {
            enum_val = expect(TK_NUM, NULL)->val;
        }
        VarScope *sc = push_var_scope(tok->str);
        sc->enum_type = type;
        sc->enum_val = enum_val++;
        consume(TK_RESERVED, ",");
    }

    if (tag_name) {
        push_tag_scope(tag_name)->type = type;
    }

    return type;
}

// Read initial values.
InitVal *read_lvar_init_val(Type *type) {
    InitVal *iv = calloc(1, sizeof(InitVal));
    Token *tok;
    switch (type->kind) {
        case TY_ARY:
            iv->vals = vec_create();
            if (consume(TK_RESERVED, "{")) {
                while (!consume(TK_RESERVED, "}")) {
                    if (iv->vals->len > 0) {
                        expect(TK_RESERVED, ",");
                    }
                    vec_push(iv->vals, read_lvar_init_val(type->base));
                }
                break;
            }
            if ((tok = consume(TK_STR, NULL))) {
                for (int i = 0; i < strlen(tok->str); i++) {
                    InitVal *v = calloc(1, sizeof(InitVal));
                    v->val = new_node_num(tok->str[i], NULL);
                    vec_push(iv->vals, v);
                }
                InitVal *v = calloc(1, sizeof(InitVal));
                v->val = new_node_num('\0', NULL);
                vec_push(iv->vals, v);
                break;
            }
            error_at(ctok->loc, "expected expression before '%s'", ctok->str);
        default:
            iv->val = assign();
            break;
    }
    return iv;
}

// Create a node to assign an initial value to a given local variable.
Node *lvar_init(Type *type, Node *node, InitVal *iv, Token *tok) {
    Node *initializer = new_node(ND_BLOCK, tok);
    initializer->stmts = vec_create();
    if (type->kind == TY_ARY) {
        for (int i = 0; i < iv->vals->len; i++) {
            Node *node_i = new_node_binop(ND_ADD, node, new_node_num(i, NULL), NULL);
            node_i = new_node_uniop(ND_DEREF, node_i, NULL);
            vec_push(initializer->stmts, lvar_init(type->base, node_i, vec_at(iv->vals, i), tok));
        }
        if (type->array_size == -1) {
            type->array_size = iv->vals->len;
            type->size = type->base->size * type->array_size;
        }
        for (int i = iv->vals->len; i < type->array_size; i++) {
            Node *node_i = new_node_binop(ND_ADD, node, new_node_num(i, NULL), NULL);
            node_i = new_node_uniop(ND_DEREF, node_i, NULL);
            InitVal *iv = calloc(1, sizeof(InitVal));
            iv->val = new_node_num(0, NULL);
            vec_push(initializer->stmts, lvar_init(type->base, node_i, iv, tok));
        }
    } else {
        Node *assign = new_node_binop(ND_ASSIGN, node, iv->val, NULL);
        vec_push(initializer->stmts, new_node_uniop(ND_EXPR_STMT, assign, NULL));
    }
    return initializer;
}

// decl = "typedef" T ident ("[" num "]")* ";"
//      | T ";"
//      | T init-declarator ";"
// init-declarator = ident ("[" num? "]")* ";"
//                 | ident ("[" num? "]")* "=" expr ";"
Node *decl() {
    Token *tok;

    if ((tok = consume(TK_RESERVED, "typedef"))) {
        Type *type = read_base_type();
        tok = consume(TK_IDENT, NULL);
        type = read_type_postfix(type);
        expect(TK_RESERVED, ";");
        push_typedef(tok->str, type);
        return new_node(ND_NULL, tok);
    }

    Type *type = read_base_type();

    if (consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL, NULL);
    }

    tok = expect(TK_IDENT, NULL);
    type = read_type_postfix(type);
    Var *var = new_lvar(type, tok->str, tok);

    if (consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL, NULL);
    }

    tok = expect(TK_RESERVED, "=");
    InitVal *iv = read_lvar_init_val(var->type);
    expect(TK_RESERVED, ";");
    return lvar_init(var->type, new_node_varref(var, tok), iv, tok);
}

// compound-stmt = "{" stmt* "}"
Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK, ctok);
    Scope *sc = enter_scope();
    node->stmts = vec_create();
    expect(TK_RESERVED, "{");
    while (!consume(TK_RESERVED, "}")) {
        vec_push(node->stmts, stmt());
    }
    leave_scope(sc);
    return node;
}

// expr-stmt = expr ";"
Node *expr_stmt() {
    Node *node = new_node_uniop(ND_EXPR_STMT, expr(), ctok);
    expect(TK_RESERVED, ";");
    return node;
}

// selection-stmt = "if" "(" expr ")" stmt ("else" stmt)?
Node *selection_stmt() {
    Token *tok;
    if ((tok = consume(TK_RESERVED, "if"))) {
        Node *node = new_node(ND_IF, tok);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        node->els = consume(TK_RESERVED, "else") ? stmt() : new_node(ND_NULL, NULL);
        return node;
    }

    return NULL;
}

// iteration-stmt = "while" "(" expr ")" stmt
//                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//                | "for" "(" decl? ";" expr? ";" expr? ")" stmt
Node *iteration_stmt() {
    Token *tok;
    if ((tok = consume(TK_RESERVED, "while"))) {
        Node *node = new_node(ND_WHILE, tok);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        return node;
    }
    if ((tok = consume(TK_RESERVED, "for"))) {
        Node *node = new_node(ND_FOR, tok);
        Scope *sc = enter_scope();
        expect(TK_RESERVED, "(");
        if (consume(TK_RESERVED, ";")) {
            node->init = new_node(ND_NULL, NULL);
        } else {
            node->init = at_typename() ? decl() : expr_stmt();
        }
        node->cond = peek(TK_RESERVED, ";") ? new_node_num(1, NULL) : expr();
        expect(TK_RESERVED, ";");
        node->upd = peek(TK_RESERVED, ")") ? new_node(ND_NULL, NULL) : new_node_uniop(ND_EXPR_STMT, expr(), ctok);
        expect(TK_RESERVED, ")");
        node->then = stmt();
        leave_scope(sc);
        return node;
    }
    return NULL;
}

// jump-stmt = "continue" ";"
//           | "break" ";"
//           | "return" expr? ";"
Node *jump_stmt() {
    Token *tok;
    if ((tok = consume(TK_RESERVED, "continue"))) {
        expect(TK_RESERVED, ";");
        return new_node(ND_CONTINUE, tok);
    }
    if ((tok = consume(TK_RESERVED, "break"))) {
        expect(TK_RESERVED, ";");
        return new_node(ND_BREAK, tok);
    }
    if ((tok = expect(TK_RESERVED, "return"))) {
        Node *node = new_node_uniop(ND_RETURN, expr(), tok);
        expect(TK_RESERVED, ";");
        return node;
    }
    return NULL;
}

// stmt = compound-stmt
//      | expr-stmt
//      | selection-stmt
//      | iteration-stmt
//      | jump-stmt
//      | decl
//      | ";"
Node *stmt() {
    Token *tok;
    if (peek(TK_RESERVED, "{")) {
        return compound_stmt();
    }
    if (peek(TK_RESERVED, "if")) {
        return selection_stmt();
    }
    if (peek(TK_RESERVED, "for") || peek(TK_RESERVED, "while")) {
        return iteration_stmt();
    }
    if (peek(TK_RESERVED, "continue") || peek(TK_RESERVED, "break") || peek(TK_RESERVED, "return")) {
        return jump_stmt();
    }
    if (at_typename()) {
        return decl();
    }
    if ((tok = consume(TK_RESERVED, ";"))) {
        return new_node(ND_NULL, tok);
    }
    return expr_stmt();
}

// expr = assign ("," assign)*
Node *expr() {
    Node *node = assign();
    Token *tok;
    while ((tok = consume(TK_RESERVED, ","))) {
        node = new_node_uniop(ND_EXPR_STMT, node, node->tok);
        node = new_node_binop(ND_COMMA, node, assign(), tok);
    }
    return node;
}

// assign = conditional (assign-operator assign)?
// assign-operator = "=" | "+=" | "-=" | "*=" | "/="
Node *assign() {
    Node *node = conditional();
    Token *tok;
    if ((tok = consume(TK_RESERVED, "="))) {
        return new_node_binop(ND_ASSIGN, node, assign(), tok);
    }
    if ((tok = consume(TK_RESERVED, "+="))) {
        return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_ADD, node, assign(), tok), tok);
    }
    if ((tok = consume(TK_RESERVED, "-="))) {
        return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_SUB, node, assign(), tok), tok);
    }
    if ((tok = consume(TK_RESERVED, "*="))) {
        return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_MUL, node, assign(), tok), tok);
    }
    if ((tok = consume(TK_RESERVED, "/="))) {
        return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_DIV, node, assign(), tok), tok);
    }
    return node;
}

// conditional = equality ("?" expr ":" conditional)?
Node *conditional() {
    Node *node = equality();

    Token *tok = consume(TK_RESERVED, "?");
    if (!tok) {
        return node;
    }

    Node *ternary = new_node(ND_TERNARY, tok);
    ternary->cond = node;
    ternary->then = expr();
    expect(TK_RESERVED, ":");
    ternary->els = conditional();
    return ternary;
}

// equality = relational (("==" | "!=") relational)?
Node *equality() {
    Node *node = relational();
    Token *tok;
    for (;;) {
        if ((tok = consume(TK_RESERVED, "=="))) {
            node = new_node_binop(ND_EQ, node, relational(), tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, "!="))) {
            node = new_node_binop(ND_NE, node, relational(), tok);
            continue;
        }
        return node;
    }
}

// relational = add
//            | relatioal relational-operator add
// relational-operator = "<=" | ">=" | "<" | ">"
Node *relational() {
    Node *node = add();
    Token *tok;
    for (;;) {
        if ((tok = consume(TK_RESERVED, "<="))) {
            node = new_node_binop(ND_LE, node, relational(), tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, ">="))) {
            node = new_node_binop(ND_LE, relational(), node, tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, "<"))) {
            node = new_node_binop(ND_LT, node, relational(), tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, ">"))) {
            node = new_node_binop(ND_LT, relational(), node, tok);
            continue;
        }
        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();
    Token *tok;
    for (;;) {
        if ((tok = consume(TK_RESERVED, "+"))) {
            node = new_node_binop(ND_ADD, node, mul(), tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, "-"))) {
            node = new_node_binop(ND_SUB, node, mul(), tok);
            continue;
        }
        return node;
    }
}

// mul = unary (("*" | "/") unary)*
Node *mul() {
    Node *node = unary();
    Token *tok;
    for (;;) {
        if ((tok = consume(TK_RESERVED, "*"))) {
            node = new_node_binop(ND_MUL, node, unary(), tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, "/"))) {
            node = new_node_binop(ND_DIV, node, unary(), tok);
            continue;
        }
        return node;
    }
}

// unary = postfix
//       | ("++" | "--") unary
//       | unary-operator unary
//       | "sizeof" unary
//       | "sizeof" "(" type-name ")"
// unary-operator = "+" | "-" | "&" | "*" | "!"
Node *unary() {
    Token *tok;
    if ((tok = consume(TK_RESERVED, "++"))) {
        return inc(unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "--"))) {
        return dec(unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "+"))) {
        return unary();
    }
    if ((tok = consume(TK_RESERVED, "-"))) {
        return new_node_binop(ND_SUB, new_node_num(0, NULL), unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "&"))) {
        return new_node_uniop(ND_ADDR, unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "*"))) {
        return new_node_uniop(ND_DEREF, unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "!"))) {
        return new_node_uniop(ND_NOT, unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "~"))) {
        return new_node_uniop(ND_BITNOT, unary(), tok);
    }
    if ((tok = consume(TK_RESERVED, "sizeof"))) {
        if (consume(TK_RESERVED, "(")) {
            if (at_typename()) {
                Node *node = new_node_num(read_base_type()->size, tok);
                expect(TK_RESERVED, ")");
                return node;
            }
            ctok = tok->next;
        }
        return new_node_uniop(ND_SIZEOF, unary(), tok);
    }
    return postfix();
}

// postfix = primary
//         | postfix ("[" expr "]" | ("." | "->") ident | "++" | "--")*
Node *postfix() {
    Node *node = primary();
    Token *tok;
    for (;;) {
        if ((tok = consume(TK_RESERVED, "["))) {
            Node *addr = new_node_binop(ND_ADD, node, expr(), tok);
            node = new_node_uniop(ND_DEREF, addr, tok);
            expect(TK_RESERVED, "]");
            continue;
        }
        if ((tok = consume(TK_RESERVED, "."))) {
            node = new_node_uniop(ND_MEMBER, node, tok);
            node->member_name = expect(TK_IDENT, NULL)->str;
            continue;
        }
        if ((tok = consume(TK_RESERVED, "->"))) {
            node = new_node_uniop(ND_DEREF, node, tok);
            node = new_node_uniop(ND_MEMBER, node, tok);
            node->member_name = expect(TK_IDENT, NULL)->str;
            continue;
        }
        if ((tok = consume(TK_RESERVED, "++"))) {
            node = new_node_binop(ND_SUB, inc(node, tok), new_node_num(1, tok), tok);
            continue;
        }
        if ((tok = consume(TK_RESERVED, "--"))) {
            node = new_node_binop(ND_ADD, dec(node, tok), new_node_num(1, tok), tok);
            continue;
        }
        return node;
    }
}

// args = "(" (expr ("," expr)*)? ")"
Vector *args() {
    Vector *args = vec_create();
    expect(TK_RESERVED, "(");
    while (!consume(TK_RESERVED, ")")) {
        if (0 < args->len) {
            expect(TK_RESERVED, ",");
        }
        vec_push(args, assign());
    }
    return args;
}

// call = ident args
Node *call() {
    Token *tok = expect(TK_IDENT, NULL);
    Func *fn_ = find_func(tok->str);
    if (!fn_) {
        error_at(tok->loc, "undefined reference to `%s'", tok->str);
    }
    Node *node = new_node(ND_FUNC_CALL, tok);
    node->func_name = tok->str;
    node->type = fn_->rtype;
    node->args = args();
    return node;
}

// stmt-expr = "(" "{" stmt+ "}" ")"
Node *stmt_expr() {
    Node *node = new_node(ND_STMT_EXPR, ctok);

    expect(TK_RESERVED, "(");
    node->stmts = compound_stmt()->stmts;
    expect(TK_RESERVED, ")");

    if (node->stmts->len == 0) {
        error_at(node->tok->loc, "void value not ignored as it ought to be");
    }
    Node *last = vec_back(node->stmts);
    if (last->kind != ND_EXPR_STMT) {
        error_at(last->tok->loc, "void value not ignored as it ought to be");
    }
    vec_set(node->stmts, node->stmts->len - 1, last->lhs);
    return node;
}

// primary = call
//         | ident
//         | num
//         | str
//         | stmt-expr
//         | "(" expr ")"
Node *primary() {
    Token *tok = ctok;
    bool is_func_call = consume(TK_IDENT, NULL) && consume(TK_RESERVED, "(");
    ctok = tok;

    if (is_func_call) {
        return call();
    }
    if ((tok = consume(TK_IDENT, NULL))) {
        VarScope *sc = find_var(tok->str);
        if (!sc) {
            error_at(tok->loc, "'%s' undeclared", tok->str);
        }
        if (sc->var) {
            return new_node_varref(sc->var, tok);
        }
        if (sc->enum_type) {
            return new_node_num(sc->enum_val, tok);
        }
    }
    if ((tok = peek(TK_NUM, NULL))) {
        tok = expect(TK_NUM, NULL);
        return new_node_num(tok->val, tok);
    }
    if ((tok = peek(TK_STR, NULL))) {
        tok = expect(TK_STR, NULL);
        return new_node_varref(new_strl(tok->str, tok), tok);
    }
    if ((tok = peek(TK_RESERVED, "("))) {
        bool is_stmt_expr = consume(TK_RESERVED, "(") && consume(TK_RESERVED, "{");
        ctok = tok;
        if (is_stmt_expr) {
            return stmt_expr();
        }
        expect(TK_RESERVED, "(");
        Node *node = expr();
        expect(TK_RESERVED, ")");
        return node;
    }
    return NULL;
}

// Increment a node.
Node *inc(Node *node, Token *tok) {
    return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_ADD, node, new_node_num(1, tok), tok), tok);
}

// Decrement a node.
Node *dec(Node *node, Token *tok) {
    return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_SUB, node, new_node_num(1, tok), tok), tok);
}

// param = T ident ("[" num "]")*
Var *param() {
    Type *type = read_base_type();
    Token *tok = expect(TK_IDENT, NULL);
    type = read_type_postfix(type);
    return new_lvar(type, tok->str, tok);
}

// params = "(" (param ("," param)*)? ")"
Vector *params() {
    Vector *params = vec_create();
    expect(TK_RESERVED, "(");
    while (!consume(TK_RESERVED, ")")) {
        if (params->len > 0) {
            expect(TK_RESERVED, ",");
        }
        vec_push(params, param());
    }
    return params;
}

// func = T ident "(" params? ")" "{" stmt* "}"
void func() {
    fn = calloc(1, sizeof(Func));
    fn->rtype = read_base_type();
    fn->tok = expect(TK_IDENT, NULL);
    fn->name = fn->tok->str;
    fn->lvars = vec_create();

    Scope *sc = enter_scope();

    // Parse the parameters.
    fn->params = params();

    // Check conflict.
    Func *fn_ = find_func(fn->name);
    if (fn_) {
        if (fn_->body) {
            error_at(fn->tok->loc, "redefinition of '%s'", fn->name);
        }
        if (!is_same_type(fn->rtype, fn_->rtype)) {
            error_at(fn->tok->loc, "conflicting types for '%s'", fn->name);
        }
        if (fn->params->len != fn_->params->len) {
            error_at(fn->tok->loc, "conflicting types for '%s'", fn->name);
        }
        for (int i = 0; i < fn->params->len; i++) {
            Var *arg = vec_at(fn->params, i);
            Var *arg_ = vec_at(fn_->params, i);
            if (!is_same_type(arg->type, arg_->type)) {
                error_at(fn->tok->loc, "conflicting types for '%s'", fn->name);
            }
        }
    }

    // Register the function to the program.
    map_insert(prog->fns, fn->name, fn);

    // Parse the body.
    if (!consume(TK_RESERVED, ";")) {
        fn->body = compound_stmt();
    }
    leave_scope(sc);
}

// gvar = T ident ("[" num "]")* ";"
void gvar() {
    Type *type = read_base_type();
    Token *tok = expect(TK_IDENT, NULL);
    type = read_type_postfix(type);
    expect(TK_RESERVED, ";");
    new_gvar(type, tok->str, tok);
}

// top-level = func | gvar
void top_level() {
    Token *tok = ctok;
    read_base_type();
    bool is_func = consume(TK_IDENT, NULL) && consume(TK_RESERVED, "(");
    ctok = tok;
    if (is_func) {
        func();
    } else {
        gvar();
    }
}

// program = top-level*
Prog *parse() {
    prog = calloc(1, sizeof(Prog));
    prog->fns = map_create();
    prog->gvars = vec_create();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}
