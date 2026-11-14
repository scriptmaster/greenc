/*
 ** editor_input.h - 2 functions.
 ** editor_clipboard - Copy, cut or paste the current selection.
 ** editor_input - Apply document keyboard shortcuts and text input.
 ** See also: editor_input.c, editor_document.h.
 */
#pragma once
#include "editor_document.h"
#include "ui/ui_window.h"

/* Application key bindings translate UI transport into document operations. */
void editor_clipboard(editor_document* document, ui_window* window, ui_key key);
void editor_input(editor_document* document, ui_input input);
