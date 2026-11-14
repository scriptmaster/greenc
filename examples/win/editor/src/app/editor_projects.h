/*
 ** editor_projects.h - 5 functions.
 ** editor_project_action - Apply a folder-tab command.
 ** editor_project_open - Activate an existing root or add a folder tab.
 ** editor_project_follow - Resolve a named document to its containing project.
 ** editor_project_draw - Paint the active folder strip in the Files panel.
 ** editor_project_click - Dispatch folder selection, browsing, scrolling and closure.
 ** See also: editor_projects.c, project_strip.c, editor_tree.h, editor_tab_strip.h.
 */
#pragma once
#include "editor_tree.h"
#include "editor_tab_strip.h"

enum { EDITOR_PROJECT_LIMIT = 24 };
typedef struct {
    char roots[EDITOR_PROJECT_LIMIT][SYS_PATH_CAPACITY];
    sys_folder_watch* watches[EDITOR_PROJECT_LIMIT];
    bool changed[EDITOR_PROJECT_LIMIT];
    u64 sampled;
    char followed[SYS_PATH_CAPACITY];
    size_t count, active;
    editor_tab_scroll scroll;
} editor_projects;

bool editor_project_open(editor_projects* projects, editor_tree* tree, const char* root);
bool editor_project_follow(editor_projects* projects, editor_tree* tree, const char* path);
void editor_project_draw(editor_projects* projects, editor_tree* tree, ui_renderer* renderer, u32 width, const editor_settings* settings);
bool editor_project_action(editor_projects* projects, editor_tree* tree, ui_window* window, editor_tab_action action, size_t index);
bool editor_project_click(editor_projects* projects, editor_tree* tree, ui_renderer* renderer, ui_window* window, u32 width, i32 x);
