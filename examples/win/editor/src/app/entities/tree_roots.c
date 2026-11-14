/*
 ** tree_roots.c - 5 functions.
 ** editor_tree_root - Replace the tree listing only after successful enumeration.
 ** editor_tree_follow - Initialize a tree from a document's containing folder.
 ** editor_tree_init - Initialize the tree from a startup file.
 ** editor_tree_select_at - Select the row under the pointer.
 ** editor_tree_destroy - Release owned directory rows.
 ** See also: tree_roots.h, tree_internal.h.
 */
#include "tree_internal.h"

bool editor_tree_root(editor_tree* tree, const char* folder) {
    editor_tree listing = {0};
    if (!insert_children(&listing, 0, folder, 0)) { return false; }

    free(tree->rows);
    tree->rows = listing.rows;
    tree->count = listing.count;
    tree->scroll = 0;
    tree->selected[0] = 0;
    tree->selected_folder = false;
    if (folder != tree->root) { strcpy(tree->root, folder); }

    return true;
}

bool editor_tree_follow(editor_tree* tree, const char* path) {
    if (!_stricmp(path, tree->followed)) { return true; }

    char folder[SYS_PATH_CAPACITY];
    if (!sys_parent_directory(path, folder)) { return false; }
    if (_stricmp(tree->root, folder) && !editor_tree_root(tree, folder)) { return false; }

    strcpy(tree->followed, path);
    strcpy(tree->selected, path);
    tree->selected_folder = false;

    return true;
}

void editor_tree_init(editor_tree* tree, const char* path) {
    tree->visible = editor_tree_follow(tree, path);
}

void editor_tree_select_at(editor_tree* tree, ui_renderer* renderer, i32 y) {
    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    tree->selected[0] = 0;
    tree->selected_folder = false;
    if (y < (i32)(2 * ch)) { return; }

    size_t index = tree->scroll + (size_t)y / ch - 2;
    if (index < tree->count) {
        strcpy(tree->selected, tree->rows[index].path);
        tree->selected_folder = tree->rows[index].folder;
    }
}

void editor_tree_destroy(editor_tree* tree) {
    free(tree->rows);
}
