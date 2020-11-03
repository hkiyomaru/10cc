SRCDIR := src
BLDDIR := bld

TARGET := $(BLDDIR)/10cc
CFLAGS := -std=c11 -g -static -Wall

SRCS := $(wildcard $(SRCDIR)/*.c)
HDRS := $(wildcard $(SRCDIR)/*.h)
OBJS := $(patsubst %.c,$(BLDDIR)/%.o,$(notdir $(SRCS)))

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(BLDDIR)/%.o: $(SRCDIR)/%.c $(HDRS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: test
test: test/test
	./$<

test/test: test/test.s test/test_tools.o
	$(CC) $(CFLAGS) -o $@ $^

test/test.s: $(TARGET) test/tests.c
	./$< test/tests.c > $@

test/test_tools.o: test/test_tools.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f $(BLDDIR)/* test/test test/test.s test/test_tools.o
