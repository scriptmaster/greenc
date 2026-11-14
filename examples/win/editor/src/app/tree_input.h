/*
 ** tree_input.h - 1 function.
 ** editor_tree_input - Dispatch tree scrolling, folder expansion and file selection.
 ** See also: tree_input.c, editor_tree_types.h.
 */
#pragma once
#include "editor_tree_types.h"

void editor_tree_input(editor_tree* tree, ui_renderer* renderer, ui_input input,
                       editor_document* document);
