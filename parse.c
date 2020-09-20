#include "10cc.h"

Program *prog;  // The program
Function *fn;   // The function being parsed

Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *suffix();
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
        error_at(token->loc, "Invalid type");
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
 * @return A node.
 */
Node *find_var(char *name) {
    Node *var = NULL;
    if (fn && fn->lvars) {
        var = map_at(fn->lvars, name);
    }
    if (!var) {
        var = map_at(prog->gvars, name);
    }
    return var;
}

/**
 * If there exists a function whose name matches given condition,
 * returns it. Otherwise, NULL will be returned.
 * @param name The name of a function.
 * @return A function.
 */
Function *find_func(char *name) { return map_at(prog->fns, name); }

/**
 * Creates a node.
 * @param kind The kind of a node.
 * @return A node.
 */
Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

/**
 * Creates a node to represent a binary operation.
 * @param kind The kind of a node.
 * @param lhs The left-hand side of a binary operation.
 * @param rhs The right-hand side of a binary operation.
 * @return A node.
 */
Node *new_bin_op(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

/**
 * Creates a node to represent a number.
 * @param val A number.
 * @return A node.
 */
Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->type = int_type();
    node->val = val;
    return node;
}

/**
 * Creates a node to represent a global variable.
 * The created variable is added into the list of global variables.
 * @param type The type of a global variable.
 * @param name The name of a global variable.
 * @return A node.
 */
Node *new_node_gvar(Type *type, char *name) {
    Node *node = new_node(ND_GVAR);
    node->type = type;
    node->name = name;
    map_insert(prog->gvars, node->name, node);
    return node;
}

/**
 * Creates a node to represent a local variable.
 * The created variable is added into the list of local variables.
 * @param type The type of a local variable.
 * @param name The name of a local variable.
 * @return A node.
 */
Node *new_node_lvar(Type *type, char *name) {
    Node *node = new_node(ND_LVAR);
    node->type = type;
    node->name = name;
    map_insert(fn->lvars, node->name, node);
    return node;
}

/**
 * Creates a node to represent a function call.
 * Here, the function name has already been consumed.
 * So, the current token must be '('.
 * @param tok The name of a function.
 * @return A node.
 */
