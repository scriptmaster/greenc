/*
 ** document_strip.h - 2 functions.
 ** editor_tabs_draw - Paint document labels and unsaved indicators.
 ** editor_tabs_click - Dispatch document tab and add-button clicks.
 ** See also: document_strip.c, editor_tabs_types.h.
 */
#pragma once
#include "editor_tabs_types.h"

void editor_tabs_draw(editor_tabs* tabs, const editor_document* document, ui_renderer* renderer,
                      u32 width, const editor_settings* settings);
void editor_tabs_click(editor_tabs* tabs, editor_document* document, editor_settings* settings,
                       ui_renderer* renderer, u32 width, i32 x);
