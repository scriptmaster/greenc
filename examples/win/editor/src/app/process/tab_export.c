/*
 ** tab_export.c - 3 functions.
 ** editor_save_documents - Save document buffers and restore the originally active tab.
 ** editor_save_folders - Save folder paths as a text list.
 ** editor_save_consoles - Save the visible terminal screen for each chosen session.
 ** See also: tab_export.h, tab_menu.c.
 */
#include "tab_export.h"
#include "settings_tab.h"
#include "tabs_internal.h"
#include "ui/ui_dialog.h"
#include "sys/fs/write_atomic.h"

void editor_save_documents(editor_view* view, editor_document* doc, editor_settings* settings,
                           ui_window* window, size_t index, bool all) {
    size_t active = view->tabs.active;

    for (size_t i = all ? 0 : index; i < (all ? view->tabs.count : index + 1); ++i) {
        editor_tab* tab = &view->tabs.items[i];

        if (tab->settings_page) {
            if (!view->themes.dirty && all) { continue; }
            char path[260];
            strcpy(path, view->themes.path);

            if (!path[0] && !ui_choose_file(window, true, "theme", path)) { break; }
            if (!editor_theme_save(&settings->theme, path)) { doc->status = "Theme save failed"; break; }

            strcpy(view->themes.path, path);
            view->themes.dirty = false;
            continue;
        }

        if (all && !(i == view->tabs.active ? doc->dirty : tab->saved_state.dirty)) { continue; }
        select_tab(&view->tabs, i, doc, settings);
        if (tab->settings_json) { editor_apply_settings_json(view, doc, settings); }
        else if (!all || doc->dirty) { editor_tabs_save(&view->tabs, doc, settings, window, false); }
        if (doc->dirty) { break; }
    }

    select_tab(&view->tabs, active, doc, settings);
}

bool editor_save_folders(const editor_projects* projects, ui_window* window, size_t index, bool all) {
    char path[260] = {0}, roots[EDITOR_PROJECT_LIMIT * (SYS_PATH_CAPACITY + 2)];
    size_t used = 0;

    if (!ui_choose_file(window, true, "txt", path)) { return true; }

    for (size_t i = all ? 0 : index; i < (all ? projects->count : index + 1); ++i) {
        size_t length = strlen(projects->roots[i]);
        memcpy(roots + used, projects->roots[i], length);
        used += length;
        roots[used++] = '\n';
    }

    return sys_file_write_atomic(path, roots, used);
}

bool editor_save_consoles(editor_terminal* panel, ui_window* window, size_t index, bool all) {
    char path[260] = {0};

    if (!ui_choose_file(window, true, "txt", path)) { return true; }

    char* text = calloc(panel->count, 262144);
    size_t used = 0;

    if (!text) { return false; }

    bool valid = true;
    for (size_t i = all ? 0 : index; valid && i < (all ? panel->count : index + 1); ++i) {
        editor_session* session = &panel->sessions[i];
        sys_console_view screen;
        u32 columns = session->columns ? session->columns : 80;
        u32 rows = session->rows ? session->rows : 25;

        if (!sys_console_viewport(session->console, columns, rows, &screen)) { valid = false; break; }

        for (u32 y = 0; y < rows; ++y) {
            char line[1025];
            if (!sys_console_row(session->console, screen.left, screen.top + y, line, columns)) { valid = false; break; }

            size_t length = strlen(line);
            if (used + length + 1 >= panel->count * 262144) { valid = false; break; }
            memcpy(text + used, line, length);
            used += length;
            text[used++] = '\n';
        }
    }

    if (valid) { valid = sys_file_write_atomic(path, text, used); }
    free(text);

    return valid;
}
