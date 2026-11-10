CC = gcc
CFLAGS = -Os -std=c17 -Wall -Wextra -Werror

.PHONY: all run clean
all: gpui.exe
gpui.exe: main.c ../ui_window.h ../../../compiler/libs/sys/builtin/types_intrinsic.h ../ui_window_win32.h ../ui_window_win32.c ../ui_gpu_win32.c
	$(CC) $(CFLAGS) main.c ../ui_window_win32.c ../ui_gpu_win32.c -o $@ -mwindows -lopengl32 -lgdi32 -luser32
run: gpui.exe
	./gpui.exe
clean:
	$(RM) gpui.exe
