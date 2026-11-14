/*
 ** document_pointer.c - 1 function.
 ** editor_mouse_select - Map document pointer gestures to selection and folding.
 ** See also: editor_view.h, view_internal.h.
 */
#include "view_internal.h"

void editor_mouse_select(editor_view* view, editor_document* document, ui_renderer* renderer,
                         ui_window* window, const editor_settings* settings, ui_input input) {
    editor_viewport viewport = {0};

    if (!editor_measure_viewport(renderer, window, document, settings, view, &viewport) ||
        !viewport.rows || !viewport.columns) { return; }

    i32 top = (i32)settings->layout.text_top;
    i32 bottom = top + (i32)(viewport.rows * viewport.cell_height);
    bool down = input.mouse == UI_MOUSE_DOWN;

    if (down && (input.y < top || input.y >= bottom)) { return; }
    if (!down && !view->dragging) { return; }

    size_t row = view->first_line;

    if (input.y < top) {
        if (row) { --row; }
    } else {
        row += (size_t)(input.y - top) / viewport.cell_height;
    }

    bool below_text = row >= viewport.visible_lines;
    if (row >= viewport.visible_lines) { row = viewport.visible_lines - 1; }

    size_t offset = viewport.map[row].offset;
    bool debug = input.x >= 0 && input.x < (i32)settings->layout.margin;
    char label[48];
    i32 link = viewport.map[row].hidden ? editor_fold_label(&viewport, view, row, label) : -1;
    bool on_link = link >= 0 && input.x >= link && input.x < link + (i32)(strlen(label) * viewport.cell_width);

    if (down && !below_text && (debug || on_link || (input.x >= (i32)viewport.text_left - 22 && input.x < (i32)viewport.text_left))) {
        view->manual_scroll = true;
        size_t finish = debug ? offset : editor_fold_end(document, offset);
        size_t next = editor_line_end(document, offset) + 1;

        if (debug || finish > next) {
            if (!document->folded) {
                document->folded = calloc(document->capacity ? document->capacity : 1, sizeof(*document->folded));

                if (!document->folded) {
                    document->status = "Insufficient memory for folding";
                    view->dragging = false;

                    return;
                }
            }

            document->folded[offset] ^= debug ? 2 : 1;

            if (debug) {
                if ((document->folded[offset] & 2) && !view->themes.debug_path[0]) {
                    view->themes.open = view->themes.expanded = view->themes.debug = true;
                }

                return;
            }

            editor_document_changed(document);
            document->animated_fold = offset;
            document->fold_changed = ui_time_ms();

            if ((document->folded[offset] & 1) && document->cursor >= next && document->cursor < finish) {
                document->cursor = offset;
                document->selecting = false;
            }
        }

        view->dragging = false;

        return;
    }

    if (down) {
        view->dragging = true;
        view->manual_scroll = false;
        if (input.shift && !document->selecting) { document->anchor = document->cursor; }
    }

    size_t end = editor_line_end(document, offset);
    i32 x = input.x - (i32)viewport.text_left;
    size_t column = view->first_column;

    if (x >= 0) {
        column += ((size_t)x + viewport.cell_width / 2) / viewport.cell_width;
    } else if (column) {
        --column;
    }

    document->cursor = below_text ? document->length : offset + (column < end - offset ? column : end - offset);
    if (input.mouse == UI_MOUSE_DOWN && !input.shift) {
        document->anchor = document->cursor;
    }

    document->selecting = true;
    if (input.mouse == UI_MOUSE_UP) { view->dragging = false; }
}
