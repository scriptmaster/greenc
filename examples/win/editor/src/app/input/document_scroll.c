/*
 ** document_scroll.c - 3 functions.
 ** document_horizontal_bar - Map document columns to horizontal track positions.
 ** document_scrollbar - Measure the scrollbar above the terminal.
 ** editor_scroll_input - Apply document wheel, track and thumb input.
 ** See also: editor_view.h, view_internal.h.
 */
#include "view_internal.h"

editor_scrollbar document_horizontal_bar(const editor_viewport* viewport, size_t longest, const editor_settings* settings) {
    i32 bottom = (i32)viewport->height - (i32)settings->layout.footer_height;
    i32 width = (i32)(viewport->columns * viewport->cell_width);

    return editor_scrollbar_layout(bottom + 4, (i32)viewport->text_left, width, viewport->columns, longest + 1);
}

editor_scrollbar document_scrollbar(const editor_viewport* viewport, const editor_settings* settings) {
    return editor_scrollbar_layout((i32)viewport->width, (i32)settings->layout.text_top,
                                   (i32)(viewport->rows * viewport->cell_height), viewport->rows, viewport->visible_lines);
}

bool editor_scroll_input(editor_view* view, editor_document* document, ui_renderer* renderer,
                         ui_window* window, const editor_settings* settings, ui_input input) {
    editor_viewport viewport = {0};
    if (!editor_measure_viewport(renderer, window, document, settings, view, &viewport)) { return false; }

    editor_scrollbar horizontal = document_horizontal_bar(&viewport, view->longest_line, settings);
    bool over_track = input.x >= horizontal.top && input.x < horizontal.top + horizontal.height &&
                      input.y >= horizontal.x && input.y < horizontal.x + 10;

    if (!view->scrolling && horizontal.maximum && (view->horizontal_drag || (input.mouse == UI_MOUSE_DOWN && over_track))) {
        if (!view->horizontal_drag) {
            i32 offset = input.x - editor_thumb_top(horizontal, view->first_column);
            view->thumb_offset = offset >= 0 && offset < horizontal.thumb ? offset : horizontal.thumb / 2;
        }

        view->first_column = editor_scroll_position(horizontal, input.x - view->thumb_offset);
        view->horizontal_drag = input.mouse != UI_MOUSE_UP;
        view->manual_scroll = true;

        return true;
    }

    view->horizontal_drag = false;
    editor_scrollbar bar = document_scrollbar(&viewport, settings);
    return editor_scroll_rows(view, bar, viewport.width, &view->first_line, input);
}
