/*
 ** debug_types.h - 0 functions.
 ** Own the debugger transport and pending event state.
 ** See also: editor_debug.h.
 */
#pragma once
#include "sys/shell.h"

typedef struct {
    sys_shell* process;
    char record[8192], source[260];
    size_t length, line;
    bool paused, discard;
} editor_debug;

