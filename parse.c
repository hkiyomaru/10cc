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
Node *primary();

Token *consume(TokenKind kind, char *str) {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind != kind) {
        return NULL;
    }
    if (str && strcmp(token->str, str) != 0) {
        return NULL;
    }
    pos++;
    return token;
}

Token *expect(TokenKind kind, char *str) {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind != kind) {
        error_at(token->loc, "Unexpected token");
    }
    if (str && strcmp(token->str, str) != 0) {
        error_at(token->loc, "Unexpected token");
    }
    pos++;
    return token;
}

bool at_eof() {
    Token *token = get_elem_from_vec(tokens, pos);
    return token->kind == TK_EOF;
}

bool at_typename() {
    Token *token = get_elem_from_vec(tokens, pos);
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

Node *ary_to_ptr(Node *ary) {
    if (ary->ty->ty != ARY) {
        error("Not an array");
    }
    Node *addr = calloc(1, sizeof(Node));
    addr->kind = ND_ADDR;
    addr->lhs = ary;
    addr->ty = ptr_to(ary->ty->ary_of);
    return addr;
}

Type *read_ty() {
    Token *token = get_elem_from_vec(tokens, pos);

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
            ty = ary_of(ty, -1);
        }
        ty = ary_of(ty, expect(TK_NUM, NULL)->val);
        expect(TK_RESERVED, "]");
    }
    return ty;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;

    if (lhs && (lhs->kind == ND_LVAR || lhs->kind == ND_GVAR) && lhs->ty->ty == ARY) {
        lhs = ary_to_ptr(lhs);
    }
    if (rhs && (lhs->kind == ND_LVAR || lhs->kind == ND_GVAR) && rhs->ty->ty == ARY) {
        rhs = ary_to_ptr(rhs);
    }

    node->lhs = lhs;
    node->rhs = rhs;
    switch (kind) {
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LE:
        case ND_LT:
        case ND_ASSIGN:
            node->ty = lhs->ty;
            break;
        case ND_ADDR:
            node->ty = ptr_to(lhs->ty);
            break;
        case ND_DEREF:
            node->ty = node->lhs->ty->ptr_to;
            break;
        default:
            break;
    }
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->ty = int_ty();
    return node;
}

Node *new_node_gvar(Type *ty, char *name) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_GVAR;
    node->name = name;
    node->ty = ty;
    add_elem_to_map(prog->gvars, node->name, node);
    return node;
}

Node *new_node_lvar(Type *ty, char *name) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->name = name;
    node->ty = ty;
    add_elem_to_map(fn->lvars, node->name, node);
    return node;
}

Node *new_node_func_call(Type *ty, char *name, Vector *args) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FUNC_CALL;
    node->ty = ty;
    node->name = name;
    node->args = args;
    return node;
}

Node *find_var(char *name) {
    Node *var = NULL;
    if (fn && fn->lvars) {
        var = get_elem_from_map(fn->lvars, name);
    }
    if (!var) {
        var = get_elem_from_map(prog->gvars, name);
    }
    return var;
}

/**
 * stmt = expr ";"
 *      | "{" stmt* "}"
 *      | "if" "(" expr ")" stmt ("else" stmt)?
 *      | "while" "(" expr ")" stmt
 *      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *      | "return" expr ";"
 */
