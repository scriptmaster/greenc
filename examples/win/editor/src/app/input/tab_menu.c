/*
 ** tab_menu.c - 1 function.
 ** editor_tab_menu - Hit-test a tab and dispatch its native context menu.
 ** See also: tab_menu.h, tab_export.h, terminal_internal.h.
 */
#include "tab_menu.h"
#include "settings_tab.h"
#include "tab_export.h"
#include "terminal_internal.h"
#include "ui/ui_dialog.h"

bool editor_tab_menu(editor_view* view, editor_document* doc, editor_settings* settings, ui_renderer* renderer,
                     ui_window* window, ui_input input, u32 width, i32 terminal_top) {
    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    u32 files_width = editor_tree_width(&view->tree, width);
    i32 files_left = (i32)(width - files_width);
    bool files = view->tree.visible && input.x >= files_left && input.y >= 0 && input.y < (i32)(2 * ch);
    bool terminal = view->terminal.visible && input.y >= terminal_top + DIVIDER_HEIGHT &&
                    input.y < terminal_top + DIVIDER_HEIGHT + (i32)settings->layout.header_height;

    if (!files && !terminal && (input.x >= files_left || input.y < 0 || input.y >= (i32)settings->layout.header_height)) {
        return false;
    }

    editor_tab_scroll scroll = files ? view->projects.scroll : terminal ? view->terminal.scroll : view->tabs.scroll;
    size_t active = files ? view->projects.active : terminal ? view->terminal.active : view->tabs.active;
    size_t count = files ? view->projects.count : terminal ? view->terminal.count : view->tabs.count;
    u32 space = files ? files_width : terminal ? width : (u32)files_left;
    editor_tab_strip strip = editor_strip_layout(&scroll, count, active, space, cw, terminal);
    size_t index = 0;
    editor_tab_action hit = editor_strip_click(&strip, &scroll, input.x - (files ? files_left : 0), &index);

    if (hit != TAB_SELECT && hit != TAB_CLOSE) { return false; }

    bool settings_tab = !files && !terminal && (view->tabs.items[index].settings_page || view->tabs.items[index].settings_json);
    const ui_popup_item items[] = {{"Close tab", 1}, {"New tab", 2}, {"Save tab...", 3}, {"Save all tabs...", 4},
                                   {settings_tab && view->tabs.items[index].settings_json ? "Open as settings UI" : "Open as JSON file", 5}};
    u16 command = ui_popup_menu(window, input.x, input.y, items, settings_tab ? 5 : 4);
    bool ok = true;

    view->tree.focused = files;
    view->terminal.focused = terminal;

    if (command == 5) {
        editor_switch_settings(view, doc, settings, index);
    } else if (command == 3 || command == 4) {
        if (files) { ok = editor_save_folders(&view->projects, window, index, command == 4); }
        else if (terminal) { ok = editor_save_consoles(&view->terminal, window, index, command == 4); }
        else { editor_save_documents(view, doc, settings, window, index, command == 4); }
    } else if (command == 1 || command == 2) {
        if (files) { ok = editor_project_action(&view->projects, &view->tree, window, command == 1 ? TAB_CLOSE : TAB_ADD, index); }
        else if (terminal && command == 1) { close_session(&view->terminal, index); }
        else if (terminal) { ok = new_session(&view->terminal, settings->shell); }
        else if (command == 1) { editor_tabs_close(&view->tabs, index, doc, settings); }
        else { ok = editor_tabs_new(&view->tabs, doc, settings); }
    }

    if (!ok) { doc->status = "Tab operation failed"; }

    return true;
}
