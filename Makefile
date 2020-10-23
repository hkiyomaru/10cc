CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

10cc: $(OBJS)
	$(CC) -o 10cc $(OBJS) $(LDFLAGS)

$(OBJS): 10cc.h

test_tools.o: test/test_tools.c
	gcc -xc -c -o test_tools.o test/test_tools.c

test: 10cc test_tools.o
	./10cc test/tests.c > tmp.s
	gcc -static -o tmp tmp.s test_tools.o
	./tmp

clean:
	rm -f 10cc *.o tmp*

.PHONY: test clean
