/*
 ** tree_roots.h - 5 functions.
 ** editor_tree_init - Initialize the tree from a startup file.
 ** editor_tree_root - Replace the tree listing only after successful enumeration.
 ** editor_tree_follow - Initialize a tree from a document's containing folder.
 ** editor_tree_select_at - Select the row under the pointer.
 ** editor_tree_destroy - Release owned directory rows.
 ** See also: tree_roots.c, editor_tree_types.h.
 */
#pragma once
#include "editor_tree_types.h"

void editor_tree_init(editor_tree* tree, const char* path);
bool editor_tree_root(editor_tree* tree, const char* folder);
bool editor_tree_follow(editor_tree* tree, const char* path);
void editor_tree_select_at(editor_tree* tree, ui_renderer* renderer, i32 y);
void editor_tree_destroy(editor_tree* tree);
