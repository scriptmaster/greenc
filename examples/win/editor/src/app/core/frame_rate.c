/*
 ** frame_rate.c - 1 function.
 ** cycle_frame_rate - Apply the next program-configured frame rate.
 ** See also: editor_session.h, main_internal.h.
 */
#include "main_internal.h"

bool cycle_frame_rate(ui_window* window, const editor_settings* settings) {
    let u32 next = editor_next_frame_rate(settings, ui_window_target_fps(window));

    return ui_window_set_target_fps(window, next);
}
