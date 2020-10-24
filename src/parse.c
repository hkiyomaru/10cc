#include "10cc.h"

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    Map *vars;
};

Prog *prog;  // The program
Func *fn;    // The function being parsed

VarScope *scope;

int str_label_cnt = 1;

void top_level();
Func *new_func(Type *rtype, char *name, Token *tok);
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

Type *read_base_type();
Type *read_type_postfix(Type *type);

Node *new_node_bin_op(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_unary_op(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);
Node *new_node_varref(Var *var, Token *tok);
Node *new_node_func_call(Token *tok);
Node *lvar_init(Var *var, Token *tok);
Node *default_lvar_init(Var *var, Token *tok);
Node *ary_init(Var *var, Token *tok);

Var *new_gvar(Type *type, char *name, Token *tok);
Var *new_lvar(Type *type, char *name, Token *tok);
Var *new_strl(char *str, Token *tok);
Var *decl();

Func *find_func(char *name);

VarScope *new_scope();
void *enter_scope();
void leave_scope();
void *push_scope(Var *var);
Var *find_var(char *name);

/**
 * Return a program, where
 *     program = (func | gvar)*
 * @return A program.
 */
Prog *parse() {
    prog = calloc(1, sizeof(Prog));
    scope = new_scope();
    prog->fns = map_create();
    prog->gvars = vec_create();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}

/**
 * Register a top-level component to the program, where
 *     top-level = func | gvar
 */
void top_level() {
    Type *type = read_base_type();
    Token *tok = expect(TK_IDENT, NULL);
    type = read_type_postfix(type);
    if (peek(TK_RESERVED, "(")) {
        new_func(type, tok->str, tok);
    } else {
        new_gvar(type, tok->str, tok);
        expect(TK_RESERVED, ";");
    }
}

/**
 * Return a function and register it to the program.
 * @param type The type of a return variable.
 * @param name A function name.
 * @param tok The name of a function.
 * @return A function.
 * @note The function name has already been consumed, the current token must be '('.
 */
Func *new_func(Type *rtype, char *name, Token *tok) {
    fn = calloc(1, sizeof(Func));
    fn->rtype = rtype;
    fn->name = name;
    fn->tok = tok;
    fn->lvars = vec_create();

    // Parse the arguments.
    fn->args = vec_create();
    expect(TK_RESERVED, "(");
    enter_scope();
    while (!consume(TK_RESERVED, ")")) {
        if (0 < fn->args->len) {
            expect(TK_RESERVED, ",");
        }
        vec_push(fn->args, new_node_varref(decl(), ctok));
    }

    // Check conflict.
    Func *fn_ = find_func(name);
    if (fn_) {
        if (fn_->body) {
            error_at(fn->tok->loc, "error: redefinition of '%s'\n", tok->str);
        }
        if (!is_same_type(fn->rtype, fn_->rtype)) {
            error_at(fn->tok->loc, "error: conflicting types for '%s'", tok->str);
        }
        if (fn->args->len != fn_->args->len) {
            error_at(fn->tok->loc, "error: conflicting types for '%s'", tok->str);
        }
        for (int i = 0; i < fn->args->len; i++) {
            if (!is_same_type((Type *)vec_at(fn->args, i), (Type *)vec_at(fn_->args, i))) {
                error_at(fn->tok->loc, "error: conflicting types for '%s'", tok->str);
            }
        }
    }

    // NOTE: Functions must be registered before parsing their bodies
    // to deal with recursion.
    map_insert(prog->fns, name, fn);

    // If this is a prototype declaration, exit.
    if (consume(TK_RESERVED, ";")) {
        leave_scope();
        return fn;
    }

    // Parse the body.
    fn->body = compound_stmt();
    leave_scope();
    return fn;
}

/**
 * Return a statement, where
 *     stmt = "return" expr ";"
 *          | "if" "(" expr ")" stmt ("else" stmt)?
 *          | "while" "(" expr ")" stmt
 *          | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *          | compound-stmt
 *          | T ident ("=" expr)? ";"
 *          | ";"
 *          | expr_stmt ";"
 * @return A node.
 */
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
        node->init = peek(TK_RESERVED, ";") ? new_node(ND_NULL, NULL) : expr_stmt();
        expect(TK_RESERVED, ";");
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
        Var *var = decl();
        Node *node = consume(TK_RESERVED, "=") ? lvar_init(var, ctok) : new_node(ND_NULL, ctok);
        expect(TK_RESERVED, ";");
        return node;
    }

    if (tok = consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL, tok);
    }

    Node *node = expr_stmt();
    expect(TK_RESERVED, ";");
    return node;
}

