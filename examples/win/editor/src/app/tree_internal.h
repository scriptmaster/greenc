/*
 ** tree_internal.h - 1 function.
 ** insert_children - Insert a sorted directory listing into the visible tree.
 ** See also: directory_listing.c, editor_tree.h, editor_icons.h.
 */
#pragma once
#include "editor_tree.h"
#include "editor_icons.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct editor_tree_row {
    char path[SYS_PATH_CAPACITY];
    size_t name_offset, depth;
    u64 changed;
    bool folder, hidden, expanded, empty;
};

typedef struct {
    editor_tree_row* rows;
    size_t count, capacity, depth;
    const char* parent;
} directory_listing;

bool insert_children(editor_tree* tree, size_t index, const char* path, size_t depth);
