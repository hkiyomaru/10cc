#!/bin/bash

set -u

assert() {
    expected="$1"
    input="$2"

    echo "${input}" > tmp.c
    ./10cc tmp.c > tmp.s
    cc -static -o tmp tmp.s
    ./tmp
    actual="$?"

    if [[ "$actual" = "$expected" ]]; then
        echo -e "\033[32m[PASSED]\033[m $input => $actual"
    else
        echo -e "\033[31m[FAILED]\033[m $input => $expected expected, but got $actual"
    fi

    rm tmp.c
}

assert 0 "int main () {return 0;}"
assert 42 "int main () {return 21+63-42;}"
assert 42 "int main () {return 21 + 63 - 42;}"
assert 42 "int main () {return 2 * 20 + 2;}"
assert 42 "int main () {return 2 * (20 + 1);}"
assert 42 "int main () {return -2 + 2 * (20 + 2);}"
assert 1 "int main () {return 1 == 1;}"
assert 0 "int main () {return 1 != 1;}"
assert 0 "int main () {return (1 < (5 + -10) * 2) == (10 < (120 - 60));}"
assert 42 "int main () {int a; a = 42; return a;}"
assert 42 "int main () {int a; a = 21; a = a + 21; return a;}"
assert 42 "int main () {int foo; foo = 21; foo = foo + 21; return foo;}"
assert 42 "int main () {return 42; return 24;}"
assert 42 "int main () {if (1) return 42;}"
assert 42 "int main () {if (1) return 42; else return 24;}"
assert 24 "int main () {if (0) return 42; else return 24;}"
assert 10 "int main () {int a; a = 0; while (a < 10) a = a + 1; return a;}"
assert 55 "int main () {int total; int i; total = 0; for (i=1; i <= 10; i=i+1) total = total + i; return total;}"
assert 3 "int foo(int a, int b) {return a + b;} int main() {return 0 + foo(1, 2);}"
assert 13 "int fibo(int n) {if (n < 2) return 1; else return fibo(n-2) + fibo(n-1);} int main() {return fibo(6);}"
assert 3 "int main() {int x; int *y; x = 3; y = &x; return *y;}"
assert 42 "int ***ptr() {int foo; int *var; return 42;} int main() {int *****a; return ptr();}"
assert 1 "int main() {int a; int b; a = 1; b = 2; int *p; p = &b; int *q; q = p + 1; return *q;}"
assert 4 "int main() {int a; return sizeof(a);}"
assert 8 "int main() {int *a; return sizeof(a);}"
assert 0 "int main() {int a[3]; return 0;}"
assert 12 "int main() {int a[3]; return sizeof(a);}"
assert 1 "int main() {int a[3]; *a = 1; return *a;}"
assert 6 "int main() {int a[2]; *a = 2; *(a + 1) = 4; return *a + *(a + 1);}"
assert 3 "int main() {int a[2]; *a = 1; *(a + 1) = 2; return a[0] + a[1];}"
assert 3 "int main() {int a[2]; *a = 1; *(a + 1) = 2; return 0[a] + 1[a];}"
assert 0 "int gvar; int main () {return 0;}"
assert 0 "int *gvar; int main () {return 0;}"
assert 1 "int gvar; int main () {gvar = 1; return gvar;}"
assert 2 "int *gvar; int main () {int a; a = 2; gvar = &a; return *gvar;}"
assert 3 "int gvar[2]; int main () {gvar[0] = 3; return gvar[0];}"
assert 1 "int main() {char a; return sizeof(a);}"
assert 4 "int main() {char a; a = 4; return a;}"
assert 8 "int main() {char a; a = 4; return a + 4;}"
assert 8 "int main() {char a; char b; a = 4; b = 4; return a + b;}"
assert 4 "int main() {char a[3]; a[1] = 4; return a[1];}"
assert 3 "int main() {char x[3]; x[0] = -1; int y; y = 4; return x[0] + y;}"
assert 3 "int main() {char x[3]; /** ARY **/ x[0] = -1; int y; y = 4; return x[0] + y;}"
assert 1 "int main() {int a; int b; int c; a = 1; b = 2; c = 3; return &a - &b;}"
assert 2 "int main() {int a; int b; int c; a = 1; b = 2; c = 3; return &a - &c;}"
assert 1 "int main() {int a; int b; int c; a = 1; b = 2; c = 3; return *(&b + 1);}"
assert 3 "int main() {int a; int b; int c; a = 1; b = 2; c = 3; return *(&b - 1);}"
