/*
 ** theme_input.c - 1 function.
 ** editor_themes_input - Apply settings navigation, edits and theme file commands.
 ** See also: editor_themes.h, themes_internal.h.
 */
#include "themes_internal.h"
#include "debug_preferences.h"
#include "explorer_menu.h"

void editor_themes_input(editor_themes* page, ui_window* window, ui_input input, editor_theme* theme) {
    if (input.key == UI_KEY_ESCAPE) {
        page->open = false;
        page->controls.pressed = 0;

        return;
    }

    if (!input.mouse) { return; }

    u32 width, height;
    if (!ui_window_client_size(window, &width, &height)) { return; }
    if (page->bottom && height > page->bottom) { height = page->bottom; }
    input.y -= page->top;
    height = height > (u32)page->top ? height - (u32)page->top : 0;
    if (input.x < 0 || input.y < 0 || input.x >= (i32)width || input.y >= (i32)height) {
        page->controls.pressed = 0;
        return;
    }
    i32 rows = visible_rows(height);
    scroll_fields(page, rows, 0);
    if (input.mouse == UI_MOUSE_WHEEL) {
        i32 step = input.wheel_delta < 0 ? 3 : -3;

        if (page->keybindings && !page->debug) {
            i32 visible = height > 10 ? (i32)(height - 10) / KEYBINDING_HEIGHT : 1;
            page->first_keybinding += step;
            if (page->first_keybinding > KEYBINDING_ROWS - visible) { page->first_keybinding = KEYBINDING_ROWS - visible; }
            if (page->first_keybinding < 0) { page->first_keybinding = 0; }
        } else {
            scroll_fields(page, rows, step);
        }

        page->controls.pressed = 0;

        return;
    }

    i32 target = 0;
    for (i32 id = CATEGORY; id < FIRST_FIELD + 2 * FIELD_COUNT; ++id) {
        if (control_visible(page, id, rows) && editor_contains(control_bounds(id, page->first), input.x, input.y)) {
            target = id;

            break;
        }
    }

    if (input.x >= 8 && input.x < 20 && input.y >= 38 && input.y < 66) { target = CATEGORY; }
    if (!editor_control_activate(&page->controls, input, target)) { return; }
    if (target >= FIRST_FIELD) {
        i32 field = (target - FIRST_FIELD) / 2;
        u32* value = theme_value(theme, field);
        if (fields[field].maximum) {
            i32 delta = (target - FIRST_FIELD) % 2 ? 1 : -1;
            i32 next = (i32)*value + delta;
            if (next < 0 || next > (i32)fields[field].maximum) { return; }

            *value = (u32)next;
        } else if (!ui_choose_color(window, value)) {
            return;
        }

        char preference[260];
        page->dirty = !editor_theme_preference(preference) || !editor_theme_save(theme, preference);
        page->status = page->dirty ? "Applied; could not save preference" : "Applied";

        return;
    }

    switch (target) {
        case INSTALL_ADMIN:
        case REMOVE_ADMIN:
            page->explorer_status = explorer_request_admin(target == INSTALL_ADMIN) ?
                "Windows 11 and classic menus updated" : "Administrator action cancelled or failed";

            return;
        case EXPLORER:
            if (!explorer_menu_set(!page->explorer_enabled)) {
                page->explorer_status = "Could not update the per-user classic menu.";
            }

            else { page->explorer_status = "Explorer registration updated"; }

            page->explorer_enabled = explorer_menu_enabled();

            return;
        case CATEGORY:
            page->expanded = !page->expanded;
            page->changed = ui_time_ms();

            return;
        case GENERAL:
            page->general = true;
            page->debug = page->keybindings = false;
            page->explorer_enabled = explorer_menu_enabled();

            return;
        case THEMES:
            page->general = false;
            page->debug = page->keybindings = false;

            return;
        case KEYS:
            page->general = false;
            page->debug = false;
            page->keybindings = true;

            return;
        case GROUP:
            page->group = (page->group + 1) % 6;
            page->first = group_edges[page->group];

            return;
        case DEBUGGER: page->debug = true; return;
        case DONE: page->open = false; return;
        case PREVIOUS: scroll_fields(page, rows, -rows); return;
        case NEXT: scroll_fields(page, rows, rows); return;
    }

    if (page->debug && target == BROWSE) {
        if (ui_choose_file(window, false, "exe", page->debug_path)) {
            page->debug_save_failed = !editor_debug_save(page->debug_path);
        }
        return;
    }

    bool save = target == SAVE;
    char path[260] = {0};
    if (save) { strcpy(path, page->path); }
    if (!ui_choose_file(window, save, "theme", path)) { return; }
    if (!save && page->dirty && !ui_confirm("Modified theme", "Replace the current unsaved colors?")) { return; }

    if (!(save ? editor_theme_save(theme, path) : editor_theme_load(theme, path))) {
        page->status = save ? "Save failed" : "Invalid theme file";
        return;
    }

    char preference[260];
    if (!save && editor_theme_preference(preference)) { editor_theme_save(theme, preference); }

    strcpy(page->path, path);
    page->dirty = false;
    page->status = save ? "Theme saved" : "Theme loaded";
}
