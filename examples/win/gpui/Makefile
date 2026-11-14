CC = gcc
UI = ../../../compiler/libs/ui
CFLAGS = -Os -std=c17 -Wall -Wextra -Werror

.PHONY: all run clean
all: gpui.exe
gpui.exe: main.c $(UI)/ui_window.h ../../../compiler/libs/sys/builtin/types_intrinsic.h $(UI)/ui_window_win32.h $(UI)/ui_window_win32.c $(UI)/ui_gpu_win32.c
	$(CC) $(CFLAGS) main.c $(UI)/ui_window_win32.c $(UI)/ui_gpu_win32.c -o $@ -mwindows -lopengl32 -lgdi32 -luser32
run: gpui.exe
	./gpui.exe
clean:
	$(RM) gpui.exe
