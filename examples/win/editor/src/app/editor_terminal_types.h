/*
 ** editor_terminal_types.h - 0 functions.
 ** Define the shared data and contracts for this module.
 ** See also: editor_tab_strip.h, editor_terminal.h, terminal_panel.h, terminal_sessions.h, terminal_toolbar.h.
 */
#pragma once
#include "editor_tab_strip.h"
#include "sys/console.h"

enum { EDITOR_TERMINAL_LIMIT = 6 };
typedef struct {
    sys_console* console;
    const char* name;
    u64 snapshot;
    u32 columns, rows;
    bool changed;
} editor_session;

typedef struct {
    editor_session sessions[EDITOR_TERMINAL_LIMIT];
    size_t count, active;
    bool visible, focused, resizing, choosing;
    editor_tab_scroll scroll;
    u32 height;
    const char* notice;
} editor_terminal;
