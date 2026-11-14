/*
 ** tree_panel.h - 4 functions.
 ** editor_tree_width - Clamp the preferred file panel width.
 ** editor_tree_divider - Test the draggable file panel boundary.
 ** editor_tree_resize - Update panel width during a captured drag.
 ** editor_tree_draw - Render folder rows, selection and inline names.
 ** See also: tree_panel.c, editor_tree_types.h.
 */
#pragma once
#include "editor_tree_types.h"

u32 editor_tree_width(const editor_tree* tree, u32 width);
bool editor_tree_divider(const editor_tree* tree, u32 width, i32 bottom, i32 x, i32 y);
bool editor_tree_resize(editor_tree* tree, u32 width, i32 bottom, ui_input input);
void editor_tree_draw(editor_tree* tree, ui_renderer* renderer, u32 width, u32 bottom,
                      const editor_settings* settings);
