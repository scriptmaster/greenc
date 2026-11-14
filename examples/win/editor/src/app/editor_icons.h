/*
 ** editor_icons.h - 3 functions.
 ** editor_triangle - Rasterize a rotated triangle with antialiased edges.
 ** editor_disclosure - Draw the animated expansion indicator.
 ** editor_plus - Paint an add button glyph.
 ** See also: editor_icons.c.
 */
#pragma once
#include "ui/ui_window.h"

void editor_triangle(ui_renderer* renderer, i32 x, i32 y, f32 turns, f32 scale, u32 color, u32 background);
void editor_disclosure(ui_renderer* renderer, i32 x, i32 y, bool expanded, u64 changed, u32 color, u32 background);
void editor_plus(ui_renderer* renderer, i32 x, i32 y, u32 color, u32 background);
