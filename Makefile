.PHONY: all build compiler

OUT = calm3
COMPILER=greenc

CC = gcc
CFLAGS = -Os
SOURCES = src/main.c

$(OUT): compiler

compiler:
	make -C compiler

build: $(OUT)
	ls -lah $(OUT)
