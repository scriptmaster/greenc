/*
 ** page_navigation.c - 4 functions.
 ** page_row - Advance three quarters of the visible height within row bounds.
 ** select_page_position - Update the caret while preserving a Shift selection.
 ** editor_text_page - Navigate the visible line map, skipping folded rows.
 ** editor_hex_page - Navigate the active grid within its displayed byte limit.
 ** See also: page_navigation.h, document_viewport.c, hex_view.c.
 */
#include "page_navigation.h"
#include "editor_hex.h"

static size_t page_row(size_t row, size_t height, size_t last, bool down) {
    size_t step = height / 4 * 3 + height % 4 * 3 / 4;

    if (!step) { step = 1; }
    if (row > last) { row = last; }
    if (down) { return step > last - row ? last : row + step; }

    return step > row ? 0 : row - step;
}

static void select_page_position(editor_document* doc, size_t position, bool shift) {
    if (shift && !doc->selecting) { doc->anchor = doc->cursor; }

    doc->selecting = shift;
    doc->cursor = position;
}

void editor_text_page(editor_view* view, editor_document* doc, const editor_viewport* viewport, ui_input input) {
    size_t height = viewport->rows;
    size_t count = viewport->visible_lines;

    if (!height || !count) { return; }

    bool down = input.key == UI_KEY_PAGE_DOWN;
    size_t row = page_row(viewport->cursor_line, height, count - 1, down);
    editor_line_entry line = viewport->map[row];
    size_t column = viewport->cursor_column;

    if (column > line.end - line.offset) { column = line.end - line.offset; }

    select_page_position(doc, line.offset + column, input.shift);
    view->first_line = page_row(view->first_line, height, count > height ? count - height : 0, down);
}

void editor_hex_page(editor_view* view, editor_document* doc, ui_input input) {
    size_t limit = editor_hex_limit(doc);
    size_t cols = view->hex_columns;
    size_t height = view->hex_rows;

    if (!limit || !cols || !height) { return; }

    bool down = input.key == UI_KEY_PAGE_DOWN;
    size_t last = limit - 1;
    size_t last_row = last / cols;
    size_t start = cols * page_row(doc->cursor / cols, height, last_row, down);
    size_t column = doc->cursor % cols;

    select_page_position(doc, column > last - start ? last : start + column, input.shift);
    doc->low_nibble = false;
    doc->hex_first = page_row(doc->hex_first, height, last_row >= height ? last_row - height + 1 : 0, down);

    size_t row = doc->cursor / cols;

    if (row < doc->hex_first) { doc->hex_first = row; }
    if (row - doc->hex_first >= height) { doc->hex_first = row - height + 1; }
}
