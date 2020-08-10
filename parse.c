#include "9cc.h"

Vector *tokens;
Map *fns;
Function *fn;
int pos;

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_lvar(Type *ty);
Node *new_node_func_call(Type *ty, char *name, Vector *args);
Node *ary_to_ptr(Node *ary);

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

bool consume(char *op) {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind != TK_RESERVED || strcmp(token->str, op) != 0) {
        return false;
    }
    pos++;
    return true;
}

bool consume_stmt(TokenKind kind) {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind != kind) {
        return false;
    }
    pos++;
    return true;
}

Token *consume_ident() {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *current_token = token;
    pos++;
    return current_token;
}

void expect(char *op) {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind != TK_RESERVED || strcmp(token->str, op) != 0) {
        error_at(token->loc, "Expected '%s'", op);
    }
    pos++;
}

int expect_number() {
    Token *token = get_elem_from_vec(tokens, pos);
    if (token->kind !=TK_NUM) {
        error_at(token->loc, "Expected a number");
    }
    int val = token->val;
    pos++;
    return val;
}

bool at_eof() {
    Token *token = get_elem_from_vec(tokens, pos);
    return token->kind == TK_EOF;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    
    if (lhs && lhs->kind == ND_LVAR && lhs->ty->ty == ARY) {
        lhs = ary_to_ptr(lhs);
    }
    if (rhs && rhs->kind == ND_LVAR && rhs->ty->ty == ARY) {
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

Node *new_node_lvar(Type *ty) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->ty = ty;
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

Node *ary_to_ptr(Node *ary) {
    if (ary->ty->ty != ARY) {
        error("Not an array");
    }
    Node *ptr = new_node_lvar(ptr_to(ary->ty->ary_of));
    Node *addr = calloc(1, sizeof(Node));
    addr->kind = ND_ADDR;
    addr->lhs = ary;
    addr->ty = ptr_to(ary->ty->ary_of);
    return new_node(ND_ASSIGN, ptr, addr);
}

/**
 * func = stmt*
 */
Function *func() {
    fn = calloc(1, sizeof(Function));
    
    // parse the return type
    if (!consume_stmt(TK_INT)) {
        Token *tok = get_elem_from_vec(tokens, pos);
        error_at(tok->loc, "Invalid type");
    }
    fn->rty = int_ty();
    while ((consume("*"))) {
        fn->rty = ptr_to(fn->rty);
    }

    // parse the function name
    Token *tok = consume_ident();
    if (!tok) {
        Token *tok = get_elem_from_vec(tokens, pos);
        error_at(tok->loc, "Invalid function name");
    }
    fn->name = tok->str;
    fn->args = create_vec();
    fn->lvars = create_map();
    // parse the arguments
    expect("(");
    for (;;) {
        // parse the type
        if (!consume_stmt(TK_INT)) {
            break;
        }
        Type *ty = int_ty();
        while (consume("*")) {
            ty = ptr_to(ty);
        }

        // parse the argument name
        Token *tok = consume_ident();
        if (!tok) {
            Token *tok = get_elem_from_vec(tokens, pos);
            error_at(tok->loc, "Invalid argument name");
        }
        Node *arg = get_elem_from_map(fn->lvars, tok->str);
        if (arg) {
            error_at(tok->loc, "Invalid argument name");
        }

        // create a node
        Node *node = new_node_lvar(ty);
        add_elem_to_vec(fn->args, node);
        add_elem_to_map(fn->lvars, tok->str, node);
        consume(",");
    }
    expect(")");
    add_elem_to_map(fns, fn->name, fn);
    expect("{");
    fn->body = create_vec();
    while (!consume("}")) {
        add_elem_to_vec(fn->body, stmt());
    }
    return fn;
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
    if (consume("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->stmts = create_vec();
        while(!consume("}")) {
            add_elem_to_vec(node->stmts, (void *) stmt());
        }
        return node;
    } else if (consume_stmt(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect(";");
        return node;
    } else if (consume_stmt(TK_IF)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume_stmt(TK_ELSE)) {
            node->els = stmt();
        }
        return node;
    } else if (consume_stmt(TK_WHILE)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    } else if (consume_stmt(TK_FOR)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("(");
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->upd = expr();
        }
        expect(")");
        node->then = stmt();
        return node;
    }
    node = expr();
    expect(";");
    return node;
}

/**
 * expr = assign
 */
Node *expr() {
    return assign();
}

/**
 * assign = equality ("=" assign)?
 */
Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

/**
 * equality = relational (("==" | "!=") relational)?
 */
Node *equality() {
    Node *node = relational();
    if (consume("==")) {
        node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
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
    if (consume("<=")) {
        return new_node(ND_LE, node, add());
    } else if (consume(">=")) {
        return new_node(ND_LE, add(), node);
    } else if (consume("<")) {
        return new_node(ND_LT, node, add());
    } else if (consume(">")) {
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
        if (consume("+")) {
            if (node->ty->ty == PTR) {
                rhs = new_node(ND_MUL, mul(), new_node_num(node->ty->ptr_to->size));
            } else if (node->ty->ty == ARY) {
                rhs = new_node(ND_MUL, mul(), new_node_num(node->ty->ary_of->size));
            } else {
                rhs = mul();
            }
            node = new_node(ND_ADD, node, rhs);
        } else if (consume("-")) {
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
        if (consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if (consume("/")) {
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
    if (consume("+")) {
        return primary();
    } else if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else if (consume("&")) {
        return new_node(ND_ADDR, primary(), NULL);
    } else if (consume("*")) {
        return new_node(ND_DEREF, primary(), NULL);
    } else if (consume_stmt(TK_SIZEOF)) {
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
    // variable declaration
    if (consume_stmt(TK_INT)) {
        Type *ty = int_ty();
        while (consume("*")) {
            ty = ptr_to(ty);
        }

        // parse the argument name
        Token *tok = consume_ident();
        if (!tok) {
            Token *tok = get_elem_from_vec(tokens, pos);
            error_at(tok->loc, "Invalid variable identifier");
        }
        Node *arg = get_elem_from_map(fn->lvars, tok->str);
        if (arg) {
            error_at(tok->loc, "Invalid variable identifier");
        }

        if (consume("[")) {
            ty = ary_of(ty, expect_number());
            expect("]");
        }

        // create a node
        Node *node = new_node_lvar(ty);
        add_elem_to_map(fn->lvars, tok->str, node);
        return node;
    }

    // variable reference or function call
    Token *tok = consume_ident();
    if (tok) {
        if (consume("(")) {
            Vector *args = create_vec();
            while (!consume(")")) {
                add_elem_to_vec(args, expr());
                consume(",");
            }
            Function *fn_ = get_elem_from_map(fns, tok->str);
            if (!fn_) {
                error_at(tok->loc, "Undefined function");   
            }
            return new_node_func_call(fn_->rty, tok->str, args);
        } else if (consume("[")) {
            Node *lvar = get_elem_from_map(fn->lvars, tok->str);
            if (!lvar) {
                error_at(tok->loc, "Undefined variable");
            }
            Node *offset = new_node(ND_MUL, expr(), new_node_num(lvar->ty->ary_of->size));
            Node *addr = new_node(ND_ADD, lvar, offset);
            expect("]");
            return new_node(ND_DEREF, addr, NULL);
        } else {
            Node *lvar = get_elem_from_map(fn->lvars, tok->str);
            if (!lvar) {
                error_at(tok->loc, "Undefined variable");
            }
            return lvar;
        }
    }

    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Node *node = new_node_num(expect_number());
    if (consume("[")) {
        Node *lvar, *offset;
        tok = consume_ident();
        if (!tok) {
            error_at(tok->loc, "Expected a variable");
        }
        lvar = get_elem_from_map(fn->lvars, tok->str);
        if (!lvar) {
            error_at(tok->loc, "Undefined variable");
        }
        if (lvar->ty->ty == PTR) {
            offset = new_node(ND_MUL, node, new_node_num(lvar->ty->ptr_to->size));
        } else if (lvar->ty->ty == ARY) {
            offset = new_node(ND_MUL, node, new_node_num(lvar->ty->ary_of->size));
        } else {
            offset = node;
        }
        Node *addr = new_node(ND_ADD, lvar, offset);
        expect("]");
        return new_node(ND_DEREF, addr, NULL);
    }
    return node;
}

Vector *parse(Vector *tokens_) {
    tokens = tokens_;
    pos = 0;
    fns = create_map();
    Vector *code = create_vec();
    while (!at_eof()) {
        add_elem_to_vec(code, func());
    }
    return code;
}
