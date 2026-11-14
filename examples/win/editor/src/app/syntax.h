/*
 ** syntax.h - 2 functions.
 ** syntax_comment_after - Track block comments between logical lines.
 ** editor_draw_syntax - Color the visible portion of a source line.
 ** See also: syntax.c, document_rows.c.
 */
#pragma once
#include "view_internal.h"

bool syntax_comment_after(const char* line, size_t length, bool comment);
void editor_draw_syntax(ui_renderer* renderer, const editor_settings* settings, const char* line,
                        size_t line_length, bool comment, size_t skip, size_t count, i32 left, i32 y, u32 cell_width);
