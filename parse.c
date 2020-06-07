#include "9cc.h"

Vector *tokens;
Function *fn;
int pos;

bool consume(char *op) {
    Token *token = tokens->data[pos];
    if (token->kind != TK_RESERVED || strcmp(token->str, op) != 0)
        return false;
    pos++;
    return true;
}

bool consume_stmt(TokenKind kind) {
    Token *token = tokens->data[pos];
    if (token->kind != kind)
        return false;
    pos++;
    return true;
}

Token *consume_ident() {
    Token *token = tokens->data[pos];
    if (token->kind != TK_IDENT)
        return NULL;
    Token *current_token = token;
    pos++;
    return current_token;
}

void expect(char *op) {
    Token *token = tokens->data[pos];
    if(token->kind != TK_RESERVED || strcmp(token->str, op) != 0)
        error_at(token->str, "Expected '%s'");
    pos++;
}

int expect_number() {
    Token *token = tokens->data[pos];
    if (token->kind !=TK_NUM)
        error_at(token->str, "Expected a number");
    int val = token->val;
    pos++;
    return val;
}

bool at_eof() {
    Token *token = tokens->data[pos];
    return token->kind == TK_EOF;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Function *func() {
    fn = calloc(1, sizeof(Function));
    Token *tok = consume_ident();
    if (!tok) {
        Token *tok = tokens->data[pos];
        error_at(tok->loc, "Invalid function identifier");
    }
    fn->name = tok->str;
    fn->args = create_vec();
    fn->lvars = create_map();
    expect("(");
    while (!consume(")")) {
        Token *tok = consume_ident();
        if (!tok) {
            Token *tok = tokens->data[pos];
            error_at(tok->loc, "Invalid function identifier");
        }
        LVar *arg = get_elem_from_map(fn->lvars, tok->str);
        if (arg)
            error_at(tok->loc, "Invalid function identifier");
        arg = calloc(1, sizeof(LVar));
        arg->offset = (fn->lvars->len + 1) * 8;
        add_elem_to_vec(fn->args, arg);
        add_elem_to_map(fn->lvars, tok->str, arg);
        consume(",");
    }
    expect("{");
    fn->body = create_vec();
    while (!consume("}"))
        add_elem_to_vec(fn->body, stmt());
    return fn;
}

Node *stmt() {
    Node *node;
    if (consume("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->stmts = create_vec();
        while(!consume("}"))
            add_elem_to_vec(node->stmts, (void *) stmt());
        return node;
    } else if (consume_stmt(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else if (consume_stmt(TK_IF)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume_stmt(TK_ELSE))
            node->els = stmt();
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
    } else {
        node = expr();
    }
    expect(";");
    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();
    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

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

Node *add() {
    Node *node = mul();
    for (;;) {
        if (consume("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

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

Node *unary() {
    if (consume("+")) {
        return primary();
    } else if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else {
        return primary();
    }
}

Node *primary() {
    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        if (consume("(")) {
            node->kind = ND_FUNC_CALL;
            node->name = tok->str;
            node->args = create_vec();
            while (!consume(")")) {
                Node *arg = expr();
                add_elem_to_vec(node->args, arg);
                consume(",");
            }
        } else {
            node->kind = ND_LVAR;
            LVar *lvar = get_elem_from_map(fn->lvars, tok->str);
            if (lvar) {
                node->offset = lvar->offset;
            } else {
                lvar = calloc(1, sizeof(LVar));
                lvar->offset = (fn->lvars->len + 1) * 8;
                node->offset = lvar->offset;
                add_elem_to_map(fn->lvars, tok->str, lvar);
            }
        }
        return node;
    }
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    return new_node_num(expect_number());
}

Vector *parse(Vector *tokens_) {
    tokens = tokens_;
    pos = 0;
    Vector *code = create_vec();
    while (!at_eof())
        add_elem_to_vec(code, func());
    return code;
}
