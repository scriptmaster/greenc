/*
 ** document_viewport.c - 2 functions.
 ** editor_measure_viewport - Measure the document area and map visible logical lines.
 ** editor_scroll_to_cursor - Keep the caret visible during keyboard navigation.
 ** See also: view_internal.h.
 */
#include "row_index.h"

bool editor_measure_viewport(ui_renderer* renderer, ui_window* window,
                                    editor_document* document,
                                    const editor_settings* settings, editor_view* view,
                                    editor_viewport* viewport) {
    if (!ui_window_client_size(window, &viewport->width, &viewport->height)) {
        return false;
    }

    ui_renderer_cell_size(renderer, &viewport->cell_width, &viewport->cell_height);
    const editor_layout* layout = &settings->layout; ///read
    u32 bottom = viewport->height > layout->footer_height ? viewport->height - layout->footer_height : 0;
    viewport->height -= editor_terminal_size(&view->terminal, bottom);
    viewport->width -= editor_tree_width(&view->tree, viewport->width);
    if (!view->mapped_rows || view->mapped_revision != document->revision) {
        if (!editor_index_rows(view, document)) { return false; }
    }

    u32 digits = 4;
    for (size_t number = view->row_map[view->mapped_rows - 1].logical + 1; number >= 10000; number /= 10) {
        ++digits;
    }

    viewport->text_left = layout->margin + (digits + 3) * viewport->cell_width;
    if (viewport->height <= layout->text_top + layout->footer_height ||
        viewport->width <= viewport->text_left + layout->margin) {
        return true;
    }

    viewport->rows = (viewport->height - layout->text_top - layout->footer_height) /
                     viewport->cell_height;
    viewport->columns = (viewport->width - viewport->text_left - layout->margin) / viewport->cell_width;

    if (viewport->columns && view->longest_line >= viewport->columns) {
        u32 available = viewport->height - layout->text_top - layout->footer_height;
        viewport->rows = available > 12 ? (available - 12) / viewport->cell_height : 0;
    }

    size_t row = editor_cursor_row(view, document->cursor);
    if (document->cursor > view->row_map[row].end) {
        /* Keyboard navigation into a hidden range reveals its containing fold. */
        document->folded[view->row_map[row].offset] &= ~1u;
        editor_document_changed(document);
        if (!editor_index_rows(view, document)) { return false; }
        row = editor_cursor_row(view, document->cursor);
    }

    viewport->map = view->row_map;
    viewport->visible_lines = view->mapped_rows;
    viewport->cursor_line = row;
    viewport->cursor_column = document->cursor - view->row_map[row].offset;

    return true;
}

void editor_scroll_to_cursor(editor_view* view, const editor_viewport* viewport) {
    if (!view->manual_scroll && viewport->cursor_line < view->first_line) {
        view->first_line = viewport->cursor_line;
    } else if (!view->manual_scroll && viewport->cursor_line >= view->first_line + viewport->rows) {
        view->first_line = viewport->cursor_line - viewport->rows + 1;
    }

    size_t maximum = view->longest_line >= viewport->columns ? view->longest_line - viewport->columns + 1 : 0;
    if (view->first_column > maximum) { view->first_column = maximum; }
    if (view->manual_scroll) { return; }

    if (viewport->cursor_column < view->first_column) {
        view->first_column = viewport->cursor_column;
    } else if (viewport->cursor_column >= view->first_column + viewport->columns) {
        view->first_column = viewport->cursor_column - viewport->columns + 1;
    }
}
