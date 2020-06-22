#!/bin/bash

set -u

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [[ "$actual" = "$expected" ]]; then
        echo -e "\033[32m[PASSED]\033[m $input => $actual"
    else
        echo -e "\033[31m[FAILED]\033[m $input => $expected expected, but got $actual"
    fi
}

assert 0 "main () {return 0;}"
assert 42 "main () {return 21+63-42;}"
assert 42 "main () {return 21 + 63 - 42;}"
assert 42 "main () {return 2 * 20 + 2;}"
assert 42 "main () {return 2 * (20 + 1);}"
assert 42 "main () {return -2 + 2 * (20 + 2);}"
assert 1 "main () {return 1 == 1;}"
assert 0 "main () {return (1 < (5 + -10) * 2) == (10 < (120 - 60));}"
assert 42 "main () {a = 42; return a;}"
assert 42 "main () {a = 21; a = a + 21; return a;}"
assert 42 "main () {foo = 21; foo = foo + 21; return foo;}"
assert 42 "main () {return 42; return 24;}"
assert 42 "main () {if (1) return 42;}"
assert 42 "main () {if (1) return 42; else return 24;}"
assert 24 "main () {if (0) return 42; else return 24;}"
assert 10 "main () {a = 0; while (a < 10) a = a + 1; return a;}"
assert 55 "main () {total = 0; for (i=1; i <= 10; i=i+1) total = total + i; return total;}"
assert 3 "foo(a, b) {return a + b;} main() {return 0 + foo(1, 2);}"
assert 13 "fibo(n) {if (n < 2) return 1; else return fibo(n-2) + fibo(n-1);} main() {return fibo(6);}"

echo OK
