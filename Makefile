.PHONY: all build compiler

CC = gcc
CFLAGS = -Os -std=c17 -DGREENC_MAIN -static-libgcc -ffreestanding -fno-builtin -nostdlib
compiler_headers = $(wildcard $(1)/*.h) $(foreach directory,$(wildcard $(1)/*/),$(call compiler_headers,$(directory)))

all: compiler
build: compiler
compiler: greenc.exe

greenc.exe: compiler/main.c $(call compiler_headers,compiler)
	$(CC) $(CFLAGS) -Icompiler/libs compiler/main.c -o $@ -lshell32 -lkernel32 -luser32 -Wl,-egreenc_start
