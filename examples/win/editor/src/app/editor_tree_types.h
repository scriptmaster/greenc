/*
 ** editor_tree_types.h - 0 functions.
 ** Collect the related module declarations.
 ** See also: editor_document.h, editor_settings.h, editor_tree.h, tree_input.h, tree_panel.h, tree_roots.h.
 */
#pragma once
#include "editor_document.h"
#include "editor_settings.h"
#include "sys/fs/directory.h"

typedef struct editor_tree_row editor_tree_row;
typedef struct {
    editor_tree_row* rows;
    size_t count, scroll;
    bool visible, resizing, focused, selected_folder, replace_name;
    u16 naming;
    char root[SYS_PATH_CAPACITY], followed[SYS_PATH_CAPACITY], selected[SYS_PATH_CAPACITY];
    char destination[SYS_PATH_CAPACITY], name[SYS_PATH_CAPACITY];
    u32 preferred_width;
    i32 grip_offset;
    char current_path[SYS_PATH_CAPACITY];
} editor_tree;
