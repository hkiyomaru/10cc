#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [[ "$actual" = "$expected" ]]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
    fi
}

assert 0 "0;"
assert 42 "21+63-42;"
assert 42 "21 + 63 - 42;"
assert 42 "2 * 20 + 2;"
assert 42 "2 * (20 + 1);"
assert 42 "-2 + 2 * (20 + 2);"
assert 1 "1 == 1;"
assert 0 "(1 < (5 + -10) * 2) == (10 < (120 - 60));"
assert 42 "a = 42;"
assert 42 "a = 0; b = 42;"
assert 42 "a = 0; b = 21 + 21;"
assert 42 "a = 21; a = a + 21;"
assert 42 "foo = 21; foo = foo + 21;"
assert 42 "_foo123 = 21; _foo123 = _foo123 + 21;"
assert 42 "return 42; return 24;"
assert 42 "if (1) return 42;"
assert 42 "if (1) return 42; else return 24;"
assert 24 "if (0) return 42; else return 24;"
assert  5 "num=10; if ((num/3)*3 == num) return 3; else if ((num/5) * 5 == num) return 5; else return 0;"
assert 10 "a = 0; while (a < 10) a = a + 1;"
assert 55 "total = 0; for (i=1; i <= 10; i=i+1) total = total + i; return total;"
assert 0 "{0; 0;}"
assert 110 "total = 0; for (i=1; i <= 10; i=i+1) {total = total + i; total = total + i;} return total;"

echo OK
