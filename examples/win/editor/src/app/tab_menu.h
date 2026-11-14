/*
 ** tab_menu.h - 1 function.
 ** editor_tab_menu - Handle a context click on any visible tab.
 ** See also: tab_menu.c, tab_export.c.
 */
#pragma once
#include "editor_view.h"

bool editor_tab_menu(editor_view* view, editor_document* doc, editor_settings* settings, ui_renderer* renderer,
                     ui_window* window, ui_input input, u32 width, i32 terminal_top);
