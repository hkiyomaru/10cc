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

assert 0 0
assert 42 42
assert 42 "21+63-42"
assert 42 "21 + 63 - 42"
assert 42 "2 * 20 + 2"
assert 42 "2 * (20 + 1)"
assert 42 "-2 + 2 * (20 + 2)"
assert 1 "1 == 1"
assert 0 "(1 < (5 + -10) * 2) == (10 < (120 - 60))"

echo OK