Node *new_node_func_call(Token *tok) {
    Function *fn_ = find_func(tok->str);
    if (!fn_) {
        error_at(tok->loc, "Undefined function");
    }

    Node *node = new_node(ND_FUNC_CALL);
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
 * Parses tokens to represent a declaration, where
 *     declaration = T ident ("[" num? "]")?
 * @return A node.
 */
Node *declaration() {
    Type *type = read_type();
    Token *tok = expect(TK_IDENT, NULL);
    if (find_var(tok->str)) {
        error_at(tok->loc, "Redefinition of '%s'", tok->str);
    }
    type = read_array(type);
    return new_node_lvar(type, tok->str);
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
    if (consume(TK_RESERVED, "{")) {
        Node *node = new_node(ND_BLOCK);
        node->stmts = vec_create();
        while (!consume(TK_RESERVED, "}")) {
            vec_push(node->stmts, (void *)stmt());
        }
        return node;
    }

    if (consume(TK_RESERVED, "return")) {
        Node *node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(TK_RESERVED, ";");
        return node;
    }

    if (consume(TK_RESERVED, "if")) {
        Node *node = new_node(ND_IF);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        if (consume(TK_RESERVED, "else")) {
            node->els = stmt();
        }
        return node;
    }

    if (consume(TK_RESERVED, "while")) {
        Node *node = new_node(ND_WHILE);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        return node;
    }

    if (consume(TK_RESERVED, "for")) {
        Node *node = new_node(ND_FOR);
        expect(TK_RESERVED, "(");
        if (!consume(TK_RESERVED, ";")) {
            node->init = expr();
            expect(TK_RESERVED, ";");
        }
        if (!consume(TK_RESERVED, ";")) {
            node->cond = expr();
            expect(TK_RESERVED, ";");
        }
        if (!consume(TK_RESERVED, ";")) {
            node->upd = expr();
        }
        expect(TK_RESERVED, ")");
        node->then = stmt();
        return node;
    }

    if (at_typename()) {
        Node *node = declaration();
        expect(TK_RESERVED, ";");
        return node;
    }

    if (consume(TK_RESERVED, ";")) {
        return new_node(ND_NULL);
    }

    Node *node = expr();
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
 * Parses tokens to represent an assignment, where
 *     assign = equality ("=" assign)?
 * @return A node.
 */
Node *assign() {
    Node *node = equality();
    if (consume(TK_RESERVED, "=")) {
        node = new_bin_op(ND_ASSIGN, node, assign());
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
    for (;;) {
        if (consume(TK_RESERVED, "==")) {
            node = new_bin_op(ND_EQ, node, relational());
        } else if (consume(TK_RESERVED, "!=")) {
            node = new_bin_op(ND_NE, node, relational());
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
    if (consume(TK_RESERVED, "<=")) {
        return new_bin_op(ND_LE, node, add());
    } else if (consume(TK_RESERVED, ">=")) {
        return new_bin_op(ND_LE, add(), node);
    } else if (consume(TK_RESERVED, "<")) {
        return new_bin_op(ND_LT, node, add());
    } else if (consume(TK_RESERVED, ">")) {
        return new_bin_op(ND_LT, add(), node);
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
    for (;;) {
        if (consume(TK_RESERVED, "+")) {
            node = new_bin_op(ND_ADD, node, mul());
        } else if (consume(TK_RESERVED, "-")) {
            node = new_bin_op(ND_SUB, node, mul());
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
    for (;;) {
        if (consume(TK_RESERVED, "*")) {
            node = new_bin_op(ND_MUL, node, unary());
        } else if (consume(TK_RESERVED, "/")) {
            node = new_bin_op(ND_DIV, node, unary());
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
    if (consume(TK_RESERVED, "+")) {
        return unary();
    } else if (consume(TK_RESERVED, "-")) {
        return new_bin_op(ND_SUB, new_node_num(0), unary());
    } else if (consume(TK_RESERVED, "&")) {
        return new_bin_op(ND_ADDR, unary(), NULL);
    } else if (consume(TK_RESERVED, "*")) {
        return new_bin_op(ND_DEREF, unary(), NULL);
    } else if (consume(TK_RESERVED, "sizeof")) {
        Node *node;
        if (consume(TK_RESERVED, "(")) {
            if (at_typename()) {
                node = new_node_num(read_type()->size);
            } else {
                node = new_bin_op(ND_SIZEOF, unary(), NULL);
            }
            expect(TK_RESERVED, ")");
            return node;
        }
        return new_bin_op(ND_SIZEOF, unary(), NULL);
    } else {
        return suffix();
    }
}

/**
 * Parses tokens to represent a primary item.
 * Then, if possible, parse the suffix, where
 *     suffix = primary ("[" assign "]")*
 * @return A node.
 */
Node *suffix() {
    Node *node = primary();
    for (;;) {
        if (consume(TK_RESERVED, "[")) {
            Node *addr = new_bin_op(ND_ADD, node, assign());
            node = new_bin_op(ND_DEREF, addr, NULL);
            expect(TK_RESERVED, "]");
            continue;
        }
        return node;
    }
}

/**
 * Parses tokens to represent a primary item, where
 *     primary = num
 *             | ident ("(" ")")?
 *             | "(" expr ")"
 * @return A node.
 */
Node *primary() {
    if (consume(TK_RESERVED, "(")) {
        Node *node = expr();
        expect(TK_RESERVED, ")");
        return node;
    }

    Token *tok = consume(TK_IDENT, NULL);
    if (tok) {
        if (peek(TK_RESERVED, "(")) {
            return new_node_func_call(tok);
        } else {
            Node *var = find_var(tok->str);
            if (!var) {
                error_at(tok->loc, "Undefined variable");
            }
            return var;
        }
    }

    return new_node_num(expect(TK_NUM, NULL)->val);
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
    fn = calloc(1, sizeof(Function));
    fn->rtype = rtype;
    fn->name = tok->str;
    fn->lvars = map_create();

    // Parse the arguments
    fn->args = vec_create();
    expect(TK_RESERVED, "(");
    while (!consume(TK_RESERVED, ")")) {
        if (0 < fn->args->len) {
            expect(TK_RESERVED, ",");
        }
        vec_push(fn->args, declaration());
    }

    // NOTE: functions must be registered before parsing their bodies
    // to deal with recursion
    map_insert(prog->fns, fn->name, fn);

    // Parse the body.
    fn->body = new_node(ND_BLOCK);
    fn->body->stmts = vec_create();
    expect(TK_RESERVED, "{");
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
        if (find_func(tok->str)) {
            error_at(tok->loc, "Redefinition of %s", tok->str);
        }
        new_func(type, tok);
    } else {
        if (find_var(tok->str)) {
            error_at(tok->loc, "Redefinition of %s", tok->str);
        }
        type = read_array(type);
        expect(TK_RESERVED, ";");
        new_node_gvar(type, tok->str);
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