/**
 * Return a compound statement, where
 *     compound-stmt = "{" stmt* "}"
 */
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

/**
 * Return an expression, where
 *     expr = assign
 * @return A node.
 */
Node *expr() { return assign(); }

/**
 * Return an expression statement, where
 *     expr_stmt = expr ";"
 * @return A node.
 */
Node *expr_stmt() { return new_node_unary_op(ND_EXPR_STMT, expr(), ctok); }

/**
 * Return a statement expression, where
 *     stmt_expr = "(" "{" stmt+ "}" ")"
 * @return A node.
 * @note Parentheses are consumed outside of this function.
 */
Node *stmt_expr() {
    Node *node = new_node(ND_STMT_EXPR, ctok);
    node->stmts = compound_stmt()->stmts;
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

/**
 * Return an assignment expression, where
 *     assign = equality (("=" | "+=", "-=") assign)?
 * @return A node.
 */
Node *assign() {
    Node *node = equality();
    Token *tok;

    if (tok = consume(TK_RESERVED, "=")) {
        return new_node_bin_op(ND_ASSIGN, node, assign(), tok);
    }

    if (tok = consume(TK_RESERVED, "+=")) {
        return new_node_bin_op(ND_ASSIGN, node, new_node_bin_op(ND_ADD, node, assign(), tok), tok);
    }

    if (tok = consume(TK_RESERVED, "-=")) {
        return new_node_bin_op(ND_ASSIGN, node, new_node_bin_op(ND_SUB, node, assign(), tok), tok);
    }

    if (tok = consume(TK_RESERVED, "*=")) {
        return new_node_bin_op(ND_ASSIGN, node, new_node_bin_op(ND_MUL, node, assign(), tok), tok);
    }

    if (tok = consume(TK_RESERVED, "/=")) {
        return new_node_bin_op(ND_ASSIGN, node, new_node_bin_op(ND_DIV, node, assign(), tok), tok);
    }

    return node;
}

/**
 * Return an equality expression, where
 *     equality = relational (("==" | "!=") relational)?
 * @return A node.
 */
Node *equality() {
    Node *node = relational();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "==")) {
            node = new_node_bin_op(ND_EQ, node, relational(), tok);
        } else if (tok = consume(TK_RESERVED, "!=")) {
            node = new_node_bin_op(ND_NE, node, relational(), tok);
        } else {
            return node;
        }
    }
}

/**
 * Return a relational expression, where
 *     relational = add (("<=" | ">=" | "<" | ">") add)?
 * @return A node.
 */
Node *relational() {
    Node *node = add();
    Token *tok;
    if (tok = consume(TK_RESERVED, "<=")) {
        return new_node_bin_op(ND_LE, node, add(), tok);
    } else if (tok = consume(TK_RESERVED, ">=")) {
        return new_node_bin_op(ND_LE, add(), node, tok);
    } else if (tok = consume(TK_RESERVED, "<")) {
        return new_node_bin_op(ND_LT, node, add(), tok);
    } else if (tok = consume(TK_RESERVED, ">")) {
        return new_node_bin_op(ND_LT, add(), node, tok);
    } else {
        return node;
    }
}

/**
 * Return an add expression, where
 *     add = mul ("+" unary | "-" unary)*
 * @return A node.
 */
Node *add() {
    Node *node = mul();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "+")) {
            node = new_node_bin_op(ND_ADD, node, mul(), tok);
        } else if (tok = consume(TK_RESERVED, "-")) {
            node = new_node_bin_op(ND_SUB, node, mul(), tok);
        } else {
            return node;
        }
    }
}

/*
 * Return a mul expression, where
 *     mul = unary (("*" | "/") unary)*
 * @return A node.
 */
