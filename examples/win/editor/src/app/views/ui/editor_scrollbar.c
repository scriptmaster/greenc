/*
 ** editor_scrollbar.c - 4 functions.
 ** editor_scrollbar_draw - Paint the common scrollbar track and thumb.
 ** editor_scrollbar_layout - Calculate thumb size and travel for the visible rows.
 ** editor_thumb_top - Convert a document row into a thumb position.
 ** editor_scroll_position - Convert a dragged thumb position into the first visible row.
 ** See also: editor_scrollbar.h.
 */
#include "editor_scrollbar.h"

editor_scrollbar editor_scrollbar_layout(i32 right, i32 top, i32 height, size_t rows, size_t total) {
    editor_scrollbar bar = {.x = right - 16, .top = top, .height = height};
    if (!rows || total <= rows || height <= 0) { return bar; }

    bar.maximum = total - rows;
    bar.thumb = (i32)((u64)height * rows / total);
    if (bar.thumb < 18) { bar.thumb = height < 18 ? height : 18; }
    bar.travel = height - bar.thumb;

    return bar;
}

i32 editor_thumb_top(editor_scrollbar bar, size_t first) {
    if (first > bar.maximum) { first = bar.maximum; }

    return bar.top + (bar.maximum ? (i32)((f64)first / bar.maximum * bar.travel) : 0);
}

size_t editor_scroll_position(editor_scrollbar bar, i32 thumb_top) {
    if (thumb_top <= bar.top || !bar.travel) { return 0; }
    if (thumb_top >= bar.top + bar.travel) { return bar.maximum; }

    size_t pixels = (size_t)(thumb_top - bar.top);
    size_t travel = (size_t)bar.travel;

    return pixels * (bar.maximum / travel) +
           (size_t)(((u64)pixels * (bar.maximum % travel) + travel / 2) / travel);
}

void editor_scrollbar_draw(editor_scrollbar bar, ui_renderer* renderer, size_t first, u32 track, u32 thumb) {
    if (!bar.maximum) { return; }

    ui_renderer_rect(renderer, bar.x, bar.top, 10, bar.height, track);
    ui_renderer_rect(renderer, bar.x + 2, editor_thumb_top(bar, first), 6, bar.thumb, thumb);
}
