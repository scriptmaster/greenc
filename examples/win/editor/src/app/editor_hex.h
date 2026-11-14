/*
 ** editor_hex.h - 5 functions.
 ** editor_hex_limit - Count bytes available to the current view.
 ** editor_hex_copy - Copy the inclusive selection as hexadecimal pairs.
 ** editor_hex_draw - Display the byte grid and optional preview action.
 ** editor_hex_mouse - Select bytes, scroll, or activate the full editor.
 ** editor_hex_key - Navigate the grid and overwrite individual nibbles.
 ** See also: hex_view.c, hex_input.c.
 */
#pragma once
#include "editor_view.h"

size_t editor_hex_limit(const editor_document* doc);
void editor_hex_copy(editor_document* doc, ui_window* window);
void editor_hex_draw(editor_view* view, editor_document* doc, ui_renderer* renderer, ui_window* window,
                     u32 width, u32 bottom, const editor_settings* settings);
void editor_hex_mouse(editor_view* view, editor_document* doc, ui_input input);
void editor_hex_key(editor_view* view, editor_document* doc, ui_input input);
