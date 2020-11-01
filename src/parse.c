#include "10cc.h"

typedef struct VarScope VarScope;
typedef struct TagScope TagScope;
typedef struct InitValue InitValue;

struct VarScope {
    VarScope *next;
    Map *vars;  // Map<Var *>
};

struct TagScope {
    TagScope *next;
    Map *types;  // Map<Type *>
};

struct InitValue {
    Vector *vals;  // Vector<InitValue *>
    Node *val;
};

Prog *prog;  // The program
Func *fn;    // The function being parsed

VarScope *var_scope;
TagScope *tag_scope;

int str_label_cnt = 1;

Var *new_var(Type *type, char *name, bool is_local, Token *tok);
Var *new_gvar(Type *type, char *name, Token *tok);
Var *new_lvar(Type *type, char *name, Token *tok);
Var *new_strl(char *str, Token *tok);

Var *push_var(Var *var);
Var *find_var(char *name);

Type *push_tag(char *tag, Type *type);
Type *find_tag(char *name);

VarScope *new_var_scope();
TagScope *new_tag_scope();
void *enter_scope();
void leave_scope();

Type *read_base_type();
Type *read_type_postfix(Type *type);
Type *struct_decl();
Member *struct_member();

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_unary(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);
Node *new_node_varref(Var *var, Token *tok);

Func *find_func(char *name);

void top_level();
void func();
void gvar();

Node *decl();

InitValue *read_lvar_init_value(Type *type);
Node *lvar_init(Type *type, Node *node, InitValue *iv, Token *tok);

Node *stmt();
Node *compound_stmt();
Node *expr();
Node *expr_stmt();
Node *stmt_expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();
Node *new_node_func_call(Token *tok);

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
    if (map_contains(var_scope->vars, name)) {
        Var *var = map_at(var_scope->vars, name);
        if (!is_same_type(type, var->type)) {
            error_at(tok->loc, "error: conflicting types for '%s'", name);
        }
        return var;
    }
    return push_var(new_var(type, name, false, tok));
}

// Create a local variable, and registers it to the function.
Var *new_lvar(Type *type, char *name, Token *tok) {
    if (map_contains(var_scope->vars, tok->str)) {
        error_at(tok->loc, "error: redeclaration of '%s'", tok->str);
    }
    return push_var(new_var(type, name, true, tok));
}

// Create a string literal.
Var *new_strl(char *str, Token *tok) {
    Type *type = ary_of(char_type(), strlen(str) + 1);
    char *name = format(".L.str%d", str_label_cnt++);
    Var *var = new_var(type, name, false, tok);
    var->data = tok->str;
    return push_var(var);
}

// Push a variable to the current scope.
Var *push_var(Var *var) {
    map_insert(var_scope->vars, var->name, var);
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
        if (map_contains(sc->vars, name)) {
            return map_at(sc->vars, name);
        }
    }
    return NULL;
}

// Push a tag to the current scope.
Type *push_tag(char *tag, Type *type) {
    map_insert(tag_scope->types, tag, type);
    return type;
}

// Find a type by name.
Type *find_tag(char *name) {
    for (TagScope *sc = tag_scope; sc; sc = sc->next) {
        if (map_contains(sc->types, name)) {
            return map_at(sc->types, name);
        }
    }
    return NULL;
}

// Create an empty variable scope.
VarScope *new_var_scope() {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->next = NULL;
    sc->vars = map_create();
    return sc;
}

// Create an empty tag scope.
TagScope *new_tag_scope() {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->next = NULL;
    sc->types = map_create();
    return sc;
}

// Enter a new scope.
void *enter_scope() {
    VarScope *var_sc = new_var_scope();
    var_sc->next = var_scope;
    var_scope = var_sc;

    TagScope *tag_sc = new_tag_scope();
    tag_sc->next = tag_scope;
    tag_scope = tag_sc;
}

// Leave the current scope.
void leave_scope() {
    var_scope = var_scope->next;
    tag_scope = tag_scope->next;
}

