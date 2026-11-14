/*
 ** byte_text.c - 4 functions.
 ** editor_open_byte_text - Keep original bytes, offsets and unsaved edits when changing views.
 ** editor_byte_needs_marker - Identify a byte displayed as a middle dot.
 ** editor_draw_byte_marks - Draw markers independently of the system code page.
 ** editor_byte_tooltip - Resolve a hovered text cell to its current document byte.
 ** See also: byte_text.h, syntax.c.
 */
#include "byte_text.h"

void editor_open_byte_text(editor_document* doc) {
    doc->binary = false;
    doc->byte_text = true;
    doc->selecting = doc->low_nibble = false;
    doc->anchor = doc->cursor;
    editor_document_changed(doc);
}

bool editor_byte_needs_marker(u8 byte) {
    return byte > 126 || (byte < 32 && byte != '\t' && byte != '\r' && byte != '\n');
}

void editor_draw_byte_marks(ui_renderer* renderer, const char* text, size_t count,
                           const editor_viewport* viewport, i32 y, u32 color) {
    for (size_t i = 0; i < count; ++i) {
        if (editor_byte_needs_marker((u8)text[i])) {
            i32 x = (i32)(viewport->text_left + i * viewport->cell_width + viewport->cell_width / 2);
            ui_renderer_rect(renderer, x - 1, y + (i32)viewport->cell_height / 2 - 1, 2, 2, color);
        }
    }
}

void editor_byte_tooltip(ui_renderer* renderer, ui_window* window, const editor_document* doc,
                         const editor_view* view, const editor_viewport* viewport, const editor_settings* settings) {
    i32 x, y;
    i32 top = (i32)settings->layout.text_top;

    if (!ui_window_pointer(window, &x, &y) || x < (i32)viewport->text_left || y < top) { return; }

    size_t row = (size_t)(y - top) / viewport->cell_height;
    size_t column = (size_t)(x - (i32)viewport->text_left) / viewport->cell_width;

    if (row >= viewport->rows || column >= viewport->columns || row + view->first_line >= viewport->visible_lines) { return; }

    editor_line_entry line = viewport->map[row + view->first_line];
    column += view->first_column;
    if (column >= line.end - line.offset) { return; }

    size_t offset = line.offset + column;
    u8 byte = (u8)doc->text[offset];

    if (!editor_byte_needs_marker(byte)) { return; }

    char label[80];
    snprintf(label, sizeof(label), "Offset %zu   Hex %02X   Decimal %u", offset, (unsigned)byte, (unsigned)byte);
    i32 width = (i32)(strlen(label) * viewport->cell_width + 12);
    i32 height = (i32)viewport->cell_height + 8;
    i32 bottom = top + (i32)(viewport->rows * viewport->cell_height);
    x += 10;
    y = y + height + 12 <= bottom ? y + 12 : y - height;

    if (x + width > (i32)viewport->width) { x = (i32)viewport->width - width; }
    if (x < 0) { x = 0; }
    if (y < top) { y = top; }

    ui_renderer_rect(renderer, x, y, width, height, settings->theme.panel);
    ui_renderer_text(renderer, x + 6, y + 4, label, settings->theme.text);
}
