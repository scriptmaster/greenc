/*
 ** editor_session.h - 3 functions.
 ** follow_active_file - Keep the appropriate project active for the current document.
 ** cycle_frame_rate - Apply the next program-configured frame rate.
 ** editor_run - Run the editor event loop and release its session resources.
 ** See also: active_file.c, frame_rate.c, editor_loop.c, editor_commands.h.
 */
#pragma once
#include "editor_commands.h"

void follow_active_file(editor_view* view, editor_document* document, const editor_settings* settings);
bool cycle_frame_rate(ui_window* window, const editor_settings* settings);
i32 editor_run(ui_window* window, ui_renderer* renderer, editor_document* document,
                       editor_settings* settings);
