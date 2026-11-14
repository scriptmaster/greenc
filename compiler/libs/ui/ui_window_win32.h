#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ui_window.h"

struct ui_window {
    HWND handle;
    HCURSOR cursor;
    HDC dc;
    HANDLE frame_timer;
    u64 clock_frequency;
    u64 frame_deadline;
    u32 frame_remainder;
    u32 target_fps;
    bool close_button;
    bool close_requested;
    ui_input inputs[256];
    size_t input_start, input_count;
};

/* Internal presenter used by the common canvas, never by applications. */
bool ui_gpu_present_framebuffer(ui_gpu* gpu, const ui_framebuffer* framebuffer);