Node *mul() {
    Node *node = unary();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "*")) {
            node = new_node_bin_op(ND_MUL, node, unary(), tok);
        } else if (tok = consume(TK_RESERVED, "/")) {
            node = new_node_bin_op(ND_DIV, node, unary(), tok);
        } else {
            return node;
        }
    }
}

/*
 * Return an unary expression, where
 *     unary = ("+" | "-" | "&" | "*")? unary
 *           | ("++", "--") unary
 *           | "sizeof" unary
 *           | "sizeof" "(" type-name ")"
 *           | postfix
 * @return A node.
 */
Node *unary() {
    Token *tok;
    if (tok = consume(TK_RESERVED, "+")) {
        return unary();
    } else if (tok = consume(TK_RESERVED, "-")) {
        return new_node_bin_op(ND_SUB, new_node_num(0, NULL), unary(), tok);
    } else if (tok = consume(TK_RESERVED, "&")) {
        return new_node_unary_op(ND_ADDR, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "*")) {
        return new_node_unary_op(ND_DEREF, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "++")) {
        return new_node_unary_op(ND_PRE_INC, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "--")) {
        return new_node_unary_op(ND_PRE_DEC, unary(), tok);
    } else if (tok = consume(TK_RESERVED, "sizeof")) {
        Node *node;
        if (consume(TK_RESERVED, "(")) {
            if (at_typename()) {
                node = new_node_num(read_base_type()->size, tok);
                expect(TK_RESERVED, ")");
                return node;
            } else {
                ctok = tok->next;
            }
        }
        return new_node_unary_op(ND_SIZEOF, unary(), tok);
    } else {
        return postfix();
    }
}

/**
 * Return a postfix expression, where
 *     postfix = primary ("[" assign "]" | "++" | "--")*
 * @return A node.
 */
Node *postfix() {
    Node *node = primary();
    Token *tok;
    for (;;) {
        if (tok = consume(TK_RESERVED, "[")) {
            Node *addr = new_node_bin_op(ND_ADD, node, assign(), tok);
            node = new_node_unary_op(ND_DEREF, addr, tok);
            expect(TK_RESERVED, "]");
            continue;
        }

        if (tok = consume(TK_RESERVED, "++")) {
            node = new_node_unary_op(ND_POST_INC, node, tok);
            continue;
        }

        if (tok = consume(TK_RESERVED, "--")) {
            node = new_node_unary_op(ND_POST_DEC, node, tok);
            continue;
        }

        return node;
    }
}

/**
 * Return a primary expression, where
 *     primary = num
 *             | str
 *             | ident ("(" ")")?
 *             | "(" "{" stmt+ "}" ")"
 *             | "(" expr ")"
 * @return A node.
 */
