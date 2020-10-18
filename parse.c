#include "10cc.h"

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    Var *var;
};

typedef struct {
    VarScope *var_scope;
} Scope;

Program *prog;  // The program
Function *fn;   // The function being parsed

VarScope *var_scope;

int str_label_cnt = 1;

void top_level();
Function *func(Type *rtype, Token *tok);
Node *stmt();
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

Type *read_type();
Type *read_array(Type *type);

Node *new_node_bin_op(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_node_unary_op(NodeKind kind, Node *lhs, Token *tok);
Node *new_node_num(int val, Token *tok);
Node *new_node_string(char *str, Token *tok);
Node *new_node_varref(Var *var, Token *tok);
Node *new_node_func_call(Token *tok);

Var *new_gvar(Type *type, char *name, Token *tok);
Var *new_lvar(Type *type, char *name, Token *tok);
Var *declaration();

Function *find_func(char *name);

Scope *enter_scope();
void leave_scope(Scope *sc);
VarScope *push_scope(Var *var);
VarScope *find_var(char *name);

/**
 * Parses tokens syntactically.
 * @return A program.
 */
Program *parse() {
    prog = calloc(1, sizeof(Program));
    prog->fns = map_create();
    prog->gvars = vec_create();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}

/**
 * Parses tokens at the top level of a program.
 */
void top_level() {
    Type *type = read_type();
    Token *tok = expect(TK_IDENT, NULL);
    if (peek(TK_RESERVED, "(")) {
        func(type, tok);
    } else {
        if (find_var(tok->str)) {
            error_at(tok->loc, "error: redefinition of %s\n", tok->str);
        }
        type = read_array(type);
        new_gvar(type, tok->str, tok);
        expect(TK_RESERVED, ";");
    }
}

/**
 * Parses a function and registers it to the program.
 * Here, the function name has already been consumed.
 * So, the current token must be '('.
 * @param type The type of a return variable.
 * @param tok The name of a function.
 * @return A function.
 */
Function *func(Type *rtype, Token *tok) {
    Function *fn_ = find_func(tok->str);
    if (fn_) {
        if (fn_->body) {
            error_at(tok->loc, "error: redefinition of %s\n", tok->str);
        }
        // TODO: Confirm that types are sane.
    }

    fn = calloc(1, sizeof(Function));
    fn->rtype = rtype;
    fn->name = tok->str;
    fn->lvars = vec_create();

    // Parse the arguments.
    fn->args = vec_create();
    expect(TK_RESERVED, "(");
    Scope *sc = enter_scope();
    while (!consume(TK_RESERVED, ")")) {
        if (0 < fn->args->len) {
            expect(TK_RESERVED, ",");
        }
        tok = token;
        vec_push(fn->args, new_node_varref(declaration(), tok));
    }

    // NOTE: functions must be registered before parsing their bodies
    // to deal with recursion.
    map_insert(prog->fns, fn->name, fn);

    // If this is a prototype declaration, exit.
    if (consume(TK_RESERVED, ";")) {
        leave_scope(sc);
        return fn;
    }

    // Parse the body.
    tok = expect(TK_RESERVED, "{");
    fn->body = new_node(ND_BLOCK, tok);
    fn->body->stmts = vec_create();
    while (!consume(TK_RESERVED, "}")) {
        vec_push(fn->body->stmts, (void *)stmt());
    }
    leave_scope(sc);
    return fn;
}

/**
 * Parses tokens to represent a statement, where
 *     stmt = expr ";"
 *          | "{" stmt* "}"
 *          | "if" "(" expr ")" stmt ("else" stmt)?
 *          | "while" "(" expr ")" stmt
 *          | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *          | "return" expr ";"
 *          | T ident ";"
 *          | ";"
 * @return A node.
 */
Node *stmt() {
    Token *tok;
    if (tok = consume(TK_RESERVED, "{")) {
        Node *node = new_node(ND_BLOCK, tok);
        node->stmts = vec_create();
        Scope *sc = enter_scope();
        while (!consume(TK_RESERVED, "}")) {
            vec_push(node->stmts, (void *)stmt());
        }
        leave_scope(sc);
        return node;
    }

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
        if (consume(TK_RESERVED, "else")) {
            node->els = stmt();
        }
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
        Scope *sc = enter_scope();
        if (!consume(TK_RESERVED, ";")) {
            node->init = expr_stmt();
            expect(TK_RESERVED, ";");
        }
        if (!consume(TK_RESERVED, ";")) {
            node->cond = expr();
            expect(TK_RESERVED, ";");
        }
        if (!consume(TK_RESERVED, ")")) {
            node->upd = expr_stmt();
        }
        expect(TK_RESERVED, ")");
        node->then = stmt();
        leave_scope(sc);
        return node;
    }

    if (at_typename()) {
        declaration();
        expect(TK_RESERVED, ";");
        return new_node(ND_NULL, tok);
    }

    if (tok = consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL, tok);
    }

    Node *node = expr_stmt();
    expect(TK_RESERVED, ";");
    return node;
}

