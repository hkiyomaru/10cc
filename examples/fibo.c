/**
 * How to build:
 *   $ make                                                      # Build 10cc.
 *   $ ./bld/10cc examples/fibo.c > fibo.s                       # Compile fibo.c using 10cc.
 *   $ cc -std=c11 -static -c -o test_tools.o test/test_tools.c  # Compile test tools using *cc*.
 *   $ cc -std=c11 -g -static fibo.s test_tools.o -o fibo        # Link them to create an executable file.
 *   $ ./fibo                                                    # Run.
 */
void printf();

int fibo(int n) {
    if (n < 2) {
        return n;
    } else {
        return fibo(n - 2) + fibo(n - 1);
    }
}

int main() {
    for (int i = 0; i < 10; i++) {
        printf("%d\n", fibo(i));
    }
}
