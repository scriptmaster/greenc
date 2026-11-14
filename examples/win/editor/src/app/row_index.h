/*
 ** row_index.h - 2 functions.
 ** editor_index_rows - Build the flat visible-row index for a document revision.
 ** editor_cursor_row - Locate a byte offset in the cached visible rows.
 ** See also: row_index.c, view_internal.h.
 */
#pragma once
#include "view_internal.h"

bool editor_index_rows(editor_view* view, editor_document* document);
size_t editor_cursor_row(const editor_view* view, size_t cursor);
