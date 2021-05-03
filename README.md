# 10cc

[![Build](https://github.com/hkiyomaru/10cc/actions/workflows/build.yml/badge.svg)](https://github.com/hkiyomaru/10cc/actions/workflows/build.yml)
[![Test](https://github.com/hkiyomaru/10cc/actions/workflows/test.yml/badge.svg)](https://github.com/hkiyomaru/10cc/actions/workflows/test.yml)

A reimplementation of [9cc](https://github.com/rui314/9cc).

## Getting started

### Development environment

10cc runs on a 64-bit version of Linux with a few development tools such as gcc and make. This project uses docker to prepare such a development environment. The following command builds a docker image named "10cc" from this [Dockerfile](./Dockerfile).

```commandline
$ docker build -t 10cc .
```

### Build

To build 10cc, run `make`.

```commandline
$ docker run -it --rm -v $(pwd):/10cc -w /10cc 10cc make
```

You will find an executable file, `10cc`, in the `bld` directory.

```commandline
$ docker run -it --rm -v $(pwd):/10cc -w /10cc 10cc ls -l ./bld/10cc
```

### Test

To test 10cc, run `make test`.

```commandline
$ docker run -it --rm -v $(pwd):/10cc -w /10cc 10cc make test
```

### Compile a C program

The [examples](./examples) directory includes several C programs that can be compiled using 10cc.
To compile [fibo.c](./examples/fibo.c) and run the executable file, run the following commands:

```commandline
$ docker run -it --rm -v $(pwd):/10cc -w /10cc 10cc   # Get into the docker container's shell.
$ make                                                # Build 10cc.
$ ./bld/10cc examples/fibo.c > fibo.s                 # Compile fibo.c using 10cc.
$ cc -std=c11 -static -c -o testkit.o test/testkit.c  # Compile the dependency using *cc*.
$ cc -std=c11 -g -static fibo.s testkit.o -o fibo     # Link them to create an executable file.
$ ./fibo                                              # Run.
```

## How 10cc works

10cc consists of four stages.

1. [Tokenization](./src/tokenize.c): A tokenizer takes code and breaks it into a list of tokens.
2. [Preprocessing](./src/preprocess.c): A preprocessor takes a list of tokens and creates a new list by expanding macros.
3. [Parsing](./src/parse.c): A recursive descent parser takes a list of macro-expanded tokens and builds abstract syntax trees (ASTs).
4. [Code generation](./src/codegen.c): A code generator takes ASTs and emits assembly code for them.

## Reference

- [低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook): A compiler textbook (in Japanese).
- [9cc](https://github.com/rui314/9cc): A small C compiler.
- [chibicc](https://github.com/rui314/chibicc): Yet another small C compiler.
- [N1570](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf): The final draft of C11.
- [Compiler Explorer](https://godbolt.org): An online compiler.
