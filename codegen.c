#include "9cc.h"

int num_argregs = 6;
char *argregs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

int label_id = 0;

/**
 * Rounds up a number.
 * @param x A number to be rounded up.
 * @param aligh A base number.
 */
int roundup(int x, int align) { return (x + align - 1) & ~(align - 1); }

/**
 * Generates a code to push an address to a local variable to the stack.
 * @param node A node representing a local variable.
 */
void gen_lval(Node *node) {
    assert(node->kind == ND_LVAR);
    printf("  lea rax, [rbp-%d]\n", node->offset);
    printf("  push rax\n");
}

/**
 * Generates a code to push an address to a global variable to the stack.
 * @param node A node representing a global variable.
 */
void gen_gval(Node *node) {
    assert(node->kind == ND_GVAR);
    printf("  lea rax, %s\n", node->name);
    printf("  push rax\n");
}

/**
 * Generates code to perform the operation of a node.
 * @param node A node.
 */
void gen(Node *node) {
    int cur_label_id;
    switch (node->kind) {
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_FUNC_CALL:
            for (int i = 0; i < node->args->len; i++) {
                gen(vec_get(node->args, i));
            }
            for (int i = node->args->len - 1; 0 <= i; i--) {
                printf("  pop %s\n", argregs[i]);
            }
            printf("  call %s\n", node->name);
            printf("  push rax\n");
            return;
        case ND_GVAR:
            gen_gval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_LVAR:
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_ASSIGN:
            if (node->lhs->kind == ND_DEREF) {
                gen(node->lhs->lhs);
            } else if (node->lhs->kind == ND_LVAR) {
                gen_lval(node->lhs);
            } else if (node->lhs->kind == ND_GVAR) {
                gen_gval(node->lhs);
            }
            gen(node->rhs);
            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            return;
        case ND_RETURN:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            printf("  ret\n");
            return;
        case ND_IF:
            cur_label_id = label_id++;
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            if (node->els) {
                printf("  je .Lelse%03d\n", cur_label_id);
                gen(node->then);
                printf("  jmp .Lend%03d\n", cur_label_id);
                printf(".Lelse%03d:\n", cur_label_id);
                gen(node->els);
            } else {
                printf("  je .Lend%03d\n", cur_label_id);
                gen(node->then);
            }
            printf(".Lend%03d:\n", cur_label_id);
            return;
        case ND_WHILE:
            cur_label_id = label_id++;
            printf(".Lbegin%03d:\n", cur_label_id);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%03d\n", cur_label_id);
            gen(node->then);
            printf("  jmp .Lbegin%03d\n", cur_label_id);
            printf(".Lend%03d:\n", cur_label_id);
            return;
        case ND_FOR:
            cur_label_id = label_id++;
            gen(node->init);
            printf(".Lbegin%03d:\n", cur_label_id);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%03d\n", cur_label_id);
            gen(node->then);
            gen(node->upd);
            printf("  jmp .Lbegin%03d\n", cur_label_id);
            printf(".Lend%03d:\n", cur_label_id);
            return;
        case ND_BLOCK:
            for (int i = 0; i < node->stmts->len; i++) {
                gen(vec_get(node->stmts, i));
            }
            return;
        case ND_ADDR:
            if (node->lhs->kind == ND_LVAR) {
                gen_lval(node->lhs);
            } else if (node->lhs->kind == ND_GVAR) {
                gen_gval(node->lhs);
            }
            return;
        case ND_DEREF:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
    }
    gen(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");
    switch (node->kind) {
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
    }
    printf("  push rax\n");
}

/**
 * Generates code to allocate memory for global variables.
 * @param prog A program.
 */
void gen_data(Program *prog) {
    printf(".data\n");
    for (int i = 0; i < prog->gvars->len; i++) {
        Node *gvar = prog->gvars->vals->data[i];
        printf("%s:\n", gvar->name);
        printf("  .zero %d\n", gvar->ty->size);
    }
}

/**
 * Generates code to perform functions.
 * @param prog A program.
 */
void gen_text(Program *prog) {
    printf(".text\n");
    for (int i = 0; i < prog->fns->len; i++) {
        Function *fn = vec_get(prog->fns->vals, i);

        int offset = 0;
        for (int i = 0; i < fn->lvars->len; i++) {
            Node *var = fn->lvars->vals->data[i];
            offset += var->ty->size;
            offset = roundup(offset, var->ty->align);
            var->offset = offset;
        }

        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);

        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", roundup(offset, 16));

        for (int i = 0; i < fn->args->len; i++) {
            printf("  mov rax, rbp\n");
            Node *arg = vec_get(fn->args, i);
            printf("  sub rax, %d\n", arg->offset);
            printf("  mov [rax], %s\n", argregs[i]);
        }

        for (int i = 0; i < fn->body->len; i++) {
            gen(vec_get(fn->body, i));
            printf("  pop rax\n");
        }
    }
}

/**
 * Generates code written in x86 assembly language.
 * @param prog A program.
 */
void gen_x86(Program *prog) {
    printf(".intel_syntax noprefix\n");
    gen_data(prog);
    gen_text(prog);
}