Node *primary() {
    Token *tok;
    if (tok = consume(TK_RESERVED, "(")) {
        Node *node = peek(TK_RESERVED, "{") ? stmt_expr() : expr();
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

/**
 * Read a base type.
 * @return A type.
 */
Type *read_base_type() {
    Type *type;
    if (consume(TK_RESERVED, "int")) {
        type = int_type();
    } else if (consume(TK_RESERVED, "char")) {
        type = char_type();
    } else if (consume(TK_RESERVED, "void")) {
        type = void_type();
    } else {
        error_at(ctok->loc, "error: unknown type name '%s'\n", ctok->str);
    }
    while ((consume(TK_RESERVED, "*"))) {
        type = ptr_to(type);
    }
    return type;
}

/**
 * Read an array size.
 * @param base A base type.
 * @return A type.
 */
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

/**
 * Create a node.
 * @param kind A node kind.
 * @param tok A token.
 * @return A node.
 */
Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

/**
 * Create a node to represent a binary operation.
 * @param kind A node kind.
 * @param lhs The left-hand side of a binary operation.
 * @param rhs The right-hand side of a binary operation.
 * @param tok A binary operator token.
 * @return A node.
 */
Node *new_node_bin_op(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

/**
 * Create a node to represent a unary operation.
 * @param kind The kind of a node.
 * @param lhs The input of a unary operation.
 * @param tok A token.
 * @return A node.
 */
Node *new_node_unary_op(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

/**
 * Create a node to represent a number.
 * @param val A number.
 * @param tok A number token.
 * @return A node.
 */
Node *new_node_num(int val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->type = int_type();
    node->val = val;
    return node;
}

/**
 * Create a node to refer a variable.
 * @param var A variable to be refered.
 * @param tok A token.
 * @return A node.
 */
Node *new_node_varref(Var *var, Token *tok) {
    Node *node = new_node(ND_VARREF, NULL);
    node->type = var->type;
    node->var = var;
    return node;
}

/**
 * Create a node to represent a function call.
 * @param tok An identifiier token.
 * @return A node.
 * @note The function name has already been consumed, so the current token must be '('.
 */
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

/**
 * Create a node to assign an initial value to a given local variable.
 * @param var A local variable.
 * @param tok A token.
 * @return A node.
 */
Node *lvar_init(Var *var, Token *tok) {
    switch (var->type->kind) {
        case TY_ARY:
            return ary_init(var, tok);
        default:
            return default_lvar_init(var, tok);
    }
}

/**
 * Create a node to assign an initial value to a given local variable.
 * @param var A variable to be refered.
 * @param tok An representative token.
 * @return A node.
 */
Node *default_lvar_init(Var *var, Token *tok) {
    Node *lvar = new_node_varref(var, tok);
    Node *asgn = new_node_bin_op(ND_ASSIGN, lvar, expr(), tok);
    return new_node_unary_op(ND_EXPR_STMT, asgn, NULL);
}

/**
 * Create a node to assign an initial value to a given array variable.
 * @param var A variable to be refered.
 * @param tok A token.
 * @return A node.
 */
Node *ary_init(Var *var, Token *tok) {
    assert(var->type->kind == TY_ARY);

    Node *node = new_node(ND_BLOCK, tok);
    node->stmts = vec_create();
    Node *lvar = new_node_varref(var, tok);
    if (tok = consume(TK_RESERVED, "{")) {
        // {expr, ...}
        for (int i = 0;; i++) {
            if (consume(TK_RESERVED, "}")) {
                break;
            }
            if (node->stmts->len > 0) {
                tok = expect(TK_RESERVED, ",");
            }
            if (var->type->array_size != -1 && i >= var->type->array_size) {
                error_at(tok->loc, "error: excess elements in array initializer");
            }
            Node *index = new_node_num(i, NULL);
            Node *addr = new_node_bin_op(ND_ADD, lvar, index, NULL);
            Node *lval = new_node_unary_op(ND_DEREF, addr, NULL);
            Node *asgn = new_node_bin_op(ND_ASSIGN, lval, expr(), NULL);
            vec_push(node->stmts, new_node_unary_op(ND_EXPR_STMT, asgn, NULL));
        }
        if (lvar->type->array_size == -1) {
            lvar->type->array_size = node->stmts->len;
            lvar->type->size = lvar->type->base->size * node->stmts->len;
        }
    } else if (tok = consume(TK_RESERVED, "\"")) {
        // string literal
        Token *tok = expect(TK_STR, NULL);
        expect(TK_RESERVED, "\"");
        if (var->type->array_size != -1 && strlen(tok->str) > var->type->array_size - 1) {
            error_at(tok->loc, "error: initializer-string for array of chars is too long");
        }
        for (int i = 0; i < strlen(tok->str); i++) {
            Node *index = new_node_num(i, NULL);
            Node *addr = new_node_bin_op(ND_ADD, lvar, index, NULL);
            Node *lval = new_node_unary_op(ND_DEREF, addr, NULL);
            Node *asgn = new_node_bin_op(ND_ASSIGN, lval, new_node_num(tok->str[i], NULL), NULL);
            vec_push(node->stmts, new_node_unary_op(ND_EXPR_STMT, asgn, NULL));
        }
        Node *index = new_node_num(strlen(tok->str), NULL);
        Node *addr = new_node_bin_op(ND_ADD, lvar, index, NULL);
        Node *lval = new_node_unary_op(ND_DEREF, addr, NULL);
        Node *asgn = new_node_bin_op(ND_ASSIGN, lval, new_node_num(0, NULL), NULL);
        vec_push(node->stmts, new_node_unary_op(ND_EXPR_STMT, asgn, NULL));
        if (lvar->type->array_size == -1) {
            lvar->type->array_size = strlen(tok->str);
            lvar->type->size = lvar->type->base->size * strlen(tok->str);
        }
    } else {
        error_at(ctok->loc, "error: expected expression before '%s'", ctok->str);
    }
    for (int i = node->stmts->len; i < lvar->type->array_size; i++) {
        Node *index = new_node_num(i, NULL);
        Node *addr = new_node_bin_op(ND_ADD, lvar, index, NULL);
        Node *lval = new_node_unary_op(ND_DEREF, addr, NULL);
        Node *asgn = new_node_bin_op(ND_ASSIGN, lval, new_node_num(0, NULL), NULL);
        vec_push(node->stmts, new_node_unary_op(ND_EXPR_STMT, asgn, NULL));
    }
    return node;
}

/**
 * Create a variable.
 * @param type The type of a variable.
 * @param name Variable name.
 * @param is_local If true, created as a local variable.
 * @param tok A token.
 * @return A variable.
 */
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

/**
 * Create a global variable, and registers it to the program.
 * @param type The type of a variable.
 * @param name Variable name.
 * @param tok A token.
 * @return A variable.
 */
Var *new_gvar(Type *type, char *name, Token *tok) {
    if (map_contains(scope->vars, name)) {
        Var *var = map_at(scope->vars, name);
        if (!is_same_type(type, var->type)) {
            error_at(tok->loc, "error: conflicting types for '%s'", name);
        }
        return var;
    }
    Var *var = new_var(type, name, false, tok);
    vec_push(prog->gvars, var);
    push_scope(var);
    return var;
}

/**
 * Create a local variable, and registers it to the function.
 * @param type The type of a variable.
 * @param name Variable name.
 * @param tok A token.
 * @return A variable.
 */
Var *new_lvar(Type *type, char *name, Token *tok) {
    if (map_contains(scope->vars, tok->str)) {
        error_at(tok->loc, "error: redeclaration of '%s'", tok->str);
    }
    Var *var = new_var(type, name, true, tok);
    vec_push(fn->lvars, var);
    push_scope(var);
    return var;
}

/**
 * Create a string literal.
 * @param str A string.
 * @param tok A token.
 * @return A variable.
 */
Var *new_strl(char *str, Token *tok) {
    Type *type = ary_of(char_type(), strlen(str) + 1);
    char *name = format(".L.str%d", str_label_cnt++);
    Var *var = new_gvar(type, name, tok);
    var->data = tok->str;
    return var;
}

/**
 * Returns a local variable by reading its declaration, where
 *     declaration = T ident ("[" num? "]")?
 * @return A variable.
 */
Var *decl() {
    Token *tok = ctok;
    Type *type = read_base_type();
    tok = expect(TK_IDENT, NULL);
    type = read_type_postfix(type);
    return new_lvar(type, tok->str, tok);
}

/**
 * Return a function with a specific name.
 * If such a function does not exist, return NULL.
 * @param name The name of a function.
 * @return A function.
 */
Func *find_func(char *name) { return map_contains(prog->fns, name) ? map_at(prog->fns, name) : NULL; }

/**
 * Create an empty variable scope.
 * @return A variable scope.
 */
VarScope *new_scope() {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->next = NULL;
    sc->vars = map_create();
    return sc;
}

/**
 * Enter a new scope.
 */
void *enter_scope() {
    VarScope *sc = new_scope();
    sc->next = scope;
    scope = sc;
}

/**
 * Leave the current scope.
 */
void leave_scope() { scope = scope->next; }

/**
 * Push a variable to the current scope.
 * @param var A variable.
 */
void *push_scope(Var *var) { map_insert(scope->vars, var->name, var); }

/**
 * Return a variable with a specific name.
 * If such a variable does not exist, return NULL.
 * @param name Variable name.
 * @return A variable.
 */
Var *find_var(char *name) {
    for (VarScope *sc = scope; sc; sc = sc->next) {
        if (map_contains(sc->vars, name)) {
            return map_at(sc->vars, name);
        }
    }
    return NULL;
}