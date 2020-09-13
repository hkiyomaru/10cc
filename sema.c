#include "9cc.h"

/**
 * Ensures the node is referable.
 * @param node A node.
 */
void check_referable(Node *node) {
    NodeKind kind = node->kind;
    assert(kind == ND_LVAR || kind == ND_GVAR || kind == ND_DEREF);
}

/**
 * Ensures the node is an integer.
 * @param node A node.
 */
void check_int(Node *node) {
    TypeKind ty = node->ty->kind;
    assert(ty == TY_INT);
}

/**
 * If the given node is an array, performs type conversion.
 * Otherwise, does nothing.
 * @param base A node.
 * @param decay If True, does nothing.
 * @return A node.
 */
Node *maybe_decay(Node *base, bool decay) {
    if (!decay || base->ty->kind != TY_ARY) {
        return base;
    }
    Node *node = new_node(ND_ADDR);
    node->ty = ptr_to(base->ty->base);
    node->lhs = base;
    return node;
}

/**
 * Scales the number by referring to the given type.
 * @param kind The kind of a node.
 * @param base A node to be scaled.
 * @param ty A type to specify the scale.
 * @return A node.
 */
Node *scale_ptr(NodeKind kind, Node *base, Type *ty) {
    Node *node = new_node(kind);
    node->lhs = base;
    node->rhs = new_node_num(ty->base->size);
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
        case ND_NUM:
            return node;
        case ND_LVAR:
        case ND_GVAR:
            return maybe_decay(node, decay);
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
            if (node->rhs->ty->kind == TY_PTR) {
                Node *tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }
            if (node->lhs->ty->kind == TY_PTR) {
                node->rhs = scale_ptr(ND_MUL, node->rhs, node->lhs->ty);
                node->ty = node->lhs->ty;
            } else {
                node->ty = int_ty();
            }
            return node;
        case ND_SUB:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);

            Type *lty = node->lhs->ty;
            Type *rty = node->rhs->ty;

            if (lty->kind == TY_PTR && rty->kind == TY_PTR) {
                node = scale_ptr(ND_DIV, node, lty);
                node->ty = lty;
            } else {
                node->ty = int_ty();
            }
            return node;
        case ND_ASSIGN:
            node->lhs = walk_nodecay(node->lhs);
            check_referable(node->lhs);
            node->rhs = walk(node->rhs);
            node->ty = node->lhs->ty;
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
            node->ty = int_ty();
            return node;
        case ND_ADDR:
            node->lhs = walk(node->lhs);
            check_referable(node->lhs);
            node->ty = ptr_to(node->lhs->ty);
            return node;
        case ND_DEREF:
            node->lhs = walk(node->lhs);
            assert(node->lhs->ty->kind == TY_PTR);
            node->ty = node->lhs->ty->base;
            return maybe_decay(node, decay);
        case ND_RETURN:
            node->lhs = walk(node->lhs);
            return node;
        case ND_FUNC_CALL:
            for (int i = 0; i < node->args->len; i++) {
                Node *arg = vec_get(node->args, i);
                arg = walk(arg);
            }
            return node;
        case ND_BLOCK:
            for (int i = 0; i < node->stmts->len; i++) {
                Node *stmt = vec_get(node->stmts, i);
                stmt = walk(stmt);
            }
            return node;
        default:
            fprintf(stderr, "\n\n%d\n\n", node->kind);
            assert(0 && "Unknown node type");
    }
}

/**
 * Semantically parses a program.
 */
void sema(Program *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Function *fn = vec_get(prog->fns->vals, i);
        fn->body = walk(fn->body);
    }
}
