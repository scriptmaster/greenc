/*
 ** tab_documents.h - 4 functions.
 ** editor_tabs_save - Save the current tab and associate it with its chosen path.
 ** editor_tabs_close - Close a tab while protecting unsaved edits.
 ** editor_tabs_dirty - Check all documents for pending edits.
 ** editor_tabs_step - Select an adjacent document tab.
 ** See also: tab_documents.c, editor_tabs_types.h.
 */
#pragma once
#include "editor_tabs_types.h"

void editor_tabs_save(editor_tabs* tabs, editor_document* document, editor_settings* settings, ui_window* window, bool save_as);
void editor_tabs_close(editor_tabs* tabs, size_t index, editor_document* document, editor_settings* settings);
bool editor_tabs_dirty(const editor_tabs* tabs, const editor_document* document);
void editor_tabs_step(editor_tabs* tabs, editor_document* document, editor_settings* settings, int direction);
