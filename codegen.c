#include "9cc.h"

int num_argregs = 6;
char *argregs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

int label_id = 0;

void gen_lval(Node *node) {
    if (node->kind != ND_LVAR)
        error("Assignment error: Left side value must be a variable");
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

void gen_stmt(Node *node) {
    int cur_label_id;
    switch (node->kind) {
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_FUNC_CALL:
        for (int i = 0; i < node->args->len; i++) {
            gen_stmt(node->args->data[i]);
        }
        for (int i = node->args->len - 1; 0 <= i; i--) {
            printf("  pop %s\n", argregs[i]);  // 0 origin
        }
        printf("  call foo\n");  // TODO: fix hard coding
        printf("  ret\n");
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen_stmt(node->rhs);
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case ND_RETURN:
        gen_stmt(node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    case ND_IF:
        cur_label_id = label_id++;
        gen_stmt(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        if (node->els) {
            printf("  je .Lelse%03d\n", cur_label_id);
            gen_stmt(node->then);
            printf("  jmp .Lend%03d\n", cur_label_id);
            printf(".Lelse%03d:\n", cur_label_id);
            gen_stmt(node->els);
        } else {
            printf("  je .Lend%03d\n", cur_label_id);
            gen_stmt(node->then);
        }
        printf(".Lend%03d:\n", cur_label_id);
        return;
    case ND_WHILE:
        cur_label_id = label_id++;
        printf(".Lbegin%03d:\n", cur_label_id);
        gen_stmt(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%03d\n", cur_label_id);
        gen_stmt(node->then);
        printf("  jmp .Lbegin%03d\n", cur_label_id);
        printf(".Lend%03d:\n", cur_label_id);
        return;
    case ND_FOR:
        cur_label_id = label_id++;
        gen_stmt(node->init);
        printf(".Lbegin%03d:\n", cur_label_id);
        gen_stmt(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%03d\n", cur_label_id);
        gen_stmt(node->then);
        gen_stmt(node->upd);
        printf("  jmp .Lbegin%03d\n", cur_label_id);
        printf(".Lend%03d:\n", cur_label_id);
        return;
    case ND_BLOCK:
        for (int i = 0; i < node->stmts->len; i++)
            gen_stmt(node->stmts->data[i]);
        return;
    }
    gen_stmt(node->lhs);
    gen_stmt(node->rhs);
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

void gen() {
    for (int i = 0; code[i]; i++) {
        gen_stmt(code[i]);
        printf("  pop rax\n");
    }
}
