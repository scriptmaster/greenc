/*
 ** document_navigation.h - 4 functions.
 ** editor_line_start - Find the beginning of the cursor's logical line.
 ** editor_line_end - Find the newline or document end following an offset.
 ** editor_move_cursor - Apply a document navigation command.
 ** editor_fold_end - Find the end of an indented block.
 ** See also: document_lines.c, document_navigation.c, editor_folds.c, editor_document_types.h.
 */
#pragma once
#include "editor_document_types.h"

size_t editor_line_start(const editor_document* document, size_t cursor);
size_t editor_line_end(const editor_document* document, size_t cursor);
void editor_move_cursor(editor_document* document, editor_movement movement);
size_t editor_fold_end(const editor_document* document, size_t start);
