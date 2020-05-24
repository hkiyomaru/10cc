#include "9cc.h"

Node *code[128];
LVar *locals;

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

void parse() {
    locals = calloc(1, sizeof(LVar));
    int i = 0;
    while (!at_eof())
        code[i++] = stmt();
    code[i] = NULL;
}

Node *stmt() {
    Node *node;
    if (consume_op("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->stmts = create_vector();
        while(!consume_op("}"))
            push(node->stmts, (void *) stmt());
        return node;
    } else if (consume_stmt(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else if (consume_stmt(TK_IF)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect_op("(");
        node->cond = expr();
        expect_op(")");
        node->then = stmt();
        if (consume_stmt(TK_ELSE))
            node->els = stmt();
        return node;
    } else if (consume_stmt(TK_WHILE)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect_op("(");
        node->cond = expr();
        expect_op(")");
        node->then = stmt();
        return node;
    } else if (consume_stmt(TK_FOR)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect_op("(");
        if (!consume_op(";")) {
            node->init = expr();
            expect_op(";");
        }
        if (!consume_op(";")) {
            node->cond = expr();
            expect_op(";");
        }
        if (!consume_op(";")) {
            node->upd = expr();
        }
        expect_op(")");
        node->then = stmt();
        return node;
    } else {
        node = expr();
    }
    expect_op(";");
    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();
    if (consume_op("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

Node *equality() {
    Node *node = relational();
    if (consume_op("==")) {
        node = new_node(ND_EQ, node, relational());
    } else if (consume_op("!=")) {
        node = new_node(ND_NE, node, relational());
    } else {
        return node;
    }
}

Node *relational() {
    Node *node = add();
    if (consume_op("<=")) {
        return new_node(ND_LE, node, add());
    } else if (consume_op(">=")) {
        return new_node(ND_LE, add(), node);
    } else if (consume_op("<")) {
        return new_node(ND_LT, node, add());
    } else if (consume_op(">")) {
        return new_node(ND_LT, add(), node);
    } else {
        return node;
    }
}

Node *add() {
    Node *node = mul();
    for (;;) {
        if (consume_op("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume_op("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume_op("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if (consume_op("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

Node *unary() {
    if (consume_op("+")) {
        return primary();
    } else if (consume_op("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else {
        return primary();
    }
}

Node *primary() {
    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        if (consume_op("(")) {
            node->kind = ND_FUNC_CALL;
            node->args = create_vector();
            for(;;) {
                if (consume_op(")")) {
                    break;
                }
                Node *arg = expr();
                push(node->args, arg);
                consume_op(",");
            }
        } else {
            node->kind = ND_LVAR;
            LVar *lvar = find_lvar(tok);
            if (lvar) {
                node->offset = lvar->offset;
            } else {
                lvar = calloc(1, sizeof(LVar));
                lvar->next = locals;
                lvar->name = tok->str;
                lvar->len = tok->len;
                lvar->offset = locals->offset + 8;
                node->offset = lvar->offset;
                locals = lvar;
            }
        }
        return node;
    }
    if (consume_op("(")) {
        Node *node = expr();
        expect_op(")");
        return node;
    }
    return new_node_num(expect_number());
}

LVar *find_lvar(Token *tok) {
    for (LVar *var=locals; var; var=var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}
