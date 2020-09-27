// A line comment.

/**
 * A block comment.
 */
int printf(char *fmt);
int assert(int expected, int actual, char *code);

int ret42() {
    return 42;
    return 24;
}

int retif() {
    if (1) {
        return 42;
    } else {
        return 24;
    }
}

int retif2() {
    if (0) {
        return 42;
    } else {
        return 24;
    }
}

// int fibo(int n);

int fibo(int n) {
    if (n < 2) {
        return 1;
    } else {
        return fibo(n - 2) + fibo(n - 1);
    }
}

char first(char *str) {
    return str[0];
}

int int_gvar;
int *intptr_gvar;
int intary_gvar[2];

int main() {
    assert(1, 1, "1;");
    assert(2, 1 + 1, "1 + 1;");
    assert(2, 1 * 2, "1 * 2;");
    assert(3, 1 + 1 * 2, "1 + 1 * 2;");
    assert(4, (1 + 1) * 2, "(1 + 1) * 2;");
    assert(1, 1 == 1, "1 == 1");
    assert(0, 1 != 1, "1 != 1");
    assert(0, 1 != 1, "1 != 1");
    assert(42, ({ int a; a = 42; a; }), "int a; a = 42; a;");
    assert(42, ({ int a; a = 21; a = a + 21; }), "int a; a = 21; a = a + 21;");
    assert(42, ({ int foo; foo = 21; foo = foo + 21; foo; }), "int foo; foo = 21; foo = foo + 21; foo;");
    assert(42, ret42(), "ret42();");
    assert(42, retif(), "retif();");
    assert(24, retif2(), "retif2();");
    assert(10, ({ int a; a = 0; while (a < 10) a = a + 1; a; }), "int a; a = 0; while (a < 10) a = a + 1; a;");  // TODO
    assert(55, ({ int total; int i; total = 0; for (i=1; i <= 10; i=i+1) total = total + i; total; }), "int total; int i; total = 0; for (i=1; i <= 10; i=i+1) total = total + i; total;");
    assert(13, fibo(6), "fibo(6);");
    // assert(3, ({ int x; int *y; x = 3; y = &x; *y; }), "int x; int *y; x = 3; y = &x; *y;");
    // assert(1, ({ int a; int b; a = 1; b = 2; int *p; p = &b; int *q; q = p + 1; *q; }), "{int a; int b; a = 1; b = 2; int *p; p = &b; int *q; q = p + 1; *q;");
    // assert(4, ({ int a; sizeof(a);}), "int a; sizeof(a);");
    // assert(8, ({ int *a; sizeof(a);}), "int *a; sizeof(a);");
    // assert(0, ({ int a[3]; 0; }), "int a[3]; 0;");
    // assert(12, ({ int a[3]; sizeof(a); }), "int a[3]; sizeof(a);");
    // assert(1, ({ int a[3]; *a = 1; *a; }), "int a[3]; *a = 1; *a;");
    // assert(6, ({ int a[2]; *a = 2; *(a + 1) = 4; *a + *(a + 1); }), "int a[2]; *a = 2; *(a + 1) = 4; *a + *(a + 1);");
    // assert(3, ({ int a[2]; *a = 1; *(a + 1) = 2; a[0] + a[1]; }), "int a[2]; *a = 1; *(a + 1) = 2; a[0] + a[1];");
    // assert(3, ({ int a[2]; *a = 1; *(a + 1) = 2; 0[a] + 1[a]; }), "int a[2]; *a = 1; *(a + 1) = 2; 0[a] + 1[a];");
    // assert(1, ({ int_gvar = 1; int_gvar;  }), "int_gvar = 1; int_gvar;");
    // assert(2, ({ int a; a = 2; intptr_gvar = &a; *intptr_gvar; }), "int a; a = 2; intptr_gvar = &a; *intptr_gvar;");
    // assert(3, ({ intary_gvar[0] = 3; intary_gvar[0]; }), "intary_gvar[0] = 3; intary_gvar[0];");
    // assert(1, ({ char a; sizeof(a); }), "char a; sizeof(a);");
    // assert(4, ({ char a; a = 4; a; }), "char a; a = 4; a;");
    // assert(8, ({ char a; a = 4; a + 4; }), "char a; a = 4; a + 4;");
    // assert(8, ({ char a; char b; a = 4; b = 4; a + b; }), "char a; char b; a = 4; b = 4; a + b;");
    // assert(4, ({ char a[3]; a[1] = 4; a[1]; }), "char a[3]; a[1] = 4; a[1];");
    // assert(3, ({ char x[3]; x[0] = -1; int y; y = 4; x[0] + y; }), "char x[3]; x[0] = -1; int y; y = 4; x[0] + y;");
    // assert(1, ({ int a; int b; int c; a = 1; b = 2; c = 3; &a - &b; }), "int a; int b; int c; a = 1; b = 2; c = 3; &a - &b;");
    // assert(2, ({ int a; int b; int c; a = 1; b = 2; c = 3; &a - &c; }), "int a; int b; int c; a = 1; b = 2; c = 3; &a - &c;");
    // assert(1, ({ int a; int b; int c; a = 1; b = 2; c = 3; *(&b + 1); }), "int a; int b; int c; a = 1; b = 2; c = 3; *(&b + 1);");
    // assert(3, ({ int a; int b; int c; a = 1; b = 2; c = 3; *(&b - 1); }), "int a; int b; int c; a = 1; b = 2; c = 3; *(&b - 1);");
    // assert(3, ({ int a; int b; int c; a = 1; b = 2; c = 3;;;;; *(&b - 1); }), "int a; int b; int c; a = 1; b = 2; c = 3; *(&b - 1);;;;");
    // assert(4, ({ sizeof(-1); }), "sizeof(-1);");
    // assert(4, ({ sizeof -1; }), "sizeof -1;");
    // assert(4, ({ sizeof(int); }), "sizeof(int);");
    // assert(1, ({ sizeof(char); }), "sizeof(char);");
    // assert(65, ({ first("ABC"); }), "first('ABC');");
    return 0;
}
