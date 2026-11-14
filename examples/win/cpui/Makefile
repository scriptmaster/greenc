CC = gcc
UI = ../../../compiler/libs/ui
CFLAGS = -Os -std=c17 -Wall -Wextra -Werror

.PHONY: all run clean
all: cpui.exe
cpui.exe: main.c $(UI)/ui_window.h ../../../compiler/libs/sys/builtin/types_intrinsic.h $(UI)/ui_window_win32.h $(UI)/ui_window_win32.c
	$(CC) $(CFLAGS) main.c $(UI)/ui_window_win32.c -o $@ -mwindows -lgdi32 -luser32
run: cpui.exe
	./cpui.exe
clean:
	$(RM) cpui.exe
