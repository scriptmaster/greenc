/*
 ** event_dispatch.c - 1 function.
 ** editor_process_input - Drain window events and dispatch editor commands.
 ** See also: editor_dispatch.h, main_internal.h.
 */
#include "main_internal.h"
#include "editor_hex.h"
#include "sys/fs/write_atomic.h"
#include "page_navigation.h"
#include "editor_undo.h"
#include "editor_debug.h"
#include "settings_tab.h"
#include "terminal_internal.h"

bool editor_process_input(ui_renderer* renderer, ui_window* window, editor_document* document, editor_view* view,
                                  editor_settings* settings) {
    ui_input input;
    while (ui_window_next_input(window, &input)) {
        view->themes.open = view->tabs.items[view->tabs.active].settings_page;
        if (input.key || input.menu_command) { view->auto_scroll = false; }
        follow_active_file(view, document, settings);
        if (view->tree.naming && !input.close_requested) {
            if (!input.menu_command && !input.mouse) {
                editor_files_name_input(view, document, settings, input);

                continue;
            }

            if (input.mouse == UI_MOUSE_UP || input.mouse == UI_MOUSE_DRAG) { continue; }
            view->tree.naming = 0;
        }

        if (input.menu_command == MENU_SETTINGS) {
            editor_settings_tab(view, document, settings);
            view->themes.expanded = true;
            view->tree.resizing = false;
            view->dragging = false;
            view->terminal.resizing = false;

            continue;
        }


        if (input.menu_command) {
            view->tree.focused = false;
            if (input.menu_command == MENU_SAVE || input.menu_command >= MENU_NEW) { view->themes.open = false; }
            view->terminal.focused = false;
            input = editor_menu_input(input.menu_command);
        }

        if (!input.mouse && !view->themes.open && !view->terminal.focused) { view->manual_scroll = false; }
        if (!input.close_requested) {
            view->caret_visible_since_ms = ui_time_ms();
        }

        if (input.close_requested || (input.control && input.key == UI_KEY_Q)) {
            if (view->themes.dirty && !ui_confirm("Modified theme", "Quit without saving the modified theme?")) {
                continue;
            }

            if (!editor_tabs_dirty(&view->tabs, document) || ui_confirm("Unsaved document", "Discard unsaved changes and quit?")) {
                ui_window_request_close(window);

                return true;
            }
        } else if (view->themes.open && input.mouse && !input.menu_command) {
            u32 width, height;
            if (!ui_window_client_size(window, &width, &height)) { return false; }
            i32 panel = (i32)(width - editor_tree_width(&view->tree, width));
            i32 bottom = (i32)height - (i32)settings->layout.footer_height;
            i32 end = bottom - (i32)editor_terminal_size(&view->terminal, bottom);
            if (input.y < (i32)settings->layout.header_height || input.x >= panel || input.y >= end) {
                editor_mouse_input(renderer, window, document, view, settings, input);
            } else {
                view->tree.focused = view->terminal.focused = false;
                editor_themes_input(&view->themes, window, input, &settings->theme);
            }
        } else if (input.key == UI_KEY_F5 || input.key == UI_KEY_F10 || input.key == UI_KEY_F11) {
            editor_debug_key(view, document, settings, window, input);
        } else if (input.key == UI_KEY_F6) {
            if (!cycle_frame_rate(window, settings)) { return false; }
        } else if (input.key == UI_KEY_F2 && view->tree.visible && view->tree.focused) {
            editor_files_rename(&view->tree);
        } else if (input.control && input.key == UI_KEY_GRAVE) {
            view->tree.focused = false;
            if (!editor_terminal_toggle(&view->terminal, settings)) { document->status = "Cannot start configured shell"; }
        } else if (input.control && input.key == UI_KEY_B) {
            view->tree.visible = !view->tree.visible;
        } else if (input.mouse) {
            if (!editor_mouse_input(renderer, window, document, view, settings, input)) { return false; }
        } else if (input.control && !input.shift && (input.key == UI_KEY_T || input.key == UI_KEY_W)) {
            bool add = input.key == UI_KEY_T;
            if (view->tree.visible && view->tree.focused) {
                if (!editor_project_action(&view->projects, &view->tree, window, add ? TAB_ADD : TAB_CLOSE, view->projects.active)) {
                    document->status = "Cannot change folder tab";
                }
            } else if (view->terminal.visible && view->terminal.focused) {
                if (add) { new_session(&view->terminal, settings->shell); }
                else if (view->terminal.count) { close_session(&view->terminal, view->terminal.active); }
            } else if (!add) {
                editor_tabs_close(&view->tabs, view->tabs.active, document, settings);
            } else if (!editor_tabs_new(&view->tabs, document, settings)) {
                document->status = "File tab limit reached";
            }
        } else if (view->terminal.focused) {
            editor_terminal_key(&view->terminal, input);
        } else if (view->themes.open && !(input.control && (input.key == UI_KEY_N || input.key == UI_KEY_O))) {
            continue;
        } else if (input.control && ((input.shift && input.key == UI_KEY_Z) || (!input.shift && input.key == UI_KEY_Y))) {
            editor_redo(document);
        } else if (input.control && !input.shift && input.key == UI_KEY_Z) {
            editor_undo(document);
        } else if (input.control && (input.key == UI_KEY_C || input.key == UI_KEY_X || input.key == UI_KEY_V)) {
            if (!document->binary) { editor_clipboard(document, window, input.key); }
            else if (input.key == UI_KEY_C) { editor_hex_copy(document, window); }
        } else if (input.control && input.key == UI_KEY_S) {
            if (view->tabs.items[view->tabs.active].settings_json && !input.shift) {
                editor_apply_settings_json(view, document, settings);
            } else if (view->tabs.items[view->tabs.active].settings_json) {
                char path[260] = "settings.json";

                if (ui_choose_file(window, true, "json", path)) {
                    if (!sys_file_write_atomic(path, document->text, document->length)) { document->open_error = "JSON export failed"; }
                }
            } else {
                editor_tabs_save(&view->tabs, document, settings, window, input.shift);
            }
        } else if (input.control && input.key == UI_KEY_N) {
            if (!editor_tabs_new(&view->tabs, document, settings)) { document->status = "File tab limit reached"; }
        } else if (input.control && input.key == UI_KEY_O) {
            char path[260] = {0};
            if (ui_choose_file(window, false, "txt", path)) { editor_tabs_open(&view->tabs, document, settings, path); }
        } else if (view->themes.open) {
            continue;
        } else if (input.key == UI_KEY_PAGE_UP || input.key == UI_KEY_PAGE_DOWN) {
            if (view->tree.focused) { continue; }

            if (document->binary) {
                editor_hex_page(view, document, input);
            } else {
                editor_viewport viewport = {0};

                if (editor_measure_viewport(renderer, window, document, settings, view, &viewport)) {
                    editor_text_page(view, document, &viewport, input);
                }
            }
        } else if (document->binary) {
            editor_hex_key(view, document, input);
        } else {
            editor_input(document, input);
        }
    }

    view->themes.open = view->tabs.items[view->tabs.active].settings_page;
    follow_active_file(view, document, settings);

    return true;
}
