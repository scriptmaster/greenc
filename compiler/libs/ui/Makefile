CC = gcc
CFLAGS = -Os -std=c17 -Wall -Wextra -Werror
SOURCES = tests_win32.c ui_window_win32.c ui_gpu_win32.c ui_renderer_win32.c ../sys/fs/write_atomic_win32.c
HEADERS = ui_window.h ui_window_win32.h ../sys/builtin/types_intrinsic.h ../sys/fs/write_atomic.h

.PHONY: test clean
test: tests_win32.exe
	./tests_win32.exe
tests_win32.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ -lopengl32 -lgdi32 -luser32 -Wl,--wrap=ui_gpu_create -Wl,--wrap=ui_gpu_present_framebuffer
clean:
	$(RM) tests_win32.exe
