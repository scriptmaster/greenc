/*
 ** workspace_restore.c - 5 functions.
 ** read_snapshot - Validate a complete session before opening tabs.
 ** open_document - Reuse an existing tab or load a new named document.
 ** restore_documents - Recreate saved document tabs and select the active one.
 ** restore_panels - Reinstate folder roots and fresh shell sessions.
 ** editor_workspace_restore - Merge the launch file or working directory into restored tabs.
 ** See also: workspace_session.h, workspace_save.c.
 */
#include "workspace_session.h"
#include "tabs_internal.h"
#include "terminal_internal.h"
#include "settings_tab.h"

static bool read_snapshot(const char* path, workspace_snapshot* saved) {
    FILE* file = path ? fopen(path, "rb") : NULL;
    if (!file) { return false; }

    bool valid = fread(saved, 1, sizeof(*saved), file) == sizeof(*saved) && fgetc(file) == EOF && !ferror(file);
    fclose(file);

    if (!valid || memcmp(saved->version, "GCtabs1", 8) || saved->documents > EDITOR_TAB_LIMIT ||
        saved->projects > EDITOR_PROJECT_LIMIT || saved->terminals > EDITOR_TERMINAL_LIMIT ||
        !memchr(saved->folder, 0, sizeof(saved->folder))) { return false; }

    for (u32 i = 0; i < saved->documents; ++i) {
        if (!memchr(saved->tabs[i].path, 0, SYS_PATH_CAPACITY) || !strchr("FUSJ", saved->tabs[i].type) || !saved->tabs[i].type) { return false; }
    }

    for (u32 i = 0; i < saved->projects; ++i) {
        if (!memchr(saved->roots[i], 0, SYS_PATH_CAPACITY)) { return false; }
    }

    for (u32 i = 0; i < saved->terminals; ++i) {
        if (!memchr(saved->shells[i], 0, SYS_PATH_CAPACITY)) { return false; }
    }

    return true;
}

static bool open_document(editor_view* view, editor_document* doc, editor_settings* settings, const char* path, bool launch) {
    char full[SYS_PATH_CAPACITY];
    if (!_fullpath(full, path, sizeof(full))) { return false; }

    for (size_t i = 0; i < view->tabs.count; ++i) {
        if (!_stricmp(view->tabs.items[i].path, full)) {
            select_tab(&view->tabs, i, doc, settings);

            return true;
        }
    }

    if (view->tabs.count >= EDITOR_TAB_LIMIT) { return false; }

    editor_document loaded = {0};
    if (!(launch ? editor_load(&loaded, full) : editor_load_existing(&loaded, full))) { return false; }

    size_t index = view->tabs.count++;
    strcpy(view->tabs.items[index].path, full);
    select_tab(&view->tabs, index, doc, settings);
    *doc = loaded;

    return true;
}

static void restore_documents(editor_view* view, editor_document* doc, editor_settings* settings, const workspace_snapshot* saved) {
    size_t active = 0;
    for (u32 i = 0; i < saved->documents; ++i) {
        const workspace_tab* tab = &saved->tabs[i];
        bool opened;

        if (tab->type == 'F') {
            opened = open_document(view, doc, settings, tab->path, false);
        } else if (tab->type == 'U') {
            opened = editor_tabs_new(&view->tabs, doc, settings);
        } else {
            opened = editor_settings_tab(view, doc, settings);
            if (opened && tab->type == 'J') { editor_switch_settings(view, doc, settings, view->tabs.active); }
        }

        if (opened && i == saved->document) { active = view->tabs.active; }
    }

    if (view->tabs.count) { select_tab(&view->tabs, active, doc, settings); }
}

static void restore_panels(editor_view* view, const workspace_snapshot* saved, const editor_settings* settings) {
    for (u32 i = 0; i < saved->projects; ++i) {
        editor_project_open(&view->projects, &view->tree, saved->roots[i]);
    }

    if (saved->project < saved->projects) { editor_project_open(&view->projects, &view->tree, saved->roots[saved->project]); }
    view->tree.visible = saved->files_visible && view->projects.count;

    size_t active = 0;
    for (u32 i = 0; i < saved->terminals; ++i) {
        const char* shell = NULL;
        if (!_stricmp(saved->shells[i], settings->shell)) { shell = settings->shell; }
        for (size_t j = 0; !shell && j < 3; ++j) {
            if (!_stricmp(saved->shells[i], profiles[j])) { shell = profiles[j]; }
        }

        if (shell && new_session(&view->terminal, shell) && i == saved->terminal) { active = view->terminal.active; }
    }

    view->terminal.active = active;
    view->terminal.visible = saved->terminal_visible && view->terminal.count;
    view->terminal.focused = false;
}

bool editor_workspace_restore(editor_view* view, editor_document* doc, editor_settings* settings, const char* path) {
    char launch[SYS_PATH_CAPACITY], folder[SYS_PATH_CAPACITY];
    if (!_fullpath(launch, settings->path, sizeof(launch)) ||
        !_fullpath(folder, settings->launch_folder ? settings->launch_folder : ".", sizeof(folder))) { return false; }

    workspace_snapshot saved;
    bool restored = read_snapshot(path, &saved);
    if (restored) {
        restore_documents(view, doc, settings, &saved);
        restore_panels(view, &saved, settings);
    }

    bool new_folder = !restored || _stricmp(folder, saved.folder);
    if (!settings->launch_folder && (settings->explicit_file || !view->tabs.count)) {
        if (!open_document(view, doc, settings, launch, true)) { return false; }
        editor_project_follow(&view->projects, &view->tree, launch);
        view->tree.visible = true;
    } else if (settings->launch_folder || new_folder) {
        if (!editor_project_open(&view->projects, &view->tree, folder)) { return false; }
        view->tree.visible = true;
        size_t length = strlen(folder);
        bool matched = false;
        for (size_t n = 0; n < view->tabs.count; ++n) {
            size_t i = (view->tabs.active + n) % view->tabs.count;
            const char* file = view->tabs.items[i].path;
            if (!_strnicmp(file, folder, length) && (folder[length - 1] == '\\' || file[length] == '\\')) {
                select_tab(&view->tabs, i, doc, settings);
                editor_project_follow(&view->projects, &view->tree, file);
                matched = true;

                break;
            }
        }

        if (settings->launch_folder && !view->tabs.count) {
            if (!editor_tabs_new(&view->tabs, doc, settings)) { return false; }
        } else if (!settings->launch_folder && !matched && !open_document(view, doc, settings, launch, true)) {
            doc->status = "Could not add the launch document";
        }
    }

    if (!view->tabs.items[view->tabs.active].untitled) {
        strcpy(view->projects.followed, settings->path);
        char parent[SYS_PATH_CAPACITY];
        if (sys_parent_directory(settings->path, parent) && !_stricmp(parent, view->tree.root)) {
            strcpy(view->tree.selected, settings->path);
            view->tree.selected_folder = false;
        }
    }

    view->themes.open = view->tabs.items[view->tabs.active].settings_page;

    return true;
}
