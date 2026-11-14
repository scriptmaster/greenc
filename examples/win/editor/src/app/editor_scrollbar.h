/*
 ** editor_scrollbar.h - 4 functions.
 ** editor_scrollbar_draw - Paint the common scrollbar track and thumb.
 ** editor_scrollbar_layout - Calculate thumb size and travel for the visible rows.
 ** editor_thumb_top - Convert a document row into a thumb position.
 ** editor_scroll_position - Convert a dragged thumb position into the first visible row.
 ** See also: editor_scrollbar.c.
 */
#pragma once
#include "ui/ui_window.h"

typedef struct {
    i32 x, top, height, thumb, travel;
    size_t maximum;
} editor_scrollbar;

editor_scrollbar editor_scrollbar_layout(i32 right, i32 top, i32 height, size_t rows, size_t total);
i32 editor_thumb_top(editor_scrollbar bar, size_t first);
size_t editor_scroll_position(editor_scrollbar bar, i32 thumb_top);

void editor_scrollbar_draw(editor_scrollbar bar, ui_renderer* renderer, size_t first, u32 track, u32 thumb);
