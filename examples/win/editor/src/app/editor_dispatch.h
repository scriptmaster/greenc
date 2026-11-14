/*
 ** editor_dispatch.h - 5 functions.
 ** editor_process_input - Drain window events and dispatch editor commands.
 ** editor_mouse_input - Route pointer events to the appropriate pane or control.
 ** editor_terminal_divider - Test the terminal's resize boundary.
 ** editor_pointer_shape - Choose the native pointer shape for the current interaction.
 ** editor_menu_input - Translate native menu commands into editor bindings.
 ** See also: event_dispatch.c, mouse_dispatch.c, pointer_shape.c, menu_commands.c, editor_commands.h.
 */
#pragma once
#include "editor_commands.h"

bool editor_process_input(ui_renderer* renderer, ui_window* window, editor_document* document, editor_view* view,
                                  editor_settings* settings);
bool editor_mouse_input(ui_renderer* renderer, ui_window* window, editor_document* document,
                               editor_view* view, editor_settings* settings, ui_input input);
bool editor_terminal_divider(const editor_terminal* panel, i32 top, i32 y);
ui_cursor editor_pointer_shape(ui_window* window, const editor_view* view, const editor_settings* settings);
ui_input editor_menu_input(u16 command);
