/*
 ** pointer_shape.c - 2 functions.
 ** editor_terminal_divider - Test the terminal's resize boundary.
 ** editor_pointer_shape - Choose the native pointer shape for the current interaction.
 ** See also: editor_dispatch.h, main_internal.h.
 */
#include "main_internal.h"

bool editor_terminal_divider(const editor_terminal* panel, i32 top, i32 y) {
    return panel->visible && y >= top && y < top + 6;
}

ui_cursor editor_pointer_shape(ui_window* window, const editor_view* view, const editor_settings* settings) {
    if (view->auto_scroll) { return UI_CURSOR_RESIZE_NS; }
    if (view->themes.open) { return UI_CURSOR_ARROW; }
    if (view->tree.visible && view->tree.resizing) { return UI_CURSOR_RESIZE_EW; }
    if (view->terminal.visible && view->terminal.resizing) { return UI_CURSOR_RESIZE_NS; }

    i32 x, y;
    u32 width, height;
    if (view->dragging || !ui_window_pointer(window, &x, &y) || !ui_window_client_size(window, &width, &height)) {
        return UI_CURSOR_ARROW;
    }

    u32 bottom = height > settings->layout.footer_height ? height - settings->layout.footer_height : 0;
    i32 top = (i32)(bottom - editor_terminal_size(&view->terminal, bottom));
    if (editor_tree_divider(&view->tree, width, top, x, y)) { return UI_CURSOR_RESIZE_EW; }
    if (editor_terminal_divider(&view->terminal, top, y)) { return UI_CURSOR_RESIZE_NS; }

    return UI_CURSOR_ARROW;
}
