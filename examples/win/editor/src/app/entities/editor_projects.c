/*
 ** editor_projects.c - 3 functions.
 ** contains_file - Match a file to a root at a directory boundary.
 ** editor_project_open - Activate an existing root or add a folder tab.
 ** editor_project_follow - Resolve a named document to its containing project.
 ** See also: editor_projects.h.
 */
#include "editor_projects.h"
#include "ui/ui_dialog.h"
#include <string.h>

static bool contains_file(const char* root, const char* path) {
    size_t length = strlen(root);
    return length && !_strnicmp(root, path, length) && (root[length - 1] == '\\' || path[length] == '\\');
}

bool editor_project_open(editor_projects* projects, editor_tree* tree, const char* folder) {
    char root[SYS_PATH_CAPACITY];
    size_t length = strlen(folder);
    if (!length || length >= sizeof(root)) { return false; }

    strcpy(root, folder);
    while (length > 3 && root[length - 1] == '\\') { root[--length] = 0; }

    size_t index = 0;
    while (index < projects->count && _stricmp(projects->roots[index], root)) { ++index; }
    if (index == EDITOR_PROJECT_LIMIT) { return false; }
    if (_stricmp(tree->root, root) && !editor_tree_root(tree, root)) { return false; }

    if (index == projects->count) {
        strcpy(projects->roots[projects->count++], root);
        projects->watches[index] = sys_folder_watch_create(root);
        projects->changed[index] = false;
    }
    projects->active = index;
    projects->scroll.reveal_active = true;

    return true;
}

bool editor_project_follow(editor_projects* projects, editor_tree* tree, const char* path) {
    if (!_stricmp(path, projects->followed)) { return true; }

    if (!contains_file(tree->root, path)) {
        size_t index = 0;
        while (index < projects->count && !contains_file(projects->roots[index], path)) { ++index; }
        char folder[SYS_PATH_CAPACITY];
        if (!sys_parent_directory(path, folder)) { return false; }
        if (!editor_project_open(projects, tree, index < projects->count ? projects->roots[index] : folder)) { return false; }
    }

    strcpy(projects->followed, path);
    strcpy(tree->selected, path);
    tree->selected_folder = false;

    return true;
}
