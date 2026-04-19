all: build

CC=gcc
OPT=-Os
CSTD=-std=c17
CFLAGS=$(OPT) $(CSTD) -nostdlib -static-libgcc -ffreestanding -fno-builtin
WINFLAGS=-lshell32 -lkernel32 -luser32 -Wl,-egreenc_main
INCLUDES=./libs/

build:
	timed $(CC) $(CFLAGS) -I$(INCLUDES) main.c -o greenc.exe $(WINFLAGS)
	ls -lah greenc.exe
	ldd greenc.exe
	# greenc.exe
	# greenc.exe --help
	# greenc.exe build ../
	greenc.exe build ../ -o ../build/app/

# LXCC=zig cc
# LXFLAGS=-target x86_64-linux-musl $(OPT) $(STD) -static-libgcc -static

LXCC=x86_64-pc-linux-gnu-gcc.exe
LXFLAGS=$(OPT) $(CSTD) -nostdlib -static-libgcc -static -ffreestanding -fno-builtin

linux:
	timed $(LXCC) $(LXFLAGS) -I$(INCLUDES) main.c -o greenc.elf
	ls -lah greenc.elf
	tinyemu.com greenc.elf
	emulator.com greenc.elf
