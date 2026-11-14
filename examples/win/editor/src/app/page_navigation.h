/*
 ** page_navigation.h - 2 functions.
 ** editor_text_page - Move the caret and viewport through visible text rows.
 ** editor_hex_page - Move the byte selection and viewport through the grid.
 ** See also: page_navigation.c.
 */
#pragma once
#include "view_internal.h"

void editor_text_page(editor_view* view, editor_document* doc, const editor_viewport* viewport, ui_input input);
void editor_hex_page(editor_view* view, editor_document* doc, ui_input input);
