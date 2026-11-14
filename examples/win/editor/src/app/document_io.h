/*
 ** document_io.h - 3 functions.
 ** editor_load - Load the startup path, allowing a new document if missing.
 ** editor_load_existing - Open an existing file and retain the precise failure reason.
 ** editor_save - Replace the destination atomically with the current document.
 ** See also: document_io.c, editor_document_types.h.
 */
#pragma once
#include "editor_document_types.h"

bool editor_load(editor_document* document, const char* path);
bool editor_load_existing(editor_document* document, const char* path);
void editor_save(editor_document* document, const char* path);
