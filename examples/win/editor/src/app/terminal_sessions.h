/*
 ** terminal_sessions.h - 3 functions.
 ** editor_terminal_toggle - Show or hide the terminal and start its first session.
 ** editor_terminal_destroy - Release all owned console sessions.
 ** editor_terminal_key - Forward terminal keystrokes to the active shell.
 ** See also: terminal_sessions.c, editor_terminal_types.h.
 */
#pragma once
#include "editor_terminal_types.h"

bool editor_terminal_toggle(editor_terminal* panel, const editor_settings* settings);
void editor_terminal_destroy(editor_terminal* panel);
void editor_terminal_key(editor_terminal* panel, ui_input input);
