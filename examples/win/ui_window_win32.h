#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ui_window.h"

struct ui_window {
    HWND handle;
    HDC dc;
    HANDLE frame_timer;
    u64 clock_frequency;
    u64 frame_deadline;
    u32 frame_remainder;
    u32 target_fps;
};
