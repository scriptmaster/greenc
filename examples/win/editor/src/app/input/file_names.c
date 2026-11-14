/*
 ** file_names.c - 2 functions.
 ** begin_name - Prepare an inline filename entry for a tree command.
 ** editor_files_rename - Start renaming the selected file.
 ** See also: editor_file_actions.h, file_actions_internal.h.
 */
#include "file_actions_internal.h"

void begin_name(editor_tree* tree, u16 command) {
    if (!target_folder(tree, tree->destination)) { return; }
    if (command == ROOT_FILE) { strcpy(tree->destination, tree->root); }

    tree->naming = command;
    tree->replace_name = true;
    tree->name[0] = 0;
    if (command == RENAME || command == COPY_AS) {
        const char* leaf = strrchr(tree->selected, '\\');
        strcpy(tree->name, leaf ? leaf + 1 : tree->selected);
    }
}

void editor_files_rename(editor_tree* tree) {
    if (tree->selected[0] && !tree->selected_folder) { begin_name(tree, RENAME); }
}