// T = ("int" | "char" | "void" | struct-decl) "*"*
Type *read_base_type() {
    Type *type;
    if (consume(TK_RESERVED, "int")) {
        type = int_type();
    } else if (consume(TK_RESERVED, "char")) {
        type = char_type();
    } else if (consume(TK_RESERVED, "void")) {
        type = void_type();
    } else if (peek(TK_RESERVED, "struct")) {
        type = struct_decl();
    } else {
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

// struct-member = type ident ("[" num "]")* ";"
Member *struct_member() {
    Member *mem = calloc(1, sizeof(Member));
    mem->type = read_base_type();
    mem->name = expect(TK_IDENT, NULL)->str;
    mem->type = read_type_postfix(mem->type);
    expect(TK_RESERVED, ";");
    return mem;
}

// func-call = ident "(" args? ")"
// args = expr ("," expr)*
Node *new_node_func_call(Token *tok) {
    Func *fn_ = find_func(tok->str);
    if (!fn_) {
        error_at(tok->loc, "error: undefined reference to `%s'\n", tok->str);
    }
    Node *node = new_node(ND_FUNC_CALL, tok);
    node->funcname = tok->str;
    node->type = fn_->rtype;
    node->args = vec_create();
    expect(TK_RESERVED, "(");
    while (!consume(TK_RESERVED, ")")) {
        if (0 < node->args->len) {
            expect(TK_RESERVED, ",");
        }
        vec_push(node->args, expr());
    }
    return node;
}

// Create a node.
Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

// Create a node to represent a binary operation.
Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// Create a node to represent a unary operation.
Node *new_node_unary(NodeKind kind, Node *lhs, Token *tok) {
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

// Find a function by name.
Func *find_func(char *name) { return map_contains(prog->fns, name) ? map_at(prog->fns, name) : NULL; }

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

// param = T ident ("[" num "]")*
Var *func_param() {
    Type *type = read_base_type();
    Token *tok = expect(TK_IDENT, NULL);
    char *name = tok->str;
    type = read_type_postfix(type);
    return new_lvar(type, name, tok);
}

// params = (param ("," param)*)?
Vector *func_params() {
    Vector *params = vec_create();
    while (!peek(TK_RESERVED, ")")) {
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

    enter_scope();

    // Parse the arguments.
    expect(TK_RESERVED, "(");
    fn->args = func_params();
    expect(TK_RESERVED, ")");

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
    leave_scope();
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

// decl = T ident ("[" num? "]")* ("=" expr) ";"
//      | T ";"
Node *decl() {
    Type *type = read_base_type();

    if (consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL, NULL);
    }

    Token *tok = expect(TK_IDENT, NULL);
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

// Read initial values.
InitValue *read_lvar_init_value(Type *type) {
    InitValue *iv = calloc(1, sizeof(InitValue));
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
            } else if (consume(TK_RESERVED, "\"")) {
                Token *strl = expect(TK_STR, NULL);
                expect(TK_RESERVED, "\"");
                for (int i = 0; i < strlen(strl->str); i++) {
                    InitValue *v = calloc(1, sizeof(InitValue));
                    v->val = new_node_num(strl->str[i], NULL);
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
            iv->val = expr();
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
            Node *node_i = new_node_binary(ND_ADD, node, new_node_num(i, NULL), NULL);
            node_i = new_node_unary(ND_DEREF, node_i, NULL);
            vec_push(initializer->stmts, lvar_init(type->base, node_i, vec_at(iv->vals, i), tok));
        }

        if (type->array_size == -1) {
            type->array_size = iv->vals->len;
            type->size = type->base->size * type->array_size;
        }

        for (int i = iv->vals->len; i < type->array_size; i++) {
            Node *node_i = new_node_binary(ND_ADD, node, new_node_num(i, NULL), NULL);
            node_i = new_node_unary(ND_DEREF, node_i, NULL);
            InitValue *iv = calloc(1, sizeof(InitValue));
            iv->val = new_node_num(0, NULL);
            vec_push(initializer->stmts, lvar_init(type->base, node_i, iv, tok));
        }
    } else {
        Node *asgn = new_node_binary(ND_ASSIGN, node, iv->val, NULL);
        vec_push(initializer->stmts, new_node_unary(ND_EXPR_STMT, asgn, NULL));
    }
    return initializer;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | compound-stmt
//      | T ident ("=" expr)? ";"
//      | ";"
//      | expr-stmt ";"
Node *stmt() {
    Token *tok;

    if (tok = consume(TK_RESERVED, "return")) {
        Node *node = new_node(ND_RETURN, tok);
        node->lhs = expr();
        expect(TK_RESERVED, ";");
        return node;
    }

    if (tok = consume(TK_RESERVED, "if")) {
        Node *node = new_node(ND_IF, tok);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        node->els = consume(TK_RESERVED, "else") ? stmt() : new_node(ND_NULL, NULL);
        return node;
    }

    if (tok = consume(TK_RESERVED, "while")) {
        Node *node = new_node(ND_WHILE, tok);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        return node;
    }

    if (tok = consume(TK_RESERVED, "for")) {
        Node *node = new_node(ND_FOR, tok);
        expect(TK_RESERVED, "(");
        enter_scope();
        if (consume(TK_RESERVED, ";")) {
            node->init = new_node(ND_NULL, NULL);
        } else {
            if (at_typename()) {
                node->init = decl();
            } else {
                node->init = expr_stmt();
                expect(TK_RESERVED, ";");
            }
        }
        node->cond = peek(TK_RESERVED, ";") ? new_node_num(1, NULL) : expr();
        expect(TK_RESERVED, ";");
        node->upd = peek(TK_RESERVED, ")") ? new_node(ND_NULL, NULL) : expr_stmt();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        leave_scope();
        return node;
    }

    if (tok = peek(TK_RESERVED, "{")) {
        return compound_stmt();
    }

    if (at_typename()) {
        return decl();
    }

    if (tok = consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL, tok);
    }

    Node *node = expr_stmt();
    expect(TK_RESERVED, ";");
    return node;
}

// compound-stmt = "{" stmt* "}"
Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK, ctok);
    node->stmts = vec_create();
    enter_scope();
    expect(TK_RESERVED, "{");
    while (!consume(TK_RESERVED, "}")) {
        vec_push(node->stmts, (void *)stmt());
    }
    leave_scope();
    return node;
}

// expr = assign
Node *expr() { return assign(); }

// expr-stmt = expr
Node *expr_stmt() { return new_node_unary(ND_EXPR_STMT, expr(), ctok); }

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

// assign = equality (("=" | "+=", "-=") assign)?
Node *assign() {
    Node *node = equality();
    Token *tok;

    if (tok = consume(TK_RESERVED, "=")) {
        return new_node_binary(ND_ASSIGN, node, assign(), tok);
    }

    if (tok = consume(TK_RESERVED, "+=")) {
        return new_node_binary(ND_ASSIGN, node, new_node_binary(ND_ADD, node, assign(), tok), tok);
    }

    if (tok = consume(TK_RESERVED, "-=")) {
        return new_node_binary(ND_ASSIGN, node, new_node_binary(ND_SUB, node, assign(), tok), tok);
    }

    if (tok = consume(TK_RESERVED, "*=")) {
        return new_node_binary(ND_ASSIGN, node, new_node_binary(ND_MUL, node, assign(), tok), tok);
    }

    if (tok = consume(TK_RESERVED, "/=")) {
        return new_node_binary(ND_ASSIGN, node, new_node_binary(ND_DIV, node, assign(), tok), tok);
    }

    return node;
}

// equality = relational (("==" | "!=") relational)?
Node *equality() {
    Node *node = relational();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "==")) {
            node = new_node_binary(ND_EQ, node, relational(), tok);
        } else if (tok = consume(TK_RESERVED, "!=")) {
            node = new_node_binary(ND_NE, node, relational(), tok);
        } else {
            return node;
        }
    }
}

