/*
 ** view_internal.h - 7 functions.
 ** document_horizontal_bar - Measure scrollable columns along the lower edge.
 ** editor_fold_label - Compute the visible fold-link position and label.
 ** editor_measure_viewport - Measure the document area and map visible logical lines.
 ** editor_scroll_to_cursor - Keep the caret visible during keyboard navigation.
 ** editor_draw_document - Paint document rows, line numbers, folds and selections.
 ** editor_draw_caret - Paint the caret during its visible blink phase.
 ** document_scrollbar - Measure the scrollbar above the terminal.
 ** See also: document_viewport.c, document_rows.c, document_scroll.c, editor_view.h, editor_icons.h, editor_scrollbar.h.
 */
#pragma once
#include "editor_view.h"
#include "editor_icons.h"
#include "editor_scrollbar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct editor_line_entry { size_t offset, end, logical, hidden;
    bool foldable, has_unsupported_byte, in_comment; } editor_line_entry;

typedef struct {
    u32 width, height;
    u32 cell_width, cell_height;
    u32 text_left;
    size_t rows, columns;
    editor_line_entry* map;
    size_t visible_lines, cursor_line, cursor_column;
} editor_viewport;

i32 editor_fold_label(const editor_viewport* viewport, const editor_view* view, size_t row, char label[48]);
bool editor_measure_viewport(ui_renderer* renderer, ui_window* window,
                                    editor_document* document,
                                    const editor_settings* settings, editor_view* view,
                                    editor_viewport* viewport);
void editor_scroll_to_cursor(editor_view* view, const editor_viewport* viewport);
void editor_draw_document(ui_renderer* renderer, const editor_document* document,
                                 const editor_view* view, const editor_viewport* viewport,
                                 const editor_settings* settings);
void editor_draw_caret(ui_renderer* renderer, const editor_view* view,
                              const editor_viewport* viewport, const editor_settings* settings);
editor_scrollbar document_scrollbar(const editor_viewport* viewport, const editor_settings* settings);
editor_scrollbar document_horizontal_bar(const editor_viewport* viewport, size_t longest, const editor_settings* settings);
