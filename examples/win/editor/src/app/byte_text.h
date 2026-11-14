/*
 ** byte_text.h - 4 functions.
 ** editor_byte_needs_marker - Identify bytes outside supported ASCII text.
 ** editor_open_byte_text - Expose a raw document through ordinary text editing.
 ** editor_draw_byte_marks - Paint one middle dot per unsupported byte.
 ** editor_byte_tooltip - Show the original byte under the mouse.
 ** See also: byte_text.c, document_rows.c.
 */
#pragma once
#include "view_internal.h"

bool editor_byte_needs_marker(u8 byte);
void editor_open_byte_text(editor_document* doc);
void editor_draw_byte_marks(ui_renderer* renderer, const char* text, size_t count,
                           const editor_viewport* viewport, i32 y, u32 color);
void editor_byte_tooltip(ui_renderer* renderer, ui_window* window, const editor_document* doc,
                         const editor_view* view, const editor_viewport* viewport, const editor_settings* settings);