// relational = add (("<=" | ">=" | "<" | ">") add)?
Node *relational() {
    Node *node = add();
    Token *tok;
    if (tok = consume(TK_RESERVED, "<=")) {
        return new_node_binary(ND_LE, node, add(), tok);
    } else if (tok = consume(TK_RESERVED, ">=")) {
        return new_node_binary(ND_LE, add(), node, tok);
    } else if (tok = consume(TK_RESERVED, "<")) {
        return new_node_binary(ND_LT, node, add(), tok);
    } else if (tok = consume(TK_RESERVED, ">")) {
        return new_node_binary(ND_LT, add(), node, tok);
    } else {
        return node;
    }
}

// add = mul ("+" unary | "-" unary)*
Node *add() {
    Node *node = mul();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "+")) {
            node = new_node_binary(ND_ADD, node, mul(), tok);
        } else if (tok = consume(TK_RESERVED, "-")) {
            node = new_node_binary(ND_SUB, node, mul(), tok);
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
        if (tok = consume(TK_RESERVED, "*")) {
            node = new_node_binary(ND_MUL, node, unary(), tok);
        } else if (tok = consume(TK_RESERVED, "/")) {
            node = new_node_binary(ND_DIV, node, unary(), tok);
        } else {
            return node;
        }
    }
}

