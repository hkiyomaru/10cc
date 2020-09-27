#include "10cc.h"

int nlabel = 1;

Program *prog;  // The program
Function *fn;   // The function being parsed

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

/**
 * Creates a type.
 * @param type A type ID.
 * @param size Required bytes to represent the type.
 * @return A type.
 */
Type *new_type(TypeKind type, int size) {
    Type *ret = calloc(1, sizeof(Type));
    ret->kind = type;
    ret->size = size;
    ret->align = size;
    return ret;
}

/**
 * Creates an INT type.
 * @return An INT type.
 */
Type *int_type() { return new_type(TY_INT, 4); }

/**
 * Creates a CHAR type.
 * @return A CHAR type.
 */
Type *char_type() { return new_type(TY_CHAR, 1); }

/**
 * Creates a PTR type.
 * @param base The type to be pointed.
 * @return A PTR type.
 */
Type *ptr_to(Type *base) {
    Type *type = new_type(TY_PTR, 8);
    type->base = base;
    return type;
}

/**
 * Creates an ARY type.
 * @param base The type of each item.
 * @param size The number of items.
 * @return An ARY type.
 */
Type *ary_of(Type *base, int len) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_ARY;
    type->size = base->size * len;
    type->align = base->align;
    type->base = base;
    type->array_size = len;
    return type;
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
        error_at(token->loc, "error: unknown type name '%s'", token->str);
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
    if (consume(TK_RESERVED, "[")) {
        if (consume(TK_RESERVED, "]")) {
            type = ary_of(type, 0);
        } else {
            type = ary_of(type, expect(TK_NUM, NULL)->val);
            expect(TK_RESERVED, "]");
        }
    }
    return type;
}

/**
 * If there exists a variable whose name matches given conditions,
 * returns it. Otherwise, NULL will be returned. The lookup process
 * priotizes local variables than global variables.
 * @param name The name of a varaible.
 * @return A variable.
 */
Var *find_var(char *name) {
    if (fn && fn->lvars && map_count(fn->lvars, name)) {
        return map_at(fn->lvars, name);
    }
    if (map_count(prog->gvars, name)) {
        return map_at(prog->gvars, name);
    }
    return NULL;
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
    map_insert(prog->gvars, name, var);
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
    map_insert(fn->lvars, name, var);
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
    char *name = format(".L.str%d", nlabel++);
    Var *var = new_gvar(type, name, tok);
    var->data = tok->cont;
    return new_node_varref(var, tok);
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
        error_at(tok->loc, "error: undefined reference to `%s'", tok->str);
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
        while (!consume(TK_RESERVED, "}")) {
            vec_push(node->stmts, (void *)stmt());
        }
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
    vec_push(node->stmts, stmt());
    while (!consume(TK_RESERVED, "}")) {
        vec_push(node->stmts, stmt());
    }

    Node *last = vec_back(node->stmts);
    if (last->kind != ND_EXPR_STMT) {
        error_at(last->tok->loc, "error: void value not ignored as it ought to be");
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
        return new_node_bin_op(ND_ADDR, unary(), NULL, tok);
    } else if (tok = consume(TK_RESERVED, "*")) {
        return new_node_bin_op(ND_DEREF, unary(), NULL, tok);
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
        return new_node_bin_op(ND_SIZEOF, unary(), NULL, tok);
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
            node = new_node_bin_op(ND_DEREF, addr, NULL, tok);
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
            Var *var = find_var(tok->str);
            if (!var) {
                error_at(tok->loc, "error: '%s' undeclared", tok->str);
            }
            return new_node_varref(var, tok);
        }
    }

    if (tok = consume(TK_STR, NULL)) {
        return new_node_string(tok->cont, tok);
    }

    tok = expect(TK_NUM, NULL);
    return new_node_num(tok->val, tok);
}

/**
 * Parses a function and registers it to the program.
 * Here, the function name has already been consumed.
 * So, the current token must be '('.
 * @param type The type of a return variable.
 * @param tok The name of a function.
 * @return A function.
 */
Function *new_func(Type *rtype, Token *tok) {
    fn = find_func(tok->str);
    if (fn) {
        if (fn->body) {
            error_at(tok->loc, "error: redefinition of %s", tok->str);
        }

        // Confirm that arguments' types are correct.
        expect(TK_RESERVED, "(");
        for (int i = 0; i < fn->args->len; i++) {
            if (0 < i) {
                expect(TK_RESERVED, ",");
            }
            Var *var = declaration();
            if (var->type->kind != ((Node *)vec_at(fn->args, i))->type->kind) {
                error_at(tok->loc, "error: conflicting types for '%s'", tok->str);
            }
        }
        expect(TK_RESERVED, ")");
    } else {
        fn = calloc(1, sizeof(Function));
        fn->rtype = rtype;
        fn->name = tok->str;
        fn->lvars = map_create();

        // Parse the arguments.
        fn->args = vec_create();
        expect(TK_RESERVED, "(");
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
    }

    // If this is a prototype declaration, exit.
    if (consume(TK_RESERVED, ";")) {
        return fn;
    }

    // Parse the body.
    tok = expect(TK_RESERVED, "{");
    fn->body = new_node(ND_BLOCK, tok);
    fn->body->stmts = vec_create();
    while (!consume(TK_RESERVED, "}")) {
        vec_push(fn->body->stmts, (void *)stmt());
    }
    return fn;
}

/**
 * Parses tokens at the top level of a program.
 */
void top_level() {
    Type *type = read_type();
    Token *tok = expect(TK_IDENT, NULL);
    if (peek(TK_RESERVED, "(")) {
        new_func(type, tok);
    } else {
        if (map_count(prog->gvars, tok->str)) {
            error_at(tok->loc, "error: redefinition of %s", tok->str);
        }
        type = read_array(type);
        new_gvar(type, tok->str, tok);
        expect(TK_RESERVED, ";");
    }
}

/**
 * Parses tokens syntactically.
 * @return A program.
 */
Program *parse() {
    prog = calloc(1, sizeof(Program));
    prog->fns = map_create();
    prog->gvars = map_create();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}
