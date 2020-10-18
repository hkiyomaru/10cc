#include "10cc.h"

Node *do_walk(Node *node, bool decay);
Node *walk(Node *node);
Node *walk_nodecay(Node *node);

Type *int_type();
Type *char_type();
Type *ptr_to(Type *base);
Type *ary_of(Type *base, int len);
Node *decay_array(Node *base);

void ensure_referable(Node *node);
void ensure_int(Node *node);

Node *scale_ptr(NodeKind kind, Node *base, Type *type);

/**
 * Assigns types to nodes recursively.
 * @param node A node.
 * @param decay If true, the node may be decayed to a pointer.
 * @return A node.
 */
Node *do_walk(Node *node, bool decay) {
    switch (node->kind) {
        case ND_NULL:
            return node;
        case ND_NUM:
            return node;
        case ND_VARREF:
            if (decay) {
                node = decay_array(node);
            }
            return node;
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
            ensure_referable(node->lhs);
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
            ensure_int(node->lhs);
            ensure_int(node->rhs);
            node->type = int_type();
            return node;
        case ND_ADDR:
            node->lhs = walk(node->lhs);
            ensure_referable(node->lhs);
            node->type = ptr_to(node->lhs->type);
            return node;
        case ND_DEREF:
            node->lhs = walk(node->lhs);
            assert(node->lhs->type->kind == TY_PTR);
            node->type = node->lhs->type->base;
            if (decay) {
                node = decay_array(node);
            }
            return node;
        case ND_RETURN:
            node->lhs = walk(node->lhs);
            return node;
        case ND_FUNC_CALL:
            for (int i = 0; i < node->args->len; i++) {
                Node *arg = vec_at(node->args, i);
                vec_set(node->args, i, walk(arg));
            }
            return node;
        case ND_BLOCK:
        case ND_STMT_EXPR:
            for (int i = 0; i < node->stmts->len; i++) {
                Node *stmt = vec_at(node->stmts, i);
                vec_set(node->stmts, i, walk(stmt));
            }
            node->type = ((Node *)vec_back(node->stmts))->type;
            return node;
        case ND_SIZEOF:
            node->lhs = walk_nodecay(node->lhs);
            return new_node_num(node->lhs->type->size, node->tok);
        default:
            error_at(node->tok->loc, "error: failed to assign type\n");
    }
}

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
 * Assign a type to each nodes in a given program.
 */
Program *assign_type(Program *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Function *fn = vec_at(prog->fns->vals, i);
        if (fn->body) {
            fn->body = walk(fn->body);
        }
    }
    return prog;
}

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
 * If the given node is an array, decays it to a pointer.
 * Otherwise, does nothing.
 * @param base A node.
 * @return A node.
 */
Node *decay_array(Node *base) {
    if (base->type->kind != TY_ARY) {
        return base;
    }
    Node *node = new_node_unary_op(ND_ADDR, base, base->tok);
    node->type = ptr_to(base->type->base);
    return node;
}

/**
 * Ensures the node is referable.
 * @param node A node.
 */
void ensure_referable(Node *node) {
    NodeKind kind = node->kind;
    if (kind != ND_VARREF && kind != ND_DEREF) {
        char *loc = node->tok->loc;
        error_at(loc, "error: lvalue required as left operand of assignment\n");
    }
}

/**
 * Ensures the node is an integer.
 * @param node A node.
 */
void ensure_int(Node *node) {
    TypeKind kind = node->type->kind;
    if (kind != TY_INT && kind != TY_CHAR) {
        char *loc = node->tok->loc;
        error_at(loc, "error: not an integer\n");
    }
}

/**
 * Scales the number by referring to the given type.
 * @param kind The kind of a node.
 * @param base A node to be scaled.
 * @param type A type to specify the scale.
 * @return A node.
 */
Node *scale_ptr(NodeKind kind, Node *base, Type *type) {
    return new_node_bin_op(kind, base, new_node_num(type->base->size, base->tok), base->tok);
}
