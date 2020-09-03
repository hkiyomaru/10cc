#include "9cc.h"

Program *prog;
Function *fn;

Vector *tokens;
int pos;

Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *suffix();
Node *primary();

Token *consume(TokenKind kind, char *str) {
    Token *token = vec_get(tokens, pos);
    if (token->kind != kind || (str && strcmp(token->str, str) != 0)) {
        return NULL;
    }
    pos++;
    return token;
}

Token *expect(TokenKind kind, char *str) {
    Token *token = vec_get(tokens, pos);
    if (token->kind != kind || (str && strcmp(token->str, str) != 0)) {
        error_at(token->loc, "Unexpected token");
    }
    pos++;
    return token;
}

bool at_eof() {
    Token *token = vec_get(tokens, pos);
    return token->kind == TK_EOF;
}

bool at_typename() {
    Token *token = vec_get(tokens, pos);
    return token->kind == TK_INT;
}

Type *new_ty(int ty, int size) {
    Type *ret = calloc(1, sizeof(Type));
    ret->ty = ty;
    ret->size = size;
    ret->align = size;
    return ret;
}

Type *int_ty() {
    Type *ty = new_ty(INT, 4);
    ty->align = 8;  // TODO
    return ty;
}

Type *ptr_to(Type *base) {
    Type *ty = new_ty(PTR, 8);
    ty->ptr_to = base;
    return ty;
}

Type *ary_of(Type *base, int size) {
    Type *ty = calloc(1, sizeof(Type));
    ty->ty = ARY;
    ty->size = base->size * size;
    ty->align = base->align;
    ty->ary_of = base;
    ty->array_size = size;
    return ty;
}

Type *read_ty() {
    Token *token = vec_get(tokens, pos);
    Type *ty;
    if (consume(TK_INT, NULL)) {
        ty = int_ty();
    } else {
        error_at(token->loc, "Invalid type");
    }
    while ((consume(TK_RESERVED, "*"))) {
        ty = ptr_to(ty);
    }
    return ty;
}

Type *read_array(Type *ty) {
    if (consume(TK_RESERVED, "[")) {
        if (consume(TK_RESERVED, "]")) {
            ty = ary_of(ty, 0);
        } else {
            ty = ary_of(ty, expect(TK_NUM, NULL)->val);
            expect(TK_RESERVED, "]");
        }
    }
    return ty;
}

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_bin_op(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    node->ty = int_ty();
    return node;
}

Node *new_node_gvar(Type *ty, char *name) {
    Node *node = new_node(ND_GVAR);
    node->ty = ty;
    node->name = name;
    map_insert(prog->gvars, node->name, node);
    return node;
}

Node *new_node_lvar(Type *ty, char *name) {
    Node *node = new_node(ND_LVAR);
    node->ty = ty;
    node->name = name;
    map_insert(fn->lvars, node->name, node);
    return node;
}

Node *new_node_func_call(Token *tok) {
    Vector *args = vec_create();
    while (!consume(TK_RESERVED, ")")) {
        if (0 < args->len) {
            expect(TK_RESERVED, ",");
        }
        vec_push(args, expr());
    }
    Function *fn_ = map_at(prog->fns, tok->str);
    if (!fn_) {
        error_at(tok->loc, "Undefined function");
    }
    Node *node = new_node(ND_FUNC_CALL);
    node->name = tok->str;
    node->ty = fn_->rty;
    node->args = args;
    return node;
}

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
 * declaration = T ident ("[" num? "]")?
 */
Node *declaration() {
    Type *ty = read_ty();
    Token *tok = expect(TK_IDENT, NULL);
    if (find_var(tok->str)) {
        error_at(tok->loc, "Redefinition of '%s'", tok->str);
    }
    ty = read_array(ty);
    return new_node_lvar(ty, tok->str);
}

/**
 * stmt = expr ";"
 *      | "{" stmt* "}"
 *      | "if" "(" expr ")" stmt ("else" stmt)?
 *      | "while" "(" expr ")" stmt
 *      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *      | "return" expr ";"
 *      | T ident ";"
 */
