#include "10cc.h"

typedef struct Scope Scope;
typedef struct VarScope VarScope;
typedef struct TagScope TagScope;
typedef struct InitValue InitValue;

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
};

struct TagScope {
    TagScope *next;
    char *name;
    int depth;

    Type *type;
};

struct InitValue {
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
Var *find_var(char *name);

Type *push_typedef(char *name, Type *type);
Type *find_typedef(char *name);

Type *push_tag(char *name, Type *type);
Type *find_tag(char *name);

Scope *enter_scope();
void leave_scope(Scope *sc);

bool at_typename();

Node *new_node_binop(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_uniop(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);
Node *new_node_varref(Var *var, Token *tok);

Type *read_base_type();
Type *read_type_postfix(Type *type);
Type *struct_decl();
Node *decl();

InitValue *read_lvar_init_value(Type *type);
Node *lvar_init(Type *type, Node *node, InitValue *iv, Token *tok);

Node *compound_stmt();
Node *expr_stmt();
Node *selection_stmt();
Node *iteration_stmt();
Node *jump_stmt();
Node *stmt();
Node *expr();
Node *stmt_expr();
Node *assign();
Node *conditional();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();

Node *increment();
Node *decrement();

// Find a function by name.
Func *find_func(char *name) { return map_contains(prog->fns, name) ? map_at(prog->fns, name) : NULL; }

// Create a variable.
Var *new_var(Type *type, char *name, bool is_local, Token *tok) {
    if (type->kind == TY_VOID) {
        error_at(tok->loc, "error: variable or field '%s' declared void", tok->str);
    }
    if (type->kind == TY_ARY) {
        Type *base = type->base;
        while (base->kind == TY_ARY) {
            base = base->base;
        }
        if (base->kind == TY_VOID) {
            error_at(tok->loc, "error: declaration of '%s' as array of voids", tok->str);
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
    Var *var = find_var(name);
    if (var) {
        if (!is_same_type(type, var->type)) {
            error_at(tok->loc, "error: conflicting types for '%s'", name);
        }
        return var;
    }
    return push_var(name, new_var(type, name, false, tok));
}

// Create a local variable, and registers it to the function.
Var *new_lvar(Type *type, char *name, Token *tok) {
    // Try to find a variable from the scope of the same depth.
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (sc->depth != scope_depth) {
            break;
        }
        if (!strcmp(name, sc->name)) {
            error_at(tok->loc, "error: redeclaration of '%s'", tok->str);
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
    if (var->is_local) {
        vec_push(fn->lvars, var);
    } else {
        vec_push(prog->gvars, var);
    }
    return var;
}

// Find a variable by name.
Var *find_var(char *name) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (!strcmp(name, sc->name)) {
            return sc->var;
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
    char *typenames[] = {"long", "int", "short", "char", "void", "struct", "typedef"};
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
Node *new_node_num(int val, Token *tok) {
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
    if (consume(TK_RESERVED, "long")) {
        type = long_type();
    } else if (consume(TK_RESERVED, "int")) {
        type = int_type();
    } else if (consume(TK_RESERVED, "short")) {
        type = short_type();
    } else if (consume(TK_RESERVED, "char")) {
        type = char_type();
    } else if (consume(TK_RESERVED, "void")) {
        type = void_type();
    } else if (peek(TK_RESERVED, "struct")) {
        type = struct_decl();
    } else {
        Token *tok = consume(TK_IDENT, NULL);
        if (tok) {
            type = find_typedef(tok->str);
        }
    }
    if (!type) {
        error_at(ctok->loc, "error: unknown type name '%s'\n", ctok->str);
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

    // Read a tag.
    Token *tag = consume(TK_IDENT, NULL);
    if (tag && !peek(TK_RESERVED, "{")) {
        Type *type = find_tag(tag->str);
        if (!type) {
            error_at(tag->loc, "error: undefined struct '%s'", tag->str);
        }
        return type;
    }

    Map *members = map_create();
    int offset = 0;
    expect(TK_RESERVED, "{");
    while (!consume(TK_RESERVED, "}")) {
        Member *mem = struct_member();
        if (map_contains(members, mem->name)) {
            error_at(ctok->loc, "error: duplicate member '%s'", mem->name);
        }
        mem->offset = offset;
        offset += mem->type->size;
        map_insert(members, mem->name, mem);
    }

    Type *type = struct_type(members);
    if (tag) {
        push_tag(tag->str, type);
    }

    return type;
}

// Read initial values.
InitValue *read_lvar_init_value(Type *type) {
    InitValue *iv = calloc(1, sizeof(InitValue));
    Token *tok;
    switch (type->kind) {
        case TY_ARY:
            iv->vals = vec_create();
            if (consume(TK_RESERVED, "{")) {
                while (!consume(TK_RESERVED, "}")) {
                    if (iv->vals->len > 0) {
                        expect(TK_RESERVED, ",");
                    }
                    vec_push(iv->vals, read_lvar_init_value(type->base));
                }
            } else if ((tok = consume(TK_STR, NULL))) {
                for (int i = 0; i < strlen(tok->str); i++) {
                    InitValue *v = calloc(1, sizeof(InitValue));
                    v->val = new_node_num(tok->str[i], NULL);
                    vec_push(iv->vals, v);
                }
                InitValue *v = calloc(1, sizeof(InitValue));
                v->val = new_node_num('\0', NULL);
                vec_push(iv->vals, v);
            } else {
                error_at(ctok->loc, "error: expected expression before '%s'", ctok->str);
            }
            break;
        default:
            iv->val = assign();
            break;
    }
    return iv;
}

// Create a node to assign an initial value to a given local variable.
Node *lvar_init(Type *type, Node *node, InitValue *iv, Token *tok) {
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
            InitValue *iv = calloc(1, sizeof(InitValue));
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
    InitValue *iv = read_lvar_init_value(var->type);
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
        vec_push(node->stmts, (void *)stmt());
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
            if (at_typename()) {
                node->init = decl();
            } else {
                node->init = expr_stmt();
            }
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

// stmt-expr = "(" "{" stmt+ "}" ")"
Node *stmt_expr() {
    Node *node = new_node(ND_STMT_EXPR, ctok);

    expect(TK_RESERVED, "(");
    node->stmts = compound_stmt()->stmts;
    expect(TK_RESERVED, ")");

    if (node->stmts->len == 0) {
        error_at(node->tok->loc, "error: void value not ignored as it ought to be\n");
    }
    Node *last = vec_back(node->stmts);
    if (last->kind != ND_EXPR_STMT) {
        error_at(last->tok->loc, "error: void value not ignored as it ought to be\n");
    }
    vec_set(node->stmts, node->stmts->len - 1, last->lhs);
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
        } else if ((tok = consume(TK_RESERVED, "!="))) {
            node = new_node_binop(ND_NE, node, relational(), tok);
        } else {
            return node;
        }
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
        } else if ((tok = consume(TK_RESERVED, ">="))) {
            node = new_node_binop(ND_LE, relational(), node, tok);
            continue;
        } else if ((tok = consume(TK_RESERVED, "<"))) {
            node = new_node_binop(ND_LT, node, relational(), tok);
            continue;
        } else if ((tok = consume(TK_RESERVED, ">"))) {
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
        } else if ((tok = consume(TK_RESERVED, "-"))) {
            node = new_node_binop(ND_SUB, node, mul(), tok);
        } else {
            return node;
        }
    }
}

// mul = unary (("*" | "/") unary)*
Node *mul() {
    Node *node = unary();
    Token *tok;
    for (;;) {
        if ((tok = consume(TK_RESERVED, "*"))) {
            node = new_node_binop(ND_MUL, node, unary(), tok);
        } else if ((tok = consume(TK_RESERVED, "/"))) {
            node = new_node_binop(ND_DIV, node, unary(), tok);
        } else {
            return node;
        }
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
        return increment(unary(), tok);
    } else if ((tok = consume(TK_RESERVED, "--"))) {
        return decrement(unary(), tok);
    } else if ((tok = consume(TK_RESERVED, "+"))) {
        return unary();
    } else if ((tok = consume(TK_RESERVED, "-"))) {
        return new_node_binop(ND_SUB, new_node_num(0, NULL), unary(), tok);
    } else if ((tok = consume(TK_RESERVED, "&"))) {
        return new_node_uniop(ND_ADDR, unary(), tok);
    } else if ((tok = consume(TK_RESERVED, "*"))) {
        return new_node_uniop(ND_DEREF, unary(), tok);
    } else if ((tok = consume(TK_RESERVED, "!"))) {
        return new_node_uniop(ND_NOT, unary(), tok);
    } else if ((tok = consume(TK_RESERVED, "sizeof"))) {
        if (consume(TK_RESERVED, "(")) {
            if (at_typename()) {
                Node *node = new_node_num(read_base_type()->size, tok);
                expect(TK_RESERVED, ")");
                return node;
            } else {
                ctok = tok->next;
            }
        }
        return new_node_uniop(ND_SIZEOF, unary(), tok);
    } else {
        return postfix();
    }
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
            node = new_node_binop(ND_SUB, increment(node, tok), new_node_num(1, tok), tok);
            continue;
        }

        if ((tok = consume(TK_RESERVED, "--"))) {
            node = new_node_binop(ND_ADD, decrement(node, tok), new_node_num(1, tok), tok);
            continue;
        }

        return node;
    }
}

// func-args = "(" (expr ("," expr)*)? ")"
Vector *func_args() {
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

// func-call = ident func-args
Node *func_call() {
    Token *tok = expect(TK_IDENT, NULL);
    Func *fn_ = find_func(tok->str);
    if (!fn_) {
        error_at(tok->loc, "error: undefined reference to `%s'\n", tok->str);
    }
    Node *node = new_node(ND_FUNC_CALL, tok);
    node->funcname = tok->str;
    node->type = fn_->rtype;
    node->args = func_args();
    return node;
}

// primary = func-call
//         | ident
//         | num
//         | str
//         | "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
Node *primary() {
    Token *tok = ctok;

    bool is_func_call = consume(TK_IDENT, NULL) && consume(TK_RESERVED, "(");
    ctok = tok;

    if (is_func_call) {
        return func_call();
    }

    if ((tok = consume(TK_IDENT, NULL))) {
        Var *var = find_var(tok->str);
        if (!var) {
            error_at(tok->loc, "error: '%s' undeclared\n", tok->str);
        }
        return new_node_varref(var, tok);
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
        } else {
            expect(TK_RESERVED, "(");
            Node *node = expr();
            expect(TK_RESERVED, ")");
            return node;
        }
    }

    return NULL;
}

// Increment a node.
Node *increment(Node *node, Token *tok) {
    return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_ADD, node, new_node_num(1, tok), tok), tok);
}

// Decrement a node.
Node *decrement(Node *node, Token *tok) {
    return new_node_binop(ND_ASSIGN, node, new_node_binop(ND_SUB, node, new_node_num(1, tok), tok), tok);
}

// param = T ident ("[" num "]")*
Var *func_param() {
    Type *type = read_base_type();
    Token *tok = expect(TK_IDENT, NULL);
    char *name = tok->str;
    type = read_type_postfix(type);
    return new_lvar(type, name, tok);
}

// params = "(" (param ("," param)*)? ")"
Vector *func_params() {
    Vector *params = vec_create();
    expect(TK_RESERVED, "(");
    while (!consume(TK_RESERVED, ")")) {
        if (params->len > 0) {
            expect(TK_RESERVED, ",");
        }
        vec_push(params, func_param());
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

    // Parse the arguments.
    fn->args = func_params();

    // Check conflict.
    Func *fn_ = find_func(fn->name);
    if (fn_) {
        if (fn_->body) {
            error_at(fn->tok->loc, "error: redefinition of '%s'\n", fn->tok->str);
        }
        if (!is_same_type(fn->rtype, fn_->rtype)) {
            error_at(fn->tok->loc, "error: conflicting types for '%s'", fn->tok->str);
        }
        if (fn->args->len != fn_->args->len) {
            error_at(fn->tok->loc, "error: conflicting types for '%s'", fn->tok->str);
        }
        for (int i = 0; i < fn->args->len; i++) {
            Var *x = vec_at(fn->args, i);
            Var *y = vec_at(fn_->args, i);
            if (!is_same_type(x->type, y->type)) {
                error_at(fn->tok->loc, "error: conflicting types for '%s'", fn->tok->str);
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
    char *name = tok->str;
    type = read_type_postfix(type);
    expect(TK_RESERVED, ";");
    new_gvar(type, name, tok);
}

// top-level = func | gvar
void top_level() {
    Token *tok = ctok;
    read_base_type();
    bool isfunc = consume(TK_IDENT, NULL) && consume(TK_RESERVED, "(");
    ctok = tok;
    if (isfunc) {
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
