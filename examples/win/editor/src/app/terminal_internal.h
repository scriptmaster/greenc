/*
 ** terminal_internal.h - 3 functions.
 ** new_session - Start a shell and apply the configured Bash fallback.
 ** close_session - Confirm and stop the selected shell session.
 ** profile_menu_left - Anchor the profile picker within the panel.
 ** See also: terminal_sessions.c, terminal_toolbar.c, editor_terminal.h.
 */
#pragma once
#include "editor_terminal.h"
#include <string.h>

enum { DIVIDER_HEIGHT = 6, PROFILE_CELLS = 16 };
extern const char* const profiles[3];

bool new_session(editor_terminal* panel, const char* shell);
void close_session(editor_terminal* panel, size_t index);
u32 profile_menu_left(const editor_tab_strip* strip, u32 menu_width);
