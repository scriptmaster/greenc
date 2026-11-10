all: build

CC=gcc
OPT=-Os
CSTD=-std=c17
CFLAGS=$(OPT) $(CSTD) -DGREENC_MAIN -static-libgcc -ffreestanding -fno-builtin -nostdlib
WINFLAGS=-lshell32 -lkernel32 -luser32 -Wl,-egreenc_start
INCLUDES=./libs/
TIME=time

build:
	$(TIME) $(CC) $(CFLAGS) -I$(INCLUDES) main.c -o greenc.exe $(WINFLAGS)
	ls -lah greenc.exe
	ldd greenc.exe
	cp greenc.exe ../
	cd .. && greenc.exe -o main.exe src/main.c

# LXCC=zig cc
# LXFLAGS=-target x86_64-linux-musl $(OPT) $(STD) -static-libgcc -static

LXCC=x86_64-pc-linux-gnu-gcc.exe
LXFLAGS=$(OPT) $(CSTD) -nostdlib -static-libgcc -static -ffreestanding -fno-builtin
# we will implement -ldl later.

linux:
	timed $(LXCC) $(LXFLAGS) -I$(INCLUDES) main.c -o greenc.elf
	ls -lah greenc.elf
	tinyemu.com greenc.elf
	emulator.com greenc.elf
