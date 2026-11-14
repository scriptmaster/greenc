#pragma once
#include "sys/builtin/types_intrinsic.h"
#include <stdbool.h>

typedef struct sys_console sys_console;
typedef struct {
    i32 top, left, cursor_x, cursor_y;
    bool cursor_visible;
} sys_console_view;

typedef enum {
    SYS_CHAR, SYS_LEFT, SYS_RIGHT, SYS_UP, SYS_DOWN, SYS_HOME, SYS_END, SYS_DELETE, SYS_ESCAPE
} sys_console_key;

/* A GUI process can attach to one session at a time. All operations run on its UI thread. */
sys_console* sys_console_start(const char* executable);
bool sys_console_running(sys_console* console);
bool sys_console_viewport(sys_console* console, u32 columns, u32 rows, sys_console_view* view);
bool sys_console_snapshot(sys_console* console, u64* fingerprint);
bool sys_console_row(sys_console* console, i32 x, i32 y, char* row, u32 width);
bool sys_console_input(sys_console* console, sys_console_key key, char character, bool control, bool shift);
void sys_console_destroy(sys_console* console);
