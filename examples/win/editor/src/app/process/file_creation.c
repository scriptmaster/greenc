/*
 ** file_creation.c - 3 functions.
 ** refresh_entry - Refresh the active project listing after a filesystem change.
 ** commit_name - Validate and apply a pending creation, copy or rename.
 ** editor_files_name_input - Edit, submit or cancel the tree's inline filename field.
 ** See also: editor_file_actions.h, file_actions_internal.h.
 */
#include "file_actions_internal.h"

void refresh_entry(editor_tree* tree, const char* folder, const char* path, bool directory,
                          editor_document* document) {
    if (!editor_tree_root(tree, tree->root[0] ? tree->root : folder)) {
        document->status = "Change saved; could not refresh Files";

        return;
    }

    strcpy(tree->selected, path);
    tree->selected_folder = directory;
}

static void commit_name(editor_view* view, editor_document* document, editor_settings* settings) {
    editor_tree* tree = &view->tree; ///read
    char path[SYS_PATH_CAPACITY];
    if (!sys_child_path(tree->destination, tree->name, path)) {
        document->status = "Enter a valid filename within the path length limit";

        return;
    }

    bool create_file = tree->naming == NEW_FILE || tree->naming == ROOT_FILE;
    if (create_file && view->tabs.count == EDITOR_TAB_LIMIT) {
        document->status = "Close a tab before creating a file";

        return;
    }

    size_t existing = file_tab(&view->tabs, path);
    if (existing < view->tabs.count && !(tree->naming == RENAME && !_stricmp(tree->selected, path))) {
        document->status = "Destination already open in the editor";

        return;
    }

    size_t source = file_tab(&view->tabs, tree->selected);
    bool folder = tree->naming == NEW_FOLDER;
    bool ok;
    if (tree->naming == RENAME) {
        ok = !strcmp(tree->selected, path) || sys_entry_rename(tree->selected, path);
    } else if (tree->naming == COPY_AS) {
        ok = sys_file_copy_new(tree->selected, path);
    } else {
        ok = folder ? sys_folder_create(path) : sys_file_create_empty(path);
    }

    if (!ok) {
        document->status = "Could not complete change; check the name, destination and permissions";

        return;
    }

    if (tree->naming == RENAME && source < view->tabs.count) {
        strcpy(view->tabs.items[source].path, path);
    }

    tree->naming = 0;
    refresh_entry(tree, tree->destination, path, folder, document);
    if (create_file && !editor_tabs_open(&view->tabs, document, settings, path)) {
        document->status = "File created but could not open it";
    } else if (create_file) {
        tree->focused = false;
    }
}

void editor_files_name_input(editor_view* view, editor_document* document, editor_settings* settings, ui_input input) {
    editor_tree* tree = &view->tree; ///read
    if (input.key == UI_KEY_ESCAPE) {
        tree->naming = 0;

        return;
    }

    if (input.character == '\n') {
        commit_name(view, document, settings);

        return;
    }

    if (input.control && input.key == UI_KEY_A) { tree->replace_name = true; }
    if (input.control || (input.character != '\b' && (input.character < 32 || input.character > 126))) { return; }

    if (tree->replace_name) { tree->name[0] = 0; }
    tree->replace_name = false;
    size_t used = strlen(tree->name);
    if (input.character == '\b') {
        if (used) { tree->name[used - 1] = 0; }
    } else if (used + 1 < sizeof(tree->name)) {
        tree->name[used] = (char)input.character;
        tree->name[used + 1] = 0;
    }
}
