#include "10cc.h"

Node *do_walk(Node *node, bool decay);
Node *walk(Node *node);
Node *walk_nodecay(Node *node);

Type *int_type();
Type *char_type();
Type *void_type();
Type *ptr_to(Type *base);
Type *ary_of(Type *base, int len);
Node *decay_array(Node *base);

bool is_same_type(Type *x, Type *y);

void ensure_referable(Node *node);
void ensure_int(Node *node);

Node *scale_ptr(NodeKind kind, Node *base, Type *type);

// Assign a type to a given node.
Node *do_walk(Node *node, bool decay) {
    switch (node->kind) {
        case ND_NULL:
        case ND_BREAK:
        case ND_CONTINUE:
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
            node->els = walk(node->els);
            return node;
        case ND_FOR:
            node->init = walk(node->init);
            node->cond = walk(node->cond);
            node->upd = walk(node->upd);
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
        case ND_COMMA:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);
            node->type = node->rhs->type;
            return node;
        case ND_TERNARY:
            node->cond = walk(node->cond);
            node->then = walk(node->then);
            node->els = walk(node->els);
            if (!is_same_type(node->then->type, node->els->type)) {
                error_at(node->tok->loc, "error");
            }
            node->type = node->then->type;
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
        case ND_NOT:
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
                vec_set(node->args, i, walk((Node *)vec_at(node->args, i)));
            }
            return node;
        case ND_BLOCK:
        case ND_STMT_EXPR:
            for (int i = 0; i < node->stmts->len; i++) {
                vec_set(node->stmts, i, walk((Node *)vec_at(node->stmts, i)));
            }
            if (node->stmts->len) {
                node->type = ((Node *)vec_back(node->stmts))->type;
            }
            return node;
        case ND_SIZEOF:
            node->lhs = walk_nodecay(node->lhs);
            return new_node_num(node->lhs->type->size, node->tok);
        case ND_MEMBER:
            node->lhs = walk(node->lhs);
            if (node->lhs->type->kind != TY_STRUCT) {
                error_at(node->tok->loc, "error: member reference base type is not a structure\n");
            }
            if (!map_contains(node->lhs->type->members, node->member_name)) {
                error_at(node->tok->loc, "error: no member named '%s'", node->member_name);
            }
            node->member = map_at(node->lhs->type->members, node->member_name);
            node->type = node->member->type;
            return node;
        default:
            error_at(node->tok->loc, "error: failed to assign type\n");
            return NULL;
    }
}

// Run do_walk with the decay property enabled.
Node *walk(Node *node) { return do_walk(node, true); }

// Run do_walk with the decay property disabled.
Node *walk_nodecay(Node *node) { return do_walk(node, false); }

// Assign a type to each node in a given program.
Prog *assign_type(Prog *prog) {
    for (int i = 0; i < prog->fns->len; i++) {
        Func *fn = vec_at(prog->fns->vals, i);
        if (fn->body) {
            fn->body = walk(fn->body);
        }
    }
    return prog;
}

// Create a type.
Type *new_type(TypeKind type, int size) {
    Type *ret = calloc(1, sizeof(Type));
    ret->kind = type;
    ret->size = size;
    return ret;
}

// Create a long type.
Type *long_type() { return new_type(TY_LONG, 8); }

// Create an int type.
Type *int_type() { return new_type(TY_INT, 4); }

// Create a short type.
Type *short_type() { return new_type(TY_SHORT, 2); }

// Create a char type.
Type *char_type() { return new_type(TY_CHAR, 1); }

// Create a void type.
Type *void_type() { return new_type(TY_VOID, 1); }

// Create a pointer type.
Type *ptr_to(Type *base) {
    Type *type = new_type(TY_PTR, 8);
    type->base = base;
    return type;
}

// Create an array type.
Type *ary_of(Type *base, int array_size) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_ARY;
    type->size = base->size * array_size;
    type->base = base;
    type->array_size = array_size;
    return type;
}

// Create a struct type.
Type *struct_type(Map *members) {
    Member *last = vec_back(members->vals);
    int size = last->offset + last->type->size;
    Type *type = new_type(TY_STRUCT, size);
    type->members = members;
    return type;
}

// Decay a given node to an pointer if it is an array.
Node *decay_array(Node *base) {
    if (base->type->kind != TY_ARY) {
        return base;
    }
    Node *node = new_node_uniop(ND_ADDR, base, base->tok);
    node->type = ptr_to(base->type->base);
    return node;
}

// Return true if given two types are the same.
bool is_same_type(Type *x, Type *y) {
    if (x->kind != y->kind) {
        return false;
    }
    switch (x->kind) {
        case TY_PTR:
            return is_same_type(x->base, y->base);
        case TY_ARY:
            return x->array_size == y->array_size && is_same_type(x->base, y->base);
        default:
            return true;
    }
}

// Ensure that a given node is referable.
void ensure_referable(Node *node) {
    NodeKind kind = node->kind;
    if (kind != ND_VARREF && kind != ND_DEREF && kind != ND_MEMBER) {
        char *loc = node->tok->loc;
        error_at(loc, "error: lvalue required as left operand of assignment\n");
    }
}

// Ensure that a given node is an integer.
void ensure_int(Node *node) {
    TypeKind kind = node->type->kind;
    if (kind != TY_INT && kind != TY_CHAR) {
        char *loc = node->tok->loc;
        error_at(loc, "error: not an integer\n");
    }
}

// Scale a number by the size of a given type.
// Say kind = ND_MUL, base = ND_NUM(2), and type = TY_INT.
// In this case, return ND_NUM(2 * 4), where 4 is the size of an integer.
Node *scale_ptr(NodeKind kind, Node *base, Type *type) {
    return new_node_binop(kind, base, new_node_num(type->base->size, base->tok), base->tok);
}
