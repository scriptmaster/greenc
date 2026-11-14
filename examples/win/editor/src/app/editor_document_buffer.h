/*
 ** editor_document_buffer.h - 3 functions.
 ** editor_document_changed - Invalidate cached document layout after editing or folding.
 ** editor_document_release - Release the document's text and optional fold storage.
 ** editor_document_reserve - Grow owned text and fold storage without changing existing content.
 ** See also: editor_buffer.c, editor_document_types.h.
 */
#pragma once
#include "editor_document_types.h"

void editor_document_changed(editor_document* document);
void editor_document_release(editor_document* document);
bool editor_document_reserve(editor_document* document, size_t required);
