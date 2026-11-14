/*
 ** mouse_dispatch.c - 1 function.
 ** editor_mouse_input - Route pointer events to the appropriate pane or control.
 ** See also: editor_dispatch.h, main_internal.h.
 */
#include "main_internal.h"
#include "editor_hex.h"
#include "settings_tab.h"
#include "tab_menu.h"
#include "byte_text.h"
#include "ui/ui_dialog.h"

bool editor_mouse_input(ui_renderer* renderer, ui_window* window, editor_document* document,
                               editor_view* view, editor_settings* settings, ui_input input) {
    u32 width, height;
    if (!ui_window_client_size(window, &width, &height)) { return false; }
    u32 bottom = height > settings->layout.footer_height ? height - settings->layout.footer_height : 0;
    i32 top = (i32)(bottom - editor_terminal_size(&view->terminal, bottom));
    bool on_fps = input.y >= (i32)bottom && editor_status_hit(&view->status_bar, input.x, input.y);
    if (input.mouse == UI_MOUSE_DOWN) { view->status_bar.pressed = on_fps; }
    if (view->status_bar.pressed) {
        if (input.mouse == UI_MOUSE_UP) {
            view->status_bar.pressed = false;
            if (on_fps && !cycle_frame_rate(window, settings)) { return false; }
        }

        return true;
    }

    bool over_tree = view->tree.visible && input.y >= 0 && input.y < top &&
                     input.x >= (i32)(width - editor_tree_width(&view->tree, width));
    if (document->binary && view->dragging && (input.mouse == UI_MOUSE_DRAG || input.mouse == UI_MOUSE_UP)) {
        editor_hex_mouse(view, document, input);

        return true;
    }

    if (input.mouse == UI_MOUSE_MIDDLE) {
        view->auto_scroll = !view->auto_scroll && !over_tree && input.y >= (i32)settings->layout.text_top && input.y < top;
        view->scroll_anchor_y = input.y;
        view->scroll_tick = ui_time_ms();
        if (view->auto_scroll) { view->manual_scroll = true; }

        return true;
    }

    if (input.mouse == UI_MOUSE_DOWN || input.mouse == UI_MOUSE_CONTEXT) { view->auto_scroll = false; }
    if (input.mouse == UI_MOUSE_CONTEXT) {
        if (editor_tab_menu(view, document, settings, renderer, window, input, width, top)) { return true; }

        if (over_tree) {
            view->tree.focused = true;
            view->terminal.focused = false;
            editor_files_context(view, document, settings, renderer, window, input);
        } else if (document->binary && input.y >= (i32)settings->layout.header_height && input.y < top) {
            const ui_popup_item action = {"Open in text editor", 1};

            if (ui_popup_menu(window, input.x, input.y, &action, 1)) {
                editor_open_byte_text(document);
                view->terminal.focused = view->tree.focused = false;
                view->manual_scroll = view->dragging = false;
            }
        }

        return true;
    }

    if (input.mouse == UI_MOUSE_DOWN) { view->tree.focused = over_tree; }
    if (!view->dragging && !view->tree.resizing && !view->terminal.resizing) {
        bool scrolled = document->binary ?
            editor_scroll_rows(view, view->hex_bar, width - editor_tree_width(&view->tree, width), &document->hex_first, input) :
            editor_scroll_input(view, document, renderer, window, settings, input);
        if (scrolled) { return true; }
    }


    if (!view->dragging && !view->terminal.resizing && editor_tree_resize(&view->tree, width, top, input)) {
        return true;
    }

    if (input.mouse == UI_MOUSE_DOWN && editor_terminal_divider(&view->terminal, top, input.y)) {
        view->terminal.resizing = true;
    }

    if (view->terminal.resizing) {
        view->terminal.height = input.y + 48 < (i32)bottom ? (u32)((i32)bottom - input.y) : 48;
        if (input.mouse == UI_MOUSE_UP) { view->terminal.resizing = false; }
        return true;
    }

    if (over_tree) {
        view->terminal.focused = false;
        u32 cw, ch;
        ui_renderer_cell_size(renderer, &cw, &ch);
        if (input.mouse == UI_MOUSE_DOWN && input.y < (i32)(2 * ch)) {
            if (!editor_project_click(&view->projects, &view->tree, renderer, window, width, input.x)) {
                document->status = "Cannot open project folder";
            }

            return true;
        }

        view->tree.current_path[0] = 0;
        editor_tree_input(&view->tree, renderer, input, document);
        if (view->tree.current_path[0]) { editor_tabs_open(&view->tabs, document, settings, view->tree.current_path); }
        view->terminal.focused = false;
        return true;
    }

    if (input.mouse == UI_MOUSE_DOWN && input.y >= 0 && input.y < (i32)settings->layout.header_height) {
        editor_tabs_click(&view->tabs, document, settings, renderer, width - editor_tree_width(&view->tree, width), input.x);
        view->terminal.focused = false;
        return true;
    }

    if (input.mouse == UI_MOUSE_DOWN) { view->terminal.focused = view->terminal.visible && input.y >= top; }
    if (view->terminal.focused && input.mouse == UI_MOUSE_DOWN && input.y < (i32)bottom) {
        editor_terminal_click(&view->terminal, renderer, width, input.x, input.y - top, settings);
    }

    if (document->binary && input.y >= (i32)settings->layout.header_height && input.y < top) {
        editor_hex_mouse(view, document, input);

        return true;
    }

    if (document->binary || view->terminal.focused || input.mouse == UI_MOUSE_WHEEL) { return true; }
    editor_mouse_select(view, document, renderer, window, settings, input);
    if (view->themes.open && !view->tabs.items[view->tabs.active].settings_page) {
        editor_settings_tab(view, document, settings);
    }

    return true;
}
