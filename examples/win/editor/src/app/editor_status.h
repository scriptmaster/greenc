/*
 ** editor_status.h - 3 functions.
 ** editor_status_draw - Arrange the path, error, statistics and FPS control.
 ** editor_status_hit - Test the footer's clickable FPS region.
 ** editor_marquee_offset - Compute the path offset with a pause at each end.
 ** See also: editor_status.c, editor_document.h, editor_settings.h.
 */
#pragma once
#include "editor_document.h"
#include "sys/process_stats.h"
#include "editor_settings.h"
#include "sys/fs/directory.h"

typedef struct {
    char path[SYS_PATH_CAPACITY];
    u32 path_width;
    u64 started, sampled;
    sys_process_stats usage;
    f64 cpu_percent;
    i32 fps_left, fps_right, top, bottom;
    bool pressed;
} editor_status_bar;

void editor_status_draw(editor_status_bar* bar, ui_renderer* renderer, ui_window* window,
                        const editor_document* document, const editor_settings* settings,
                        u32 width, u32 height, size_t line, size_t column);
bool editor_status_hit(const editor_status_bar* bar, i32 x, i32 y);
u32 editor_marquee_offset(u32 overflow, u64 elapsed_ms);
