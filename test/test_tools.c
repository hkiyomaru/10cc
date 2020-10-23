#include <stdio.h>
#include <stdlib.h>

int assert(int expected, int actual, char *code) {
    if (expected == actual) {
        printf("\033[32m[PASSED]\033[m %s => %d\n", code, expected);
        return 0;
    } else {
        printf("\033[31m[[FAILED]\033[m %s => %d expected, but got %d\n", code, expected, actual);
        return 1;
    }
}
