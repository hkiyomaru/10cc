#include <stdio.h>
#include <stdlib.h>

void assert(long expected, long actual, char *code) {
    if (expected == actual) {
        printf("\033[32m[PASSED]\033[m %s => %ld\n", code, expected);
    } else {
        printf("\033[31m[[FAILED]\033[m %s => %ld expected, but got %ld\n", code, expected, actual);
    }
}
