/*
 ** project_strip.c - 3 functions.
 ** editor_project_action - Apply a folder-tab command from the keyboard or tab strip.
 ** editor_project_draw - Paint the active folder strip in the Files panel.
 ** editor_project_click - Dispatch folder selection, browsing, scrolling and closure.
 ** See also: editor_projects.h.
 */
#include "editor_projects.h"
#include "ui/ui_dialog.h"
#include <string.h>

void editor_project_draw(editor_projects* projects, editor_tree* tree, ui_renderer* renderer, u32 width, const editor_settings* settings) {
    if (!tree->visible || tree->naming) { return; }

    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    u32 panel = editor_tree_width(tree, width);
    editor_tab_strip strip = editor_strip_layout(&projects->scroll, projects->count, projects->active, panel, cw, false);
    editor_tab_label labels[EDITOR_PROJECT_LIMIT];
    for (size_t i = 0; i < projects->count; ++i) {
        const char* name = strrchr(projects->roots[i], '\\');
        labels[i] = (editor_tab_label){.dirty = projects->changed[i], .name = name && name[1] ? name + 1 : projects->roots[i]};
    }

    editor_strip_draw(&strip, renderer, settings, width - panel, 0, ch * 2, projects->active, labels);
}

bool editor_project_click(editor_projects* projects, editor_tree* tree, ui_renderer* renderer, ui_window* window, u32 width, i32 x) {
    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    u32 panel = editor_tree_width(tree, width);
    editor_tab_strip strip = editor_strip_layout(&projects->scroll, projects->count, projects->active, panel, cw, false);
    size_t index = 0;
    editor_tab_action action = editor_strip_click(&strip, &projects->scroll, x - (i32)(width - panel), &index);
    return editor_project_action(projects, tree, window, action, index);
}

bool editor_project_action(editor_projects* projects, editor_tree* tree, ui_window* window,
                           editor_tab_action action, size_t index) {
    if (action == TAB_ADD) {
        char root[SYS_PATH_CAPACITY];
        return !ui_choose_folder(window, root) || editor_project_open(projects, tree, root);
    }

    if (action == TAB_SELECT) { return editor_project_open(projects, tree, projects->roots[index]); }
    if (action == TAB_CLOSE && index < projects->count) {
        if (projects->count > 1 && index == projects->active) {
            size_t next = index ? index - 1 : 1;
            if (!editor_project_open(projects, tree, projects->roots[next])) { return false; }
        }

        sys_folder_watch_destroy(projects->watches[index]);
        --projects->count;
        size_t remaining = projects->count - index;
        memmove(projects->watches + index, projects->watches + index + 1, remaining * sizeof(projects->watches[0]));
        memmove(projects->changed + index, projects->changed + index + 1, remaining * sizeof(projects->changed[0]));
        memmove(projects->roots + index, projects->roots + index + 1, (projects->count - index) * sizeof(projects->roots[0]));
        if (index < projects->active) { --projects->active; }
        if (!projects->count) {
            editor_tree_destroy(tree);
            tree->rows = NULL;
            tree->count = tree->scroll = 0;
            tree->root[0] = tree->selected[0] = 0;
        }
    }

    return true;
}
