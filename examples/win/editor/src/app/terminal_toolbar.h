/*
 ** terminal_toolbar.h - 1 function.
 ** editor_terminal_click - Dispatch terminal tab and profile choices.
 ** See also: terminal_toolbar.c, editor_terminal_types.h.
 */
#pragma once
#include "editor_terminal_types.h"

void editor_terminal_click(editor_terminal* panel, ui_renderer* renderer, u32 width, i32 x, i32 y, const editor_settings* settings);