Node *stmt() {
    Node *node;
    if (consume(TK_RESERVED, "{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->stmts = create_vec();
        while (!consume(TK_RESERVED, "}")) {
            add_elem_to_vec(node->stmts, (void *)stmt());
        }
        return node;
    } else if (consume(TK_RETURN, NULL)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect(TK_RESERVED, ";");
        return node;
    } else if (consume(TK_IF, NULL)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        if (consume(TK_ELSE, NULL)) {
            node->els = stmt();
        }
        return node;
    } else if (consume(TK_WHILE, NULL)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect(TK_RESERVED, "(");
        node->cond = expr();
        expect(TK_RESERVED, ")");
        node->then = stmt();
        return node;
    } else if (consume(TK_FOR, NULL)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
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
    node = expr();
    expect(TK_RESERVED, ";");
    return node;
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
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

/**
 * equality = relational (("==" | "!=") relational)?
 */
Node *equality() {
    Node *node = relational();
    if (consume(TK_RESERVED, "==")) {
        node = new_node(ND_EQ, node, relational());
    } else if (consume(TK_RESERVED, "!=")) {
        node = new_node(ND_NE, node, relational());
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
        return new_node(ND_LE, node, add());
    } else if (consume(TK_RESERVED, ">=")) {
        return new_node(ND_LE, add(), node);
    } else if (consume(TK_RESERVED, "<")) {
        return new_node(ND_LT, node, add());
    } else if (consume(TK_RESERVED, ">")) {
        return new_node(ND_LT, add(), node);
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
            if (node->ty->ty == PTR) {
                rhs = new_node(ND_MUL, mul(), new_node_num(node->ty->ptr_to->size));
            } else if (node->ty->ty == ARY) {
                rhs = new_node(ND_MUL, mul(), new_node_num(node->ty->ary_of->size));
            } else {
                rhs = mul();
            }
            node = new_node(ND_ADD, node, rhs);
        } else if (consume(TK_RESERVED, "-")) {
            if (node->ty->ty == PTR) {
                rhs = new_node(ND_MUL, mul(), new_node_num(node->ty->ptr_to->size));
            } else if (node->ty->ty == ARY) {
                rhs = new_node(ND_MUL, mul(), new_node_num(node->ty->ary_of->size));
            } else {
                rhs = mul();
            }
            node = new_node(ND_SUB, node, rhs);
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
            node = new_node(ND_MUL, node, unary());
        } else if (consume(TK_RESERVED, "/")) {
            node = new_node(ND_DIV, node, unary());
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
        return primary();
    } else if (consume(TK_RESERVED, "-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else if (consume(TK_RESERVED, "&")) {
        return new_node(ND_ADDR, primary(), NULL);
    } else if (consume(TK_RESERVED, "*")) {
        return new_node(ND_DEREF, primary(), NULL);
    } else if (consume(TK_SIZEOF, NULL)) {
        return new_node_num(unary()->ty->size);
    } else {
        return primary();
    }
}

/**
 * primary = num
 *         | ident ("(" ")")?
 *         | T ident
 *         | "(" expr ")"
 */
Node *primary() {
    // Variable declaration
    if (at_typename()) {
        Type *ty = read_ty();
        Token *tok = expect(TK_IDENT, NULL);
        if (find_var(tok->str)) {
            error_at(tok->loc, "Redefinition of '%s'", tok->str);
        }
        ty = read_array(ty);
        return new_node_lvar(ty, tok->str);
    }

    // Variable reference or function call
    Token *tok = consume(TK_IDENT, NULL);
    if (tok) {
        if (consume(TK_RESERVED, "(")) {
            Vector *args = create_vec();
            while (!consume(TK_RESERVED, ")")) {
                add_elem_to_vec(args, expr());
                consume(TK_RESERVED, ",");
            }
            Function *fn_ = get_elem_from_map(prog->fns, tok->str);
            if (!fn_) {
                error_at(tok->loc, "Undefined function");
            }
            return new_node_func_call(fn_->rty, tok->str, args);
        } else if (consume(TK_RESERVED, "[")) {
            Node *var = find_var(tok->str);
            if (!var) {
                error_at(tok->loc, "Undefined variable");
            }
            Node *offset = new_node(ND_MUL, expr(), new_node_num(var->ty->ary_of->size));
            Node *addr = new_node(ND_ADD, var, offset);
            expect(TK_RESERVED, "]");
            return new_node(ND_DEREF, addr, NULL);
        } else {
            Node *var = find_var(tok->str);
            if (!var) {
                error_at(tok->loc, "Undefined variable");
            }
            return var;
        }
    }

    if (consume(TK_RESERVED, "(")) {
        Node *node = expr();
        expect(TK_RESERVED, ")");
        return node;
    }

    Node *node = new_node_num(expect(TK_NUM, NULL)->val);
    if (consume(TK_RESERVED, "[")) {
        Node *var, *offset;
        tok = expect(TK_IDENT, NULL);
        var = find_var(tok->str);
        if (!var) {
            error_at(tok->loc, "Undefined variable");
        }
        if (var->ty->ty == PTR) {
            offset = new_node(ND_MUL, node, new_node_num(var->ty->ptr_to->size));
        } else if (var->ty->ty == ARY) {
            offset = new_node(ND_MUL, node, new_node_num(var->ty->ary_of->size));
        } else {
            offset = node;
        }
        Node *addr = new_node(ND_ADD, var, offset);
        expect(TK_RESERVED, "]");
        return new_node(ND_DEREF, addr, NULL);
    }
    return node;
}

void top_level() {
    Type *ty = read_ty();
    Token *tok = expect(TK_IDENT, NULL);

    if (consume(TK_RESERVED, "(")) {
        // Function
        fn = calloc(1, sizeof(Function));
        fn->rty = ty;
        fn->name = tok->str;
        fn->args = create_vec();
        fn->lvars = create_map();

        // Parse the arguments
        while (!consume(TK_RESERVED, ")")) {
            if (0 < fn->args->len) {
                expect(TK_RESERVED, ",");
            }

            Type *ty = read_ty();
            Token *tok = expect(TK_IDENT, NULL);
            if (get_elem_from_map(fn->lvars, tok->str)) {
                error_at(tok->loc, "Redefinition of '%s'", tok->str);
            }
            ty = read_array(ty);
            Node *node = new_node_lvar(ty, tok->str);
            add_elem_to_vec(fn->args, node);
        }

        // Register the function
        add_elem_to_map(prog->fns, fn->name, fn);

        // Parse the body
        fn->body = create_vec();
        expect(TK_RESERVED, "{");
        while (!consume(TK_RESERVED, "}")) {
            add_elem_to_vec(fn->body, stmt());
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
    prog->fns = create_map();
    prog->gvars = create_map();
    while (!at_eof()) {
        top_level();
    }
    return prog;
}