// unary = ("+" | "-" | "&" | "*")? unary
//       | ("++", "--") unary
//       | "sizeof" unary
//       | "sizeof" "(" type-name ")"
//       | postfix
Node *unary() {
    Token *tok;
    if (tok = consume(TK_RESERVED, "+")) {
        return unary();
    } else if (tok = consume(TK_RESERVED, "-")) {
        return new_node_binary(ND_SUB, new_node_num(0, NULL), unary(), tok);
    } else if (tok = consume(TK_RESERVED, "&")) {
        return new_node_unary(ND_ADDR, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "*")) {
        return new_node_unary(ND_DEREF, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "++")) {
        return new_node_unary(ND_PRE_INC, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "--")) {
        return new_node_unary(ND_PRE_DEC, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "sizeof")) {
        if (consume(TK_RESERVED, "(")) {
            if (at_typename()) {
                Node *node = new_node_num(read_base_type()->size, tok);
                expect(TK_RESERVED, ")");
                return node;
            } else {
                ctok = tok->next;
            }
        }
        return new_node_unary(ND_SIZEOF, unary(), tok);
    } else {
        return postfix();
    }
}

// postfix = primary ("[" assign "]" | "++" | "--" | "." | "->" ident)*
Node *postfix() {
    Node *node = primary();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "[")) {
            Node *addr = new_node_binary(ND_ADD, node, assign(), tok);
            node = new_node_unary(ND_DEREF, addr, tok);
            expect(TK_RESERVED, "]");
            continue;
        }

        if (tok = consume(TK_RESERVED, "++")) {
            node = new_node_unary(ND_POST_INC, node, tok);
            continue;
        }

        if (tok = consume(TK_RESERVED, "--")) {
            node = new_node_unary(ND_POST_DEC, node, tok);
            continue;
        }

        if (tok = consume(TK_RESERVED, ".")) {
            node = new_node_unary(ND_MEMBER, node, tok);
            node->member_name = expect(TK_IDENT, NULL)->str;
            continue;
        }

        if (tok = consume(TK_RESERVED, "->")) {
            node = new_node_unary(ND_DEREF, node, tok);
            node = new_node_unary(ND_MEMBER, node, tok);
            node->member_name = expect(TK_IDENT, NULL)->str;
            continue;
        }

        return node;
    }
}

// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | ident ("(" ")")?
//         | str
//         | num
Node *primary() {
    Token *tok = ctok;

    bool is_stmt_expr = consume(TK_RESERVED, "(") && consume(TK_RESERVED, "{");
    ctok = tok;

    if (is_stmt_expr) {
        return stmt_expr();
    }

    if (tok = consume(TK_RESERVED, "(")) {
        Node *node = expr();
        expect(TK_RESERVED, ")");
        return node;
    }

    if (tok = consume(TK_IDENT, NULL)) {
        if (peek(TK_RESERVED, "(")) {
            return new_node_func_call(tok);
        } else {
            Var *var = find_var(tok->str);
            if (!var) {
                error_at(tok->loc, "error: '%s' undeclared\n", tok->str);
            }
            return new_node_varref(var, tok);
        }
    }

    if (tok = consume(TK_RESERVED, "\"")) {
        tok = expect(TK_STR, NULL);
        expect(TK_RESERVED, "\"");
        return new_node_varref(new_strl(tok->str, tok), tok);
    }

    tok = expect(TK_NUM, NULL);
    return new_node_num(tok->val, tok);
}

// program = top-level*
Prog *parse() {
    prog = calloc(1, sizeof(Prog));
    var_scope = new_var_scope();
    tag_scope = new_tag_scope();
    prog->fns = map_create();
    prog->gvars = vec_create();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}
