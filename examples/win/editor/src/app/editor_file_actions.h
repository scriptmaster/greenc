/*
 ** editor_file_actions.h - 3 functions.
 ** editor_files_context - Offer filesystem actions appropriate to the clicked tree row.
 ** editor_files_rename - Start renaming the selected file.
 ** editor_files_name_input - Edit, submit or cancel the tree's inline filename field.
 ** See also: file_actions.c, file_names.c, file_creation.c, editor_view.h.
 */
#pragma once
#include "editor_view.h"

void editor_files_context(editor_view* view, editor_document* document, editor_settings* settings,
                          ui_renderer* renderer, ui_window* window, ui_input input);
void editor_files_rename(editor_tree* tree);
void editor_files_name_input(editor_view* view, editor_document* document, editor_settings* settings, ui_input input);
