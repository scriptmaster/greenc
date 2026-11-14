/*
 ** editor_debug.h - 4 functions.
 ** editor_debug_start - Load a symbol-bearing executable and install marked lines.
 ** editor_debug_key - Start, resume, step or stop the debugger.
 ** editor_debug_poll - Read debugger events and activate stopped source lines.
 ** editor_debug_stop - Release the debugger and its target process.
 ** See also: debug_session.c, debug_events.c.
 */
#pragma once
#include "editor_view.h"
bool editor_debug_start(editor_view* view, editor_document* active, const char* executable);
void editor_debug_key(editor_view* view, editor_document* document, editor_settings* settings, ui_window* window, ui_input input);
void editor_debug_poll(editor_view* view, editor_document* document, editor_settings* settings);
void editor_debug_stop(editor_debug* debug);
