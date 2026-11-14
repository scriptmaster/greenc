/*
 ** tab_export.h - 3 functions.
 ** editor_save_documents - Save selected or all modified document tabs.
 ** editor_save_folders - Export selected or all folder roots.
 ** editor_save_consoles - Export visible terminal screens.
 ** See also: tab_export.c, tab_menu.c.
 */
#pragma once
#include "editor_view.h"

void editor_save_documents(editor_view* view, editor_document* doc, editor_settings* settings, ui_window* window, size_t index, bool all);
bool editor_save_folders(const editor_projects* projects, ui_window* window, size_t index, bool all);
bool editor_save_consoles(editor_terminal* panel, ui_window* window, size_t index, bool all);
