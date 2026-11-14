/*
 ** document_edit.h - 4 functions.
 ** editor_selection - Return the ordered selection endpoints.
 ** editor_replace_selection - Replace the selected range and update its caret and fold state.
 ** editor_insert_character - Insert a supported character or expand a tab.
 ** editor_delete_character - Remove the selection or adjacent character.
 ** See also: document_edit.c, editor_document_types.h.
 */
#pragma once
#include "editor_document_types.h"

void editor_selection(const editor_document* document, size_t* start, size_t* end);
bool editor_replace_selection(editor_document* document, const char* text, size_t count);
void editor_insert_character(editor_document* document, u32 character);
void editor_delete_character(editor_document* document, bool backward);