Node *stmt() {
    Node *node;
    if (consume(TK_RESERVED, "{")) {
        node = new_node(ND_BLOCK);
        node->stmts = vec_create();
        while (!consume(TK_RESERVED, "}")) {
            vec_push(node->stmts, (void *)stmt());
        }
        return node;
    } else if (consume(TK_RETURN, NULL)) {
        node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(TK_RESERVED, ";");
        return node;
    } else if (consume(TK_IF, NULL)) {
        node = new_node(ND_IF);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        if (consume(TK_ELSE, NULL)) {
            node->els = stmt();
        }
        return node;
    } else if (consume(TK_WHILE, NULL)) {
        node = new_node(ND_WHILE);
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        return node;
    } else if (consume(TK_FOR, NULL)) {
        node = new_node(ND_FOR);
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
        node = declaration();
        expect(TK_RESERVED, ";");
        return node;
    } else {
        node = expr();
        expect(TK_RESERVED, ";");
        return node;
    }
}

/**
 * expr = assign
 */
Node *expr() { return assign(); }

/**
 * assign = equality ("=" assign)?
 */
Node *assign() {
    Node *node = equality();
    if (consume(TK_RESERVED, "=")) {
        node = new_bin_op(ND_ASSIGN, node, assign());
    }
    return node;
}

/**
 * equality = relational (("==" | "!=") relational)?
 */
Node *equality() {
    Node *node = relational();
    if (consume(TK_RESERVED, "==")) {
        node = new_bin_op(ND_EQ, node, relational());
    } else if (consume(TK_RESERVED, "!=")) {
        node = new_bin_op(ND_NE, node, relational());
    } else {
        return node;
    }
}

/**
 * relational = add (("<=" | ">=" | "<" | ">") add)?
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
 * add = mul ("+" unary | "-" unary)*
 */
Node *add() {
    Node *node = mul();
    for (;;) {
        Node *rhs;
        if (consume(TK_RESERVED, "+")) {
            rhs = mul();
            node = new_bin_op(ND_ADD, node, rhs);
        } else if (consume(TK_RESERVED, "-")) {
            rhs = mul();
            node = new_bin_op(ND_SUB, node, rhs);
        } else {
            return node;
        }
    }
}

/*
 * mul = unary (("*" | "/") unary)*
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
 * unary = "sizeof" unary
 *       | ("+" | "-" | "&" | "*")? primary
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
    } else if (consume(TK_SIZEOF, NULL)) {
        return new_node_num(unary()->ty->size);
    } else {
        return suffix();
    }
}

Node *suffix() {
    Node *lhs = primary();
    for (;;) {
        if (consume(TK_RESERVED, "[")) {
            Node *node = new_bin_op(ND_ADD, lhs, assign());
            lhs = new_bin_op(ND_DEREF, node, NULL);
            expect(TK_RESERVED, "]");
            continue;
        }
        return lhs;
    }
}

/**
 * primary = num
 *         | ident ("(" ")")?
 *         | "(" expr ")"
 */
Node *primary() {
    if (consume(TK_RESERVED, "(")) {
        Node *node = expr();
        expect(TK_RESERVED, ")");
        return node;
    }

    // Variable reference or function call
    Token *tok = consume(TK_IDENT, NULL);
    if (tok) {
        if (consume(TK_RESERVED, "(")) {
            return new_node_func_call(tok);
        } else {
            Node *var = find_var(tok->str);
            if (!var) {
                error_at(tok->loc, "Undefined variable");
            }
            return var;
        }
    }

    // Number
    return new_node_num(expect(TK_NUM, NULL)->val);
}

void top_level() {
    Type *ty = read_ty();
    Token *tok = expect(TK_IDENT, NULL);

    if (consume(TK_RESERVED, "(")) {
        // Function.
        fn = calloc(1, sizeof(Function));
        fn->rty = ty;
        fn->name = tok->str;
        fn->args = vec_create();
        fn->lvars = map_create();

        // Arguments.
        while (!consume(TK_RESERVED, ")")) {
            if (0 < fn->args->len) {
                expect(TK_RESERVED, ",");
            }
            vec_push(fn->args, declaration());
        }

        // NOTE: functions must be registered before parsing their bodies to deal with recursion
        map_insert(prog->fns, fn->name, fn);

        // Body.
        fn->body = vec_create();
        expect(TK_RESERVED, "{");
        while (!consume(TK_RESERVED, "}")) {
            vec_push(fn->body, stmt());
        }
        return;
    } else {
        // Global variable
        if (find_var(tok->str)) {
            error_at(tok->loc, "Redefinition of %s", tok->str);
        }
        ty = read_array(ty);
        expect(TK_RESERVED, ";");
        new_node_gvar(ty, tok->str);
    }
}

Program *parse(Vector *tokens_) {
    tokens = tokens_;
    pos = 0;

    prog = calloc(1, sizeof(Program));
    prog->fns = map_create();
    prog->gvars = map_create();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}
