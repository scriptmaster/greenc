/*
 ** editor_frame.c - 2 functions.
 ** scroll_from_anchor - Move through rows according to pointer displacement and elapsed time.
 ** editor_draw - Compose and present a complete editor frame.
 ** See also: editor_view.h, view_internal.h.
 */
#include "view_internal.h"
#include "editor_hex.h"
#include "byte_text.h"

static void scroll_from_anchor(editor_view* view, ui_window* window, size_t* first, size_t maximum) {
    if (!view->auto_scroll) { return; }
    i32 x, y;
    if (!ui_window_pointer(window, &x, &y)) { view->auto_scroll = false; return; }
    i32 delta = y - view->scroll_anchor_y;
    u64 now = ui_time_ms();
    if (abs(delta) <= 6) { view->scroll_tick = now; return; }
    u64 interval = 1200u / (u32)abs(delta) + 8;
    size_t steps = (size_t)((now - view->scroll_tick) / interval);
    if (!steps) { return; }
    view->scroll_tick += steps * interval;
    if (*first > maximum) { *first = maximum; }
    if (delta < 0) { *first -= steps > *first ? *first : steps; }
    else { *first += steps > maximum - *first ? maximum - *first : steps; }
}

bool editor_draw(ui_renderer* renderer, ui_window* window, editor_document* document,
                 editor_view* view, const editor_settings* settings) {
    if (!ui_renderer_begin(renderer, settings->theme.background)) {
        return false;
    }

    editor_viewport viewport = {0};
    if (document->binary) {
        if (!ui_window_client_size(window, &viewport.width, &viewport.height)) { return false; }
        viewport.width -= editor_tree_width(&view->tree, viewport.width);
        u32 bottom = viewport.height > settings->layout.footer_height ? viewport.height - settings->layout.footer_height : 0;
        bottom -= editor_terminal_size(&view->terminal, bottom);
        scroll_from_anchor(view, window, &document->hex_first, view->hex_bar.maximum);
        editor_hex_draw(view, document, renderer, window, viewport.width, bottom, settings);
        viewport.cursor_line = document->cursor / view->hex_columns;
        viewport.cursor_column = document->cursor % view->hex_columns;
    } else if (!editor_measure_viewport(renderer, window, document, settings, view, &viewport)) {
        return false;
    }

    editor_tabs_draw(&view->tabs, document, renderer, viewport.width, settings);
    if (viewport.rows && viewport.columns) {
        editor_scrollbar bar = document_scrollbar(&viewport, settings);
        scroll_from_anchor(view, window, &view->first_line, bar.maximum);
        if (view->first_line > bar.maximum) { view->first_line = bar.maximum; }
        editor_scroll_to_cursor(view, &viewport);
        size_t guide = settings->theme.column_guide;
        if (guide && guide >= view->first_column && guide - view->first_column < viewport.columns) {
            i32 x = (i32)(viewport.text_left + (guide - view->first_column) * viewport.cell_width);
            ui_renderer_rect(renderer, x, settings->layout.text_top, 1, viewport.rows * viewport.cell_height, settings->theme.panel);
        }

        editor_draw_document(renderer, document, view, &viewport, settings);
        editor_scrollbar horizontal = document_horizontal_bar(&viewport, view->longest_line, settings);
        if (horizontal.maximum) {
            ui_renderer_rect(renderer, horizontal.top, horizontal.x, horizontal.height, 10, settings->theme.panel);
            ui_renderer_rect(renderer, editor_thumb_top(horizontal, view->first_column), horizontal.x + 2,
                             horizontal.thumb, 6, settings->theme.muted);
        }
        editor_scrollbar_draw(bar, renderer, view->first_line, settings->theme.panel, settings->theme.muted);

        if (!view->terminal.focused) { editor_draw_caret(renderer, view, &viewport, settings); }
        if (document->byte_text) { editor_byte_tooltip(renderer, window, document, view, &viewport, settings); }
    }

    if (view->themes.open) {
        view->themes.top = (i32)settings->layout.header_height;
        view->themes.pointer_x = view->themes.pointer_y = -1;
        ui_window_pointer(window, &view->themes.pointer_x, &view->themes.pointer_y);
        view->themes.bottom = viewport.height > settings->layout.footer_height ? viewport.height - settings->layout.footer_height : 0;
        editor_themes_draw(&view->themes, renderer, viewport.width, view->themes.bottom, &settings->theme);
    }

    if (!ui_window_client_size(window, &viewport.width, &viewport.height)) { return false; }
    u32 bottom = viewport.height > settings->layout.footer_height ? viewport.height - settings->layout.footer_height : 0;
    editor_terminal_draw(&view->terminal, renderer, viewport.width, bottom, settings);
    editor_tree_draw(&view->tree, renderer, viewport.width, bottom - editor_terminal_size(&view->terminal, bottom), settings);
    editor_project_draw(&view->projects, &view->tree, renderer, viewport.width, settings);
    if (viewport.height >= settings->layout.footer_height) {
        editor_status_draw(&view->status_bar, renderer, window, document, settings, viewport.width, viewport.height,
                           viewport.map ? (size_t)viewport.map[viewport.cursor_line].logical + 1 : viewport.cursor_line + 1, viewport.cursor_column + 1);
    } else {
        view->status_bar.bottom = view->status_bar.top;
    }

    return ui_renderer_present(renderer);
}
