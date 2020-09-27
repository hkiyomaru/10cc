CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

10cc: $(OBJS)
	$(CC) -o 10cc $(OBJS) $(LDFLAGS)

$(OBJS): 10cc.h

test.o: test/test.h
	gcc -xc -c -o test.o test/test.h

test: 10cc test.o
	./10cc test/tests.c > tmp.s
	gcc -static -o tmp tmp.s test.o
	./tmp

clean:
	rm -f 10cc *.o tmp*

.PHONY: test clean
