/*
 ** editor_undo.h - 4 functions.
 ** editor_redo - Reapply the next undone change in this document.
 ** editor_record_edit - Capture replaced bytes and the pre-edit selection.
 ** editor_undo - Reverse the most recent edit in this document.
 ** editor_history_release - Dispose of all remaining history records.
 ** See also: editor_undo.c, document_edit.c, hex_input.c.
 */
#pragma once
#include "editor_document_types.h"

bool editor_record_edit(editor_document* doc, size_t offset, size_t removed, size_t inserted);
void editor_redo(editor_document* doc);
void editor_undo(editor_document* doc);
void editor_history_release(editor_document* doc);
