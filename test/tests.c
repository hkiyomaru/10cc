// null directive
#

// A line comment.

/**
 * A block comment.
 */
void assert(long expected, long actual, char *code);

int ret3() {
    return 3;
    return 6;
}

int retif() {
    if (1) {
        return 3;
    } else {
        return 6;
    }
}

int retif2() {
    if (0) {
        return 3;
    } else {
        return 6;
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

void do_nothing() {
    int a = 1;
    int b = 1;
    int c = 1;
}

void do_nothing2() {
    int a = 1;
    int b = 1;
    int c = 1;
    return;
}

int int_gvar;
int *intptr_gvar;
int intary_gvar[2];

// Redefinition is allowd for global variables.
int a; int a;

int main() {
    assert(1, ({ 1; }), "1;");
    assert(2, ({ 1 + 1; }), "1 + 1;");
    assert(2, ({ 1 * 2; }), "1 * 2;");
    assert(3, ({ 1 + 1 * 2; }), "1 + 1 * 2;");
    assert(4, ({ (1 + 1) * 2; }), "(1 + 1) * 2;");
    assert(1, ({ 1 == 1; }), "1 == 1");
    assert(0, ({ 1 != 1; }), "1 != 1");
    assert(0, ({ 1 != 1; }), "1 != 1");
    assert(3, ({ int a; a = 3; a; }), "int a; a = 3; a;");
    assert(6, ({ int a; a = 3; a = a + 3; }), "int a; a = 3; a = a + 3;");
    assert(6, ({ int foo; foo = 3; foo = foo + 3; foo; }), "int foo; foo = 3; foo = foo + 3; foo;");
    assert(3, ({ ret3(); }), "ret3();");
    assert(3, ({ retif(); }), "retif();");
    assert(6, ({ retif2(); }), "retif2();");
    assert(10, ({ int a; a = 0; while (a < 10) a = a + 1; a; }), "int a; a = 0; while (a < 10) a = a + 1; a;");  // TODO
    assert(55, ({ int total; int i; total = 0; for (i=1; i <= 10; i=i+1) total = total + i; total; }), "int total; int i; total = 0; for (i=1; i <= 10; i=i+1) total = total + i; total;");
    assert(13, ({ fibo(6); }), "fibo(6);");
    assert(3, ({ int x; int *y; x = 3; y = &x; *y; }), "int x; int *y; x = 3; y = &x; *y;");
    assert(1, ({ int a; int b; a = 1; b = 2; int *p; p = &b; int *q; q = p + 1; *q; }), "{int a; int b; a = 1; b = 2; int *p; p = &b; int *q; q = p + 1; *q;");
    assert(4, ({ int a; sizeof(a); }), "int a; sizeof(a);");
    assert(8, ({ int *a; sizeof(a); }), "int *a; sizeof(a);");
    assert(0, ({ int a[3]; 0; }), "int a[3]; 0;");
    assert(12, ({ int a[3]; sizeof(a); }), "int a[3]; sizeof(a);");
    assert(1, ({ int a[3]; *a = 1; *a; }), "int a[3]; *a = 1; *a;");
    assert(6, ({ int a[2]; *a = 2; *(a + 1) = 4; *a + *(a + 1); }), "int a[2]; *a = 2; *(a + 1) = 4; *a + *(a + 1);");
    assert(3, ({ int a[2]; *a = 1; *(a + 1) = 2; a[0] + a[1]; }), "int a[2]; *a = 1; *(a + 1) = 2; a[0] + a[1];");
    assert(3, ({ int a[2]; *a = 1; *(a + 1) = 2; 0[a] + 1[a]; }), "int a[2]; *a = 1; *(a + 1) = 2; 0[a] + 1[a];");
    assert(1, ({ int_gvar = 1; int_gvar; }), "int_gvar = 1; int_gvar;");
    assert(2, ({ int a; a = 2; intptr_gvar = &a; *intptr_gvar; }), "int a; a = 2; intptr_gvar = &a; *intptr_gvar;");
    assert(3, ({ intary_gvar[0] = 3; intary_gvar[0]; }), "intary_gvar[0] = 3; intary_gvar[0];");
    assert(1, ({ char a; sizeof(a); }), "char a; sizeof(a);");
    assert(4, ({ char a; a = 4; a; }), "char a; a = 4; a;");
    assert(8, ({ char a; a = 4; a + 4; }), "char a; a = 4; a + 4;");
    assert(8, ({ char a; char b; a = 4; b = 4; a + b; }), "char a; char b; a = 4; b = 4; a + b;");
    assert(4, ({ char a[3]; a[1] = 4; a[1]; }), "char a[3]; a[1] = 4; a[1];");
    assert(3, ({ char x[3]; x[0] = -1; int y; y = 4; x[0] + y; }), "char x[3]; x[0] = -1; int y; y = 4; x[0] + y;");
    assert(1, ({ int a; int b; int c; a = 1; b = 2; c = 3; &a - &b; }), "int a; int b; int c; a = 1; b = 2; c = 3; &a - &b;");
    assert(2, ({ int a; int b; int c; a = 1; b = 2; c = 3; &a - &c; }), "int a; int b; int c; a = 1; b = 2; c = 3; &a - &c;");
    assert(1, ({ int a; int b; int c; a = 1; b = 2; c = 3; *(&b + 1); }), "int a; int b; int c; a = 1; b = 2; c = 3; *(&b + 1);");
    assert(3, ({ int a; int b; int c; a = 1; b = 2; c = 3; *(&b - 1); }), "int a; int b; int c; a = 1; b = 2; c = 3; *(&b - 1);");
    assert(3, ({ int a; int b; int c; a = 1; b = 2; c = 3;;;;; *(&b - 1); }), "int a; int b; int c; a = 1; b = 2; c = 3;;;;; *(&b - 1);");
    assert(4, ({ sizeof(-1); }), "sizeof(-1);");
    assert(4, ({ sizeof -1; }), "sizeof -1;");
    assert(4, ({ sizeof(int); }), "sizeof(int);");
    assert(1, ({ sizeof(char); }), "sizeof(char);");
    assert(65, ({ first("ABC"); }), "first(\"ABC\");");
    assert(24, ({ int x[2][3]; sizeof(x); }), "int x[2][3]; sizeof(x);");
    assert(12, ({ int x[3][3]; sizeof(*x); }), "int x[3][3]; sizeof(*x);");
    assert(12, ({ int x[4][3]; sizeof(*x); }), "int x[4][3]; sizeof(*x);");
    assert(12, ({ int x[3][3]; sizeof(x[0]); }), "int x[3][3]; sizeof(x[0]);");
    assert(4, ({ int x[3][3]; sizeof(x[0][0]); }), "int x[3][3]; sizeof(x[0][0]);");
    assert(3, ({ int x[2][3]; x[0][0] = 3; x[0][0]; }), "int x[2][3]; x[0][0] = 3; x[0][0];");
    assert(0, ({ int x[2][3]; int *y; y=*x; *y=0; 0; }), "int x[2][3]; int *y; y=*x; *y=0; 0;");
    assert(3, ({ int a = 3; a; }), "int a = 3; a;");
    assert(3, ({ char a = 3; a; }), "int a = 3; a;");
    assert(0, ({ int a[3] = {0, 1, 2}; a[0]; }), "int a[3] = {0, 1, 2}; a[0];");
    assert(0, ({ int a[3] = {0}; a[0]; }), "int a[3] = {0}; a[0];");
    assert(0, ({ int a[3] = {0}; a[1]; }), "int a[3] = {0}; a[1];");
    assert(0, ({ int a[3] = {0}; a[2]; }), "int a[3] = {0}; a[2];");
    assert(0, ({ int a[3] = {0, 1}; a[0]; }), "int a[3] = {0, 1}; a[0];");
    assert(1, ({ int a[3] = {0, 1}; a[1]; }), "int a[3] = {0, 1}; a[1];");
    assert(0, ({ int a[3] = {0, 1}; a[2]; }), "int a[3] = {0, 1}; a[2];");
    assert(0, ({ int a[3] = {0, 1, 2}; a[0]; }), "int a[3] = {0, 1, 2}; a[0];");
    assert(1, ({ int a[3] = {0, 1, 2}; a[1]; }), "int a[3] = {0, 1, 2}; a[1];");
    assert(2, ({ int a[3] = {0, 1, 2}; a[2]; }), "int a[3] = {0, 1, 2}; a[2];");
    assert(0, ({ int a[] = {0, 1, 2}; a[0]; }), "int a[] = {0, 1, 2}; a[0];");
    assert(1, ({ int a[] = {0, 1, 2}; a[1]; }), "int a[] = {0, 1, 2}; a[1];");
    assert(2, ({ int a[] = {0, 1, 2}; a[2]; }), "int a[] = {0, 1, 2}; a[2];");
    assert(65, ({ int a[4] = "ABC"; a[0]; }), "int a[4] = \"ABC\"; a[0];");
    assert(66, ({ int a[4] = "ABC"; a[1]; }), "int a[4] = \"ABC\"; a[1];");
    assert(67, ({ int a[4] = "ABC"; a[2]; }), "int a[4] = \"ABC\"; a[2];");
    assert(0, ({ int a[4] = "ABC"; a[3]; }), "int a[4] = \"ABC\"; a[3];");
    assert(65, ({ int a[] = "ABC"; a[0]; }), "int a[] = \"ABC\"; a[0];");
    assert(66, ({ int a[] = "ABC"; a[1]; }), "int a[] = \"ABC\"; a[1];");
    assert(67, ({ int a[] = "ABC"; a[2]; }), "int a[] = \"ABC\"; a[2];");
    assert(0, ({ int a[] = "ABC"; a[3]; }), "int a[] = \"ABC\"; a[3];");
    assert(65, ({ char *a = "ABC"; a[0]; }), "char *a = \"ABC\"; a[0];");
    assert(66, ({ char *a = "ABC"; a[1]; }), "char *a = \"ABC\"; a[1];");
    assert(67, ({ char *a = "ABC"; a[2]; }), "char *a = \"ABC\"; a[2];");
    assert(0, ({ char *a = "ABC"; a[3]; }), "char *a = \"ABC\"; a[3];");
    assert(10, ({ int i = 0; for (;i < 10;) {i = i + 1; } i; }), "int i = 0; for (;i < 10;) {i = i + 1; } i;");
    assert(0, ({ {}; 0; }), "{}; 0;");
    assert(0, ({ ({{}; 0;}); }), "({{}; 0;});");
    assert(1, ({ sizeof(void); }), "sizeof(void);");
    assert(0, ({ do_nothing(); 0; }), "do_nothing(); 0;");
    assert(65, ({ char *ary[] = {"ABC", "CBA"}; *ary[0]; }), "char *ary[] = {\"ABC\", \"CBA\"}; *ary[0];");
    assert(8, ({ char *ary[] = {"ABC", "CBA"}; sizeof ary[0]; }), "char *ary[] = {\"ABC\", \"CBA\"}; sizeof ary[0];");
    assert(8, ({ char *ary[] = {"ABC", "CBA"}; sizeof *ary; }), "char *ary[] = {\"ABC\", \"CBA\"}; sizeof *ary;");
    assert(67, ({ char *ary[] = {"ABC", "CBA"}; ary[1][0]; }), "char *ary[] = {\"ABC\", \"CBA\"}; ary[1][0];");
    assert(66, ({ char *ary[] = {"ABC", "CBA"}; ary[1][1]; }), "char *ary[] = {\"ABC\", \"CBA\"}; ary[1][1];");
    assert(1, ({ int a = 0; ++a; }), "int a = 0; ++a;");
    assert(1, ({ int a = 0; int b = 0; a = a + ++b; a; }), "int a = 0; int b = 0; a = a + ++b; a;");
    assert(1, ({ int a = 0; int b = 0; a = a + ++b; b; }), "int a = 0; int b = 0; a = a + ++b; b;");
    assert(1, ({ int a[3] = {0, 1, 2}; int *p = a; ++p; *p; }), "int a[3] = {0, 1, 2}; int *p = a; ++p; *p;");
    assert(0, ({ int a = 0; a++; }), "int a = 0; a++;");
    assert(0, ({ int a = 0; int b = 0; a = a + b++; a; }), "int a = 0; int b = 0; a = a + b++; a;");
    assert(1, ({ int a = 0; int b = 0; a = a + b++; b; }), "int a = 0; int b = 0; a = a + b++; b;");
    assert(1, ({ int a[3] = {0, 1, 2}; int *p = a; p++; *p; }), "int a[3] = {0, 1, 2}; int *p = a; p++; *p;");
    assert(55, ({ int total; int i; total = 0; for (i=1; i <= 10; i++) total = total + i; total; }), "int total; int i; total = 0; for (i=1; i <= 10; i++) total = total + i; total;");
    assert(0, ({ int a = 1; --a; }), "int a = 1; --a;");
    assert(1, ({ int a = 1; a--; }), "int a = 1; a--;");
    assert(0, ({ int a = 1; a--; a; }), "int a = 1; a--; a;");
    assert(0, ({ int a[3] = {0, 1, 2}; int *p = a; ++p; --p; *p; }), "int a[3] = {0, 1, 2}; int *p = a; ++p; --p; *p;");
    assert(0, ({ int a[3] = {0, 1, 2}; int *p = a; p++; p--; *p; }), "int a[3] = {0, 1, 2}; int *p = a; p++; p--; *p;");
    assert(55, ({ int total; int i; total = 0; for (i=10; i >= 0; i--) total = total + i; total; }), "int total; int i; total = 0; for (i=10; i >= 0; i--) total = total + i; total;");
    assert(1, ({ int a = 0; a+=1; }), "int a = 0; a+=1;");
    assert(1, ({ char a = 0; a+=1; }), "char a = 0; a+=1;");
    assert(-1, ({ int a = 0; a-=1; }), "int a = 0; a-=1;");
    assert(-1, ({ char a = 0; a-=1; }), "char a = 0; a-=1;");
    assert(1, ({ int ary[] = {0, 1, 2}; int *p = ary; p += 1; *p; }), "int ary[] = {1, 2, 3}; int *p = ary; p += 1; *p;");
    assert(2, ({ int ary[] = {0, 1, 2}; int *p = ary; p += 2; *p; }), "int ary[] = {1, 2, 3}; int *p = ary; p += 2; *p;");
    assert(4, ({ int a = 1; a*=4; }), "int a = 1; a*=4;");
    assert(2, ({ int a = 4; a/=2; }), "int a = 4; a/=2;");
    assert(1, ({ struct {int a; int b;} X; 1; }), "struct {int a; int b;} X; 1;");
    assert(8, ({ struct {int a; int b;} X; sizeof X; }), "struct {int a; int b;} X; sizeof X;");
    assert(5, ({ struct {int a; char b;} X; sizeof X; }), "struct {int a; char b;} X; sizeof X;");
    assert(15, ({ struct {int a; char b;} X[3]; sizeof X; }), "struct {int a; char b;} X[3]; sizeof X;");
    assert(1, ({ struct {int a; char b;} X; X.a = 1; X.b = 2; X.a; }), "struct {int a; char b;} X; X.a = 1; X.b = 2; X.a;");
    assert(2, ({ struct {int a; char b;} X; X.a = 1; X.b = 2; X.b; }), "struct {int a; char b;} X; X.a = 1; X.b = 2; X.b;");
    assert(1, ({ struct X {int a; char b;}; struct X x; x.a = 1; x.b = 2; x.a; }), "struct X {int a; char b;}; struct X x; x.a = 1; x.b = 2; x.a;");
    assert(2, ({ struct X {int a; char b;}; struct X x; x.a = 1; x.b = 2; x.b; }), "struct X {int a; char b;}; struct X x; x.a = 1; x.b = 2; x.b;");
    assert(1, ({ struct Vector {int x; int y;}; struct Vector a; struct Vector *pa = &a; pa->x = 1; pa->x; }), "struct Vector {int x; int y;}; struct Vector a; struct Vector *pa = &a; pa->x = 1; pa->x;");
    assert(2, ({ struct Vector {int x; int y;}; struct Vector a; struct Vector *pa = &a; pa->x = 1; pa->y = 2; pa->y; }), "struct Vector {int x; int y;}; struct Vector a; struct Vector *pa = &a; pa->x = 1; pa->y = 2; pa->y;");
    assert(2, ({ struct Vector {int x; int y;}; struct Vector a; struct Vector *pa = &a; pa->x = 1; pa->y = 2; a.y; }), "struct Vector {int x; int y;}; struct Vector a; struct Vector *pa = &a; pa->x = 1; pa->y = 2; a.y;");
    assert(55, ({ int total = 0; for (int i = 0; i <= 10; i++) total = total + i; total; }), "int total = 0; for (int i = 0; i <= 10; i++) total = total + i; total;");
    assert(1, ({ typedef struct {int x; int y;} Vector; Vector a; a.x = 1; a.y = 2; a.x; }), "typedef struct {int x; int y;} Vector; Vector a; a.x = 1; a.y = 2; a.x;");
    assert(2, ({ typedef struct {int x; int y;} Vector; Vector a; a.x = 1; a.y = 2; a.y; }), "typedef struct {int x; int y;} Vector; Vector a; a.x = 1; a.y = 2; a.y;");
    assert(1, ({ typedef struct {int x; int y;} Vector; Vector a; Vector *pa = &a; pa->x = 1; pa->y = 2; a.x; }), "typedef struct {int x; int y;} Vector; Vector a; Vector *pa = &a; pa->x = 1; pa->y = 2; a.x;");
    assert(8, ({ sizeof(long); }), "sizeof(long);");
    assert(2, ({ sizeof(short); }), "sizeof(short);");
    assert(8, ({ long x; sizeof(x); }), "long x; sizeof(x);");
    assert(2, ({ short x; sizeof(x); }), "short x; sizeof(x);");
    assert(2, ({ short x = 4; short y = 2; x - y; }), "short x = 4; short y = 2; x - y;");
    assert(2, ({ long x = 4; long y = 2; x - y; }), "long x = 4; long y = 2; x - y;");
    assert(5, ({ int i = 0; while (1) { i++; if (i == 5) break; } i; }), "int i = 0; while (1) { i++; if (i == 5) break; } i;");
    assert(15, ({ int total = 0; for (int i = 0; i <= 10; i++) { total = total + i; if (i == 5) break; } total; }), "int total = 0; for (int i = 0; i <= 10; i++) { total = total + i; if (i == 5) break; } total;");
    assert(15, ({ int total = 0; for (int i = 0; i <= 10; i++) { if (i > 5) continue; total = total + i; } total; }), "int total = 0; for (int i = 0; i <= 10; i++) { total = total + i; if (i == 5) break; } total;");
    assert(1, ({ 1 ? 1 : 0; }), "1 ? 1 : 0;");
    assert(0, ({ 0 ? 1 : 0; }), "1 ? 1 : 0;");
    assert(2, ({ 0 ? 1 : 1 ? 2 : 3; }), "0 ? 1 : 1 ? 2 : 3;");
    assert(3, ({ 1 ? 0 ? 2 : 3 : 4; }), "1 ? 0 ? 2 : 3 : 4;");
    assert(0, ({ !1; }), "!1;");
    assert(1, ({ !0; }), "!0;");
    assert(5, ({ (1, 2, 3, 4, 5); }), "(1, 2, 3, 4, 5);");
    assert(3, ({ int a; (a = 3, a); }), "int a; (a = 3, a);");
    assert(1, ({ 0 < 1 < 2; }), "0 < 1 < 2;");  // same as (0 < 1) < 2;
    assert(0, ({ 0 < 1 < 1; }), "0 < 1 < 1;");  // same as (0 < 1) < 1;
    assert(0, ({ _Bool x = 0; x; }), "_Bool x = 0; x;");
    assert(1, ({ _Bool x = 1; x; }), "_Bool x = 1; x;");
    assert(1, ({ _Bool x = 2; x; }), "_Bool x = 2; x;");
    assert(1, ({ _Bool x = -1; x; }), "_Bool x = -1; x;");
    assert(2, ({ sizeof "\n"; }), "sizeof \"\\n\";");
    assert(2, ({ sizeof "\a"; }), "sizeof \"\\a\";");
    assert(2, ({ sizeof "\""; }), "sizeof \"\\\"\";");
    assert(65, 'A', "'A';");
    assert(10, '\n', "\'\\n\'");
    assert(2147483648, 2147483648, "2147483648");
    assert(8, sizeof(2147483648), "sizeof(2147483648)");
    assert(4, sizeof(2147483647), "sizeof(2147483647)");
    return 0;
}
