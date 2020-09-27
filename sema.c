#include "10cc.h"

/**
 * Ensures the node is referable.
 * @param node A node.
 */
void check_referable(Node *node) {
    NodeKind kind = node->kind;
    if (kind != ND_VARREF && kind != ND_DEREF) {
        char *loc = node->tok->loc;
        error_at(loc, "error: lvalue required as left operand of assignment");
    }
}

/**
 * Ensures the node is an integer.
 * @param node A node.
 */
void check_int(Node *node) {
    TypeKind kind = node->type->kind;
    if (kind != TY_INT && kind != TY_CHAR) {
        char *loc = node->tok->loc;
        error_at(loc, "error: not an integer");
    }
}

/**
 * If the given node is an array, decays it to a pointer.
 * Otherwise, does nothing.
 * @param base A node.
 * @param decay If True, does nothing.
 * @return A node.
 */
Node *maybe_decay(Node *base, bool decay) {
    if (!decay || base->type->kind != TY_ARY) {
        return base;
    }
    Node *node = new_node(ND_ADDR, base->tok);
    node->type = ptr_to(base->type->base);
    node->lhs = base;
    return node;
}

/**
 * Scales the number by referring to the given type.
 * @param kind The kind of a node.
 * @param base A node to be scaled.
 * @param type A type to specify the scale.
 * @return A node.
 */
Node *scale_ptr(NodeKind kind, Node *base, Type *type) {
    Node *node = new_node(kind, base->tok);
    node->lhs = base;
    node->rhs = new_node_num(type->base->size, base->tok);
    return node;
}

Node *do_walk(Node *node, bool decay);

/**
 * Semantically parses a node.
 * @param node A node.
 * @return A node.
 */
Node *walk(Node *node) { return do_walk(node, true); }

/**
 * Semantically parses a node without decay.
 * @param node A node.
 * @return A node.
 */
Node *walk_nodecay(Node *node) { return do_walk(node, false); }

/**
 * Semantically parses a node.
 * @param node A node.
 * @param decay If True, the node may be decayed to a pointer.
 * @return A node.
 */
Node *do_walk(Node *node, bool decay) {
    switch (node->kind) {
        case ND_NULL:
            return node;
        case ND_NUM:
            return node;
        case ND_VARREF:
            return maybe_decay(node, decay);
        case ND_EXPR_STMT:
            walk(node->lhs);
            return node;
        case ND_IF:
            node->cond = walk(node->cond);
            node->then = walk(node->then);
            if (node->els) {
                node->els = walk(node->els);
            }
            return node;
        case ND_FOR:
            if (node->init) {
                node->init = walk(node->init);
            }
            if (node->cond) {
                node->cond = walk(node->cond);
            }
            if (node->upd) {
                node->upd = walk(node->upd);
            }
            node->then = walk(node->then);
            return node;
        case ND_WHILE:
            node->cond = walk(node->cond);
            node->then = walk(node->then);
            return node;
        case ND_ADD:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);
            if (node->rhs->type->kind == TY_PTR) {
                Node *tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }
            if (node->lhs->type->kind == TY_PTR) {
                node->rhs = scale_ptr(ND_MUL, node->rhs, node->lhs->type);
                node->type = node->lhs->type;
            } else {
                node->type = int_type();
            }
            return node;
        case ND_SUB:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);

            Type *ltype = node->lhs->type;
            Type *rtype = node->rhs->type;

            if (ltype->kind == TY_PTR) {
                if (rtype->kind == TY_PTR) {
                    node = scale_ptr(ND_DIV, node, ltype);
                    node->type = ltype;
                } else {
                    node->rhs = scale_ptr(ND_MUL, node->rhs, ltype);
                }
                node->type = ltype;
            } else {
                node->type = int_type();
            }
            return node;
        case ND_ASSIGN:
            node->lhs = walk_nodecay(node->lhs);
            check_referable(node->lhs);
            node->rhs = walk(node->rhs);
            node->type = node->lhs->type;
            return node;
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LE:
        case ND_LT:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);
            check_int(node->lhs);
            check_int(node->rhs);
            node->type = int_type();
            return node;
        case ND_ADDR:
            node->lhs = walk(node->lhs);
            check_referable(node->lhs);
            node->type = ptr_to(node->lhs->type);
            return node;
        case ND_DEREF:
            node->lhs = walk(node->lhs);
            assert(node->lhs->type->kind == TY_PTR);
            node->type = node->lhs->type->base;
            return maybe_decay(node, decay);
        case ND_RETURN:
            node->lhs = walk(node->lhs);
            return node;
        case ND_FUNC_CALL:
            for (int i = 0; i < node->args->len; i++) {
                Node *arg = vec_at(node->args, i);
                arg = walk(arg);
            }
            return node;
        case ND_BLOCK:
        case ND_STMT_EXPR:
            for (int i = 0; i < node->stmts->len; i++) {
                Node *stmt = vec_at(node->stmts, i);
                stmt = walk(stmt);
            }
            return node;
        case ND_SIZEOF:
            node->lhs = walk_nodecay(node->lhs);
            return new_node_num(node->lhs->type->size, node->tok);
        default:
            error_at(node->tok->loc, "error: failed to assign type");
    }
}

/**
 * Semantically parses a program.
 */
void sema(Program *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Function *fn = vec_at(prog->fns->vals, i);
        // Skip a function whose body is empty.
        if (fn->body) {
            fn->body = walk(fn->body);
        }
    }
}