/**
 * Parses tokens to represent an expression, where
 *     expr = assign
 * @return A node.
 */
Node *expr() { return assign(); }

/**
 * Creates a node to represent an expression statement.
 * @return A node.
 */
Node *expr_stmt() { return new_node_unary_op(ND_EXPR_STMT, expr(), token); }

/**
 * Parses tokens to represent a statement expression, where
 *     stmt_expr = "(" "{" stmt+ "}" ")"
 * @return A node.
 * @note Parentheses are consumed outside of this function.
 */
Node *stmt_expr() {
    Token *tok = expect(TK_RESERVED, "{");

    Node *node = new_node(ND_STMT_EXPR, tok);
    node->stmts = vec_create();

    Scope *sc = enter_scope();
    vec_push(node->stmts, stmt());
    while (!consume(TK_RESERVED, "}")) {
        vec_push(node->stmts, stmt());
    }
    leave_scope(sc);

    Node *last = vec_back(node->stmts);
    if (last->kind != ND_EXPR_STMT) {
        error_at(last->tok->loc, "error: void value not ignored as it ought to be\n");
    }
    memcpy(last, last->lhs, sizeof(Node));
    return node;
}

/**
 * Parses tokens to represent an assignment, where
 *     assign = equality ("=" assign)?
 * @return A node.
 */
Node *assign() {
    Node *node = equality();
    Token *tok;
    if (tok = consume(TK_RESERVED, "=")) {
        node = new_node_bin_op(ND_ASSIGN, node, assign(), tok);
    }
    return node;
}

/**
 * Parses tokens to represent an equality, where
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
 * Parses tokens to represent a relationality, where
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
 * Parses tokens to represent addition, where
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
 * Parses tokens to represent multiplication, where
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
 * Parses tokens to represent an unary operation, where
 *     unary = "sizeof" unary
 *           | "sizeof" "(" type-name ")"
 *           | ("+" | "-" | "&" | "*")? primary
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
    } else if (tok = consume(TK_RESERVED, "sizeof")) {
        Node *node;
        if (consume(TK_RESERVED, "(")) {
            if (at_typename()) {
                node = new_node_num(read_type()->size, tok);
                expect(TK_RESERVED, ")");
                return node;
            } else {
                token = tok->next;
            }
        }
        return new_node_unary_op(ND_SIZEOF, unary(), tok);
    } else {
        return postfix();
    }
}

/**
 * Parses tokens to represent a primary item.
 * Then, if possible, parses the postfix, where
 *     postfix = primary ("[" assign "]")*
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
        return node;
    }
}

/**
 * Parses tokens to represent a primary item, where
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
        Node *node;
        if (peek(TK_RESERVED, "{")) {
            node = stmt_expr();
        } else {
            node = expr();
        }
        expect(TK_RESERVED, ")");
        return node;
    }

    if (tok = consume(TK_IDENT, NULL)) {
        if (peek(TK_RESERVED, "(")) {
            return new_node_func_call(tok);
        } else {
            VarScope *sc = find_var(tok->str);
            if (sc->var) {
                return new_node_varref(sc->var, tok);
            } else {
                error_at(tok->loc, "error: '%s' undeclared\n", tok->str);
            }
        }
    }

    if (tok = consume(TK_RESERVED, "\"")) {
        tok = expect(TK_STR, NULL);
        expect(TK_RESERVED, "\"");
        return new_node_string(tok->str, tok);
    }

    tok = expect(TK_NUM, NULL);
    return new_node_num(tok->val, tok);
}

/**
 * Parses tokens to represent a type.
 * @return A type.
 */
Type *read_type() {
    Type *type;
    if (consume(TK_RESERVED, "int")) {
        type = int_type();
    } else if (consume(TK_RESERVED, "char")) {
        type = char_type();
    } else {
        error_at(token->loc, "error: unknown type name '%s'\n", token->str);
    }
    while ((consume(TK_RESERVED, "*"))) {
        type = ptr_to(type);
    }
    return type;
}

/**
 * Parses tokens to represent an array.
 * @param type The type of each item.
 * @return A type.
 */
