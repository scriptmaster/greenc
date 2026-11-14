/*
 ** hex_input.c - 2 functions.
 ** editor_hex_mouse - Scroll the preview, activate full editing or select a byte.
 ** editor_hex_key - Navigate bytes and overwrite one hex digit at a time.
 ** See also: editor_hex.h, hex_view.c.
 */
#include "editor_hex.h"
#include <ctype.h>
#include "editor_undo.h"

void editor_hex_mouse(editor_view* view, editor_document* doc, ui_input input) {
    u32 cols = view->hex_columns, cw = view->hex_cell_width;
    if (!cols || !cw) { return; }

    bool on_button = !doc->full_hex && editor_contains(view->hex_action, input.x, input.y);
    if (editor_control_activate(&view->hex_button, input, on_button ? 1 : 0)) {
        doc->full_hex = true;
        doc->hex_first = 0;

        return;
    }

    bool down = input.mouse == UI_MOUSE_DOWN;
    if (!down && (!view->dragging || (input.mouse != UI_MOUSE_DRAG && input.mouse != UI_MOUSE_UP))) { return; }
    if (input.mouse == UI_MOUSE_UP) { view->dragging = false; }
    if (input.y < view->hex_top || input.x < 8 || !view->hex_cell_height) { return; }

    u32 row = (input.y - view->hex_top) / view->hex_cell_height;
    u32 cell = (input.x - 8) / cw;
    u32 ascii = 11 + cols * 3;
    if (row >= view->hex_rows || cell < 10) { return; }

    if (cell < ascii && (cell - 10) % 3 == 2) { return; }
    u32 column = cell >= ascii ? cell - ascii : (cell - 10) / 3;
    size_t pos = (doc->hex_first + row) * cols + column;
    if (column < cols && pos < editor_hex_limit(doc)) {
        if (down) {
            if (!input.shift) { doc->anchor = pos; }
            else if (!doc->selecting) { doc->anchor = doc->cursor; }

            view->dragging = true;
            doc->selecting = input.shift;
        } else {
            doc->selecting = true;
        }

        doc->cursor = pos;
        doc->low_nibble = cell < ascii && (cell - 10) % 3 == 1;
    }
}

void editor_hex_key(editor_view* view, editor_document* doc, ui_input input) {
    size_t limit = editor_hex_limit(doc);

    if (!limit || !view->hex_columns) { return; }

    size_t pos = doc->cursor, last = limit - 1;
    if (input.control && input.key == UI_KEY_A) {
        doc->anchor = 0;
        doc->selecting = true;
        pos = last;
        input.key = UI_KEY_END;
    } else if (input.key >= UI_KEY_LEFT && input.key <= UI_KEY_END) {
        if (input.shift && !doc->selecting) { doc->anchor = doc->cursor; }
        doc->selecting = input.shift;
    }

    u32 cols = view->hex_columns;
    switch (input.key) {
        case UI_KEY_LEFT: pos -= pos > 0; break;
        case UI_KEY_RIGHT: pos += pos < last; break;
        case UI_KEY_UP: pos = pos < cols ? 0 : pos - cols; break;
        case UI_KEY_DOWN: pos = last - pos < cols ? last : pos + cols; break;
        case UI_KEY_HOME: pos = input.control ? 0 : pos - pos % cols; break;
        case UI_KEY_END:
            pos = input.control || last - pos < cols - 1 - pos % cols ? last : pos + cols - 1 - pos % cols;
            break;
        default:
            if (!doc->full_hex || input.control || input.character > 127 || !isxdigit((int)input.character)) { return; }
            u8 digit = input.character <= '9' ? (int)input.character - '0' : toupper((int)input.character) - 'A' + 10;
            u8 old = (u8)doc->text[pos];
            if (!editor_record_edit(doc, pos, 1, 1)) { return; }

            doc->selecting = false;
            doc->text[pos] = doc->low_nibble ? (char)((old & 0xf0) | digit) : (char)((old & 15) | digit << 4);
            if (doc->low_nibble && pos < last) { ++pos; }

            doc->low_nibble = !doc->low_nibble;
            doc->dirty = true;
            break;
    }

    if (!input.character) { doc->low_nibble = false; }
    doc->cursor = pos;
    size_t row = pos / cols;
    if (row < doc->hex_first) { doc->hex_first = row; }
    if (view->hex_rows && row >= doc->hex_first + view->hex_rows) { doc->hex_first = row - view->hex_rows + 1; }
}
