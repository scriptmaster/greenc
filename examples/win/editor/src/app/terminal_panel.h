/*
 ** terminal_panel.h - 2 functions.
 ** editor_terminal_size - Limit the terminal height to its available space.
 ** editor_terminal_draw - Compose the terminal strip, screen and profile picker.
 ** See also: terminal_panel.c, editor_terminal_types.h.
 */
#pragma once
#include "editor_terminal_types.h"

u32 editor_terminal_size(const editor_terminal* panel, u32 height);
void editor_terminal_draw(editor_terminal* panel, ui_renderer* renderer, u32 width, u32 bottom,
                          const editor_settings* settings);