Type *read_array(Type *type) {
    Vector *sizes = vec_create();
    while (consume(TK_RESERVED, "[")) {
        Token *tok = consume(TK_NUM, NULL);
        vec_pushi(sizes, tok ? tok->val : -1);  // TODO
        expect(TK_RESERVED, "]");
    }
    for (int i = sizes->len - 1; i >= 0; i--) {
        type = ary_of(type, vec_ati(sizes, i));
    }
    return type;
}

/**
 * Creates a node.
 * @param kind The kind of a node.
 * @param tok The representative token of a node.
 * @return A node.
 */
Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

/**
 * Creates a node to represent a binary operation.
 * @param kind The kind of a node.
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
 * Creates a node to represent a unary operation.
 * @param kind The kind of a node.
 * @param lhs The input of a unary operation.
 * @param tok A representative token.
 * @return A node.
 */
Node *new_node_unary_op(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

/**
 * Creates a node to represent a number.
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
 * Creates a node to represent a string literal.
 * @param str A string literal.
 * @param tok A string token.
 * @return A node.
 */
Node *new_node_string(char *str, Token *tok) {
    Type *type = ary_of(char_type(), strlen(str) + 1);
    char *name = format(".L.str%d", str_label_cnt++);
    Var *var = new_gvar(type, name, tok);
    var->data = tok->str;
    return new_node_varref(var, tok);
}

/**
 * Creates a node to refer a variable.
 * @param var A variable to be refered.
 * @param tok An representative token.
 * @return A node.
 */
Node *new_node_varref(Var *var, Token *tok) {
    Node *node = new_node(ND_VARREF, NULL);
    node->type = var->type;
    node->var = var;
    return node;
}

/**
 * Creates a node to represent a function call.
 * Here, the function name has already been consumed.
 * So, the current token must be '('.
 * @param tok An identifiier token.
 * @return A node.
 */
Node *new_node_func_call(Token *tok) {
    Function *fn_ = find_func(tok->str);
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
 * Creates a variable.
 * @param type The type of a global variable.
 * @param name A name of a variable.
 * @param is_local If true, creates a local variable.
 * @return A variable.
 */
Var *new_var(Type *type, char *name, bool is_local) {
    Var *var = calloc(1, sizeof(Var));
    var->type = type;
    var->name = name;
    var->is_local = is_local;
    return var;
}

/**
 * Creates a node to represent a global variable.
 * The created variable is added into the list of global variables.
 * @param type The type of a global variable.
 * @param name The name of a global variable.
 * @param tok An identifiier token.
 * @return A variable.
 * @note `new_node_string` also registers variables to the global variable map.
 */
Var *new_gvar(Type *type, char *name, Token *tok) {
    Var *var = new_var(type, name, false);
    vec_push(prog->gvars, var);
    push_scope(var);
    return var;
}

/**
 * Creates a node to represent a local variable.
 * The created variable is added into the list of local variables.
 * @param type The type of a local variable.
 * @param name The name of a global variable.
 * @param tok An identifiier token.
 * @return A variable.
 */
Var *new_lvar(Type *type, char *name, Token *tok) {
    Var *var = new_var(type, name, true);
    vec_push(fn->lvars, var);
    push_scope(var);
    return var;
}

/**
 * Parses tokens to represent a declaration, where
 *     declaration = T ident ("[" num? "]")?
 * @return A variable.
 */
Var *declaration() {
    Type *type = read_type();
    Token *tok = expect(TK_IDENT, NULL);
    if (find_var(tok->str)) {
        // TODO: Resurrect this assertion by implementing variable scope.
        // error_at(tok->loc, "error: redeclaration of '%s'", tok->str);
    }
    type = read_array(type);
    return new_lvar(type, tok->str, tok);
}

/**
 * If there exists a function whose name matches given condition,
 * returns it. Otherwise, NULL will be returned.
 * @param name The name of a function.
 * @return A function.
 */
Function *find_func(char *name) {
    if (map_count(prog->fns, name)) {
        return map_at(prog->fns, name);
    } else {
        return NULL;
    }
}

/**
 * Enters scope.
 * @return A scope.
 */
Scope *enter_scope() {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->var_scope = var_scope;
    return sc;
}

/**
 * Leaves scope.
 * @param sc A scope.
 */
void leave_scope(Scope *sc) { var_scope = sc->var_scope; }

/**
 * Pushes a variable scope.
 * @param var A variable.
 */
VarScope *push_scope(Var *var) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->next = var_scope;
    sc->var = var;
    var_scope = sc;
    return sc;
}

/**
 * Finds a variable.
 * @param name A variable name.
 * @return A variable scope.
 */
VarScope *find_var(char *name) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (!strcmp(name, sc->var->name)) {
            return sc;
        }
    }
    return NULL;
}
