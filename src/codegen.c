#include "10cc.h"

char *argregs1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argregs4[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *argregs8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

char *funcname;
int label_cnt = 0;

void gen_data(Prog *prog);
void gen_text(Prog *prog);
void gen(Node *node);
void gen_lval(Node *node);
void load_arg(Node *node, int index);
void load(Type *type);
void store(Type *type);
void inc(Type *type);

/**
 * Generate assembly code.
 * @param prog A program.
 */
void codegen(Prog *prog) {
    printf(".intel_syntax noprefix\n");
    gen_data(prog);
    gen_text(prog);
}

/**
 * Generate assembly code for a data segment.
 * @param prog A program.
 */
void gen_data(Prog *prog) {
    printf(".data\n");
    for (int i = 0; i < prog->gvars->len; i++) {
        Var *var = vec_at(prog->gvars, i);
        printf("%s:\n", var->name);
        if (var->data) {
            printf("  .string \"%s\"\n", var->data);
        } else {
            printf("  .zero %d\n", var->type->size);
        }
    }
}

/**
 * Generate assemly code for a code segment.
 * @param prog A program.
 */
void gen_text(Prog *prog) {
    printf(".text\n");
    for (int i = 0; i < prog->fns->len; i++) {
        Func *fn = vec_at(prog->fns->vals, i);
        if (!fn->body) {
            continue;
        }
        funcname = fn->name;

        int offset = 0;
        for (int i = 0; i < fn->lvars->len; i++) {
            Var *var = vec_at(fn->lvars, i);
            offset += var->type->size;
            var->offset = offset;
        }

        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);

        // Prologue.
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", offset);

        // Push arguments to the stack.
        for (int i = 0; i < fn->args->len; i++) {
            load_arg(vec_at(fn->args, i), i);
        }

        // Emit code.
        gen(fn->body);

        // Epilogue.
        printf(".Lreturn.%s:\n", funcname);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}

/**
 * Generate assembly code for a given node.
 * @param node A node.
 */
void gen(Node *node) {
    int cur_label_cnt;
    switch (node->kind) {
        case ND_NULL:
            return;
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_ADDR:
            gen_lval(node->lhs);
            return;
        case ND_DEREF:
            gen(node->lhs);
            load(node->type);
            return;
        case ND_VARREF:
            gen_lval(node);
            if (node->type->kind != TY_ARY) {
                load(node->type);
            }
            return;
        case ND_FUNC_CALL:
            for (int i = 0; i < node->args->len; i++) {
                gen(vec_at(node->args, i));
            }
            for (int i = node->args->len - 1; i >= 0; i--) {
                printf("  pop %s\n", argregs8[i]);
            }
            printf("  mov al, 0\n");
            printf("  call %s\n", node->funcname);
            printf("  push rax\n");
            if (node->type->kind == TY_VOID) {
                printf("  pop rax\n");
                printf("  movsx rax, al\n");
                printf("  push rax\n");
            }
            return;
        case ND_ASSIGN:
            if (node->lhs->kind == ND_DEREF) {
                gen(node->lhs->lhs);
            } else {
                gen_lval(node->lhs);
            }
            gen(node->rhs);
            store(node->lhs->type);
            return;
        case ND_PRE_INC:
            gen_lval(node->lhs);
            gen_lval(node->lhs);
            load(node->type);
            inc(node->type);
            store(node->type);
            return;
        case ND_IF:
            cur_label_cnt = label_cnt++;
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            if (node->els) {
                printf("  je .Lelse%03d\n", cur_label_cnt);
                gen(node->then);
                printf("  jmp .Lend%03d\n", cur_label_cnt);
                printf(".Lelse%03d:\n", cur_label_cnt);
                gen(node->els);
            } else {
                printf("  je .Lend%03d\n", cur_label_cnt);
                gen(node->then);
            }
            printf(".Lend%03d:\n", cur_label_cnt);
            return;
        case ND_WHILE:
            cur_label_cnt = label_cnt++;
            printf(".Lbegin%03d:\n", cur_label_cnt);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%03d\n", cur_label_cnt);
            gen(node->then);
            printf("  jmp .Lbegin%03d\n", cur_label_cnt);
            printf(".Lend%03d:\n", cur_label_cnt);
            return;
        case ND_FOR:
            cur_label_cnt = label_cnt++;
            gen(node->init);
            printf(".Lbegin%03d:\n", cur_label_cnt);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%03d\n", cur_label_cnt);
            gen(node->then);
            gen(node->upd);
            printf("  jmp .Lbegin%03d\n", cur_label_cnt);
            printf(".Lend%03d:\n", cur_label_cnt);
            return;
        case ND_RETURN:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  jmp .Lreturn.%s\n", funcname);
            return;
        case ND_EXPR_STMT:
            gen(node->lhs);
            printf("  add rsp, 8\n");
            return;
        case ND_BLOCK:
        case ND_STMT_EXPR:
            for (int i = 0; i < node->stmts->len; i++) {
                gen(vec_at(node->stmts, i));
            }
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
 * Push an address to a lvalue to the stack.
 * @param node A node for an lvalue.
 */
void gen_lval(Node *node) {
    switch (node->kind) {
        case ND_VARREF:
            if (node->var->is_local) {
                printf("  lea rax, [rbp-%d]\n", node->var->offset);
            } else {
                printf("  lea rax, %s\n", node->var->name);
            }
            printf("  push rax\n");
            break;
        case ND_DEREF:
            gen(node->lhs);
            break;
    }
}

/**
 * Push a value to a pre-defined address, which follows the function calling convention.
 * @param Node A node.
 * @param index The index of an argument.
 */
void load_arg(Node *node, int index) {
    switch (node->var->type->size) {
        case 1:
            printf("  mov [rbp-%d], %s\n", node->var->offset, argregs1[index]);
            break;
        case 4:
            printf("  mov [rbp-%d], %s\n", node->var->offset, argregs4[index]);
            break;
        case 8:
            printf("  mov [rbp-%d], %s\n", node->var->offset, argregs8[index]);
            break;
    }
}

/**
 * Load a value from an address on the top of the stack, and push the value to the stack.
 * @param type A type of a value to be loaded.
 */
void load(Type *type) {
    printf("  pop rax\n");
    switch (type->size) {
        case 1:
            printf("  movsx rax, byte ptr [rax]\n");
            break;
        case 4:
            printf("  movsx rax, dword ptr [rax]\n");
            break;
        case 8:
            printf("  mov rax, [rax]\n");
            break;
    }
    printf("  push rax\n");
}

/**
 * Load a value and an address from the top of the stack. The loaded value is stored to the loaded address.
 * @param type A type of a value to be stored.
 */
void store(Type *type) {
    printf("  pop rdi\n");
    printf("  pop rax\n");
    switch (type->size) {
        case 1:
            printf("  mov [rax], dil\n");
            break;
        case 4:
            printf("  mov [rax], edi\n");
            break;
        case 8:
            printf("  mov [rax], rdi\n");
            break;
    }
    printf("  push rdi\n");
}

/**
 * Increment the value on the top of the stack.
 * @param type A type.
 */
void inc(Type *type) {
    printf("  pop rax\n");
    printf("  add rax, %d\n", type->base ? type->base->size : 1);
    printf("  push rax\n");
}
