/*
 ** file_targets.c - 2 functions.
 ** target_folder - Resolve the directory for the current tree action.
 ** file_tab - Locate the document tab associated with a filesystem path.
 ** See also: file_actions_internal.h.
 */
#include "file_actions_internal.h"

bool target_folder(const editor_tree* tree, char folder[SYS_PATH_CAPACITY]) {
    if (tree->selected[0] && !tree->selected_folder) { return sys_parent_directory(tree->selected, folder); }

    strcpy(folder, tree->selected_folder ? tree->selected : tree->root);

    return folder[0] != 0;
}

size_t file_tab(const editor_tabs* tabs, const char* path) {
    for (size_t i = 0; i < tabs->count; ++i) {
        if (!tabs->items[i].untitled && !_stricmp(path, tabs->items[i].path)) { return i; }
    }

    return tabs->count;
}
