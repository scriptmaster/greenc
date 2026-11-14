/*
 ** hex_view.c - 3 functions.
 ** draw_hex_row - Paint offsets, hex pairs and their printable representation.
 ** draw_preview_action - Paint the preview notice and themed full-editor button.
 ** editor_hex_draw - Lay out and render only visible byte rows.
 ** See also: editor_hex.h, hex_input.c.
 */
#include "editor_hex.h"
#include <stdio.h>

static void draw_hex_row(ui_renderer* renderer, const editor_document* doc, size_t start, size_t limit,
                         const editor_view* view, i32 y, const editor_theme* theme) {
    u32 cw = view->hex_cell_width, ch = view->hex_cell_height;
    char offset[24];
    snprintf(offset, sizeof(offset), "%08zX", start);
    ui_renderer_text(renderer, 8, y, offset, theme->muted);
    i32 ascii_x = 8 + (11 + view->hex_columns * 3) * cw;
    size_t first, last;
    editor_selection(doc, &first, &last);

    for (u32 i = 0; i < view->hex_columns && start + i < limit; ++i) {
        size_t pos = start + i;
        u8 byte = (u8)doc->text[pos];
        char pair[3], glyph[2] = {(char)byte, 0};
        snprintf(pair, sizeof(pair), "%02X", byte);
        i32 x = 8 + (10 + i * 3) * cw;
        if (pos >= first && pos <= last) {
            ui_renderer_rect(renderer, ascii_x + i * cw, y, cw, ch, theme->selection);
            ui_renderer_rect(renderer, x, y, 2 * cw, ch, theme->selection);
        }

        if (pos == doc->cursor) {
            ui_renderer_rect(renderer, x + (doc->low_nibble ? cw : 0), y + ch - 2, cw, 2, theme->caret);
        }

        ui_renderer_text(renderer, x, y, pair, theme->text);
        if (byte >= 32 && byte <= 126) {
            ui_renderer_text(renderer, ascii_x + i * cw, y, glyph, theme->text);
        } else {
            ui_renderer_rect(renderer, ascii_x + i * cw + cw / 2, y + ch / 2, 2, 2, theme->muted);
        }
    }
}

static void draw_preview_action(editor_view* view, ui_renderer* renderer, ui_window* window, i32 top,
                                const editor_settings* settings) {
    ui_renderer_text(renderer, 8, top, "Binary preview: first 256 bytes", settings->theme.muted);
    editor_bounds button = {8, top + (i32)view->hex_cell_height + 4, 148, 30};
    view->hex_action = button;
    i32 x, y;
    bool hover = ui_window_pointer(window, &x, &y) && editor_contains(button, x, y);
    editor_control_draw(renderer, button, "Open Hex Editor", EDITOR_PRIMARY, &settings->theme,
                        hover, view->hex_button.pressed != 0, &view->hex_transition);
}

void editor_hex_draw(editor_view* view, editor_document* doc, ui_renderer* renderer, ui_window* window,
                     u32 width, u32 bottom, const editor_settings* settings) {
    ui_renderer_cell_size(renderer, &view->hex_cell_width, &view->hex_cell_height);
    u32 cells = width > 32 ? (width - 32) / view->hex_cell_width : 0;
    u32 cols = cells > 12 ? (cells - 12) / 4 : 1;
    view->hex_columns = cols >= 16 ? 16 : cols >= 8 ? 8 : cols >= 4 ? 4 : 1;
    view->hex_top = settings->layout.text_top;
    view->hex_action = (editor_bounds){0};
    if (!doc->full_hex && doc->length > 256) {
        draw_preview_action(view, renderer, window, view->hex_top, settings);
        view->hex_top += view->hex_cell_height + 44;
    }

    size_t limit = editor_hex_limit(doc);
    size_t total = limit / view->hex_columns + (limit % view->hex_columns != 0);
    view->hex_rows = (i32)bottom > view->hex_top ? (bottom - view->hex_top) / view->hex_cell_height : 0;
    view->hex_bar = editor_scrollbar_layout((i32)width, view->hex_top, view->hex_rows * view->hex_cell_height,
                                            view->hex_rows, total);
    if (doc->hex_first > view->hex_bar.maximum) { doc->hex_first = view->hex_bar.maximum; }
    editor_scrollbar_draw(view->hex_bar, renderer, doc->hex_first, settings->theme.panel, settings->theme.muted);

    for (u32 row = 0; row < view->hex_rows && doc->hex_first + row < total; ++row) {
        draw_hex_row(renderer, doc, (doc->hex_first + row) * view->hex_columns, limit,
                     view, view->hex_top + row * view->hex_cell_height, &settings->theme);
    }
}
