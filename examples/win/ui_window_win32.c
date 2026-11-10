#include "ui_window_win32.h"
#include <limits.h>

static LRESULT CALLBACK ui_window_proc(HWND window, UINT message,
                                      WPARAM wparam, LPARAM lparam) {
    if (message == WM_KEYDOWN && wparam == VK_ESCAPE) {
        PostMessageA(window, WM_CLOSE, 0, 0);

        return 0;
    }

    if (message == WM_CLOSE) {
        /* The application releases renderers before destroying the window. */
        PostQuitMessage(0);

        return 0;
    }

    return DefWindowProcA(window, message, wparam, lparam);
}

ui_window* ui_window_create_fullscreen(const char* title, u32 target_fps) {
    if (target_fps == 0) {
        target_fps = UI_DEFAULT_TARGET_FPS;
    }

    if (target_fps > 1000) {
        return NULL;
    }

    HINSTANCE instance = GetModuleHandleA(NULL);
    WNDCLASSA wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = ui_window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GreenCFullscreenExample";
    if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return NULL;
    }

    POINT origin = {0, 0};
    MONITORINFO monitor = {0};
    monitor.cbSize = sizeof(monitor);
    if (!GetMonitorInfoA(MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY), &monitor)) {
        return NULL;
    }

    ui_window* window = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*window));
    if (!window) {
        return NULL;
    }

    RECT bounds = monitor.rcMonitor;
    window->handle = CreateWindowExA(0, wc.lpszClassName, title, WS_POPUP,
                                    bounds.left, bounds.top, bounds.right - bounds.left,
                                    bounds.bottom - bounds.top, NULL, NULL, instance, NULL);
    if (!window->handle) {
        ui_window_destroy(window);

        return NULL;
    }

    window->dc = GetDC(window->handle);
    if (!window->dc) {
        ui_window_destroy(window);

        return NULL;
    }

    LARGE_INTEGER frequency, now;
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&now)) {
        ui_window_destroy(window);

        return NULL;
    }

    window->clock_frequency = (u64)frequency.QuadPart;
    window->target_fps = target_fps;
    window->frame_deadline = (u64)now.QuadPart;
    window->frame_timer = CreateWaitableTimerExW(NULL, NULL,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);
    if (!window->frame_timer) {
        ui_window_destroy(window);

        return NULL;
    }

    ShowWindow(window->handle, SW_SHOW);

    return window;
}

void ui_window_destroy(ui_window* window) {
    if (!window) {
        return;
    }

    if (window->frame_timer) {
        CloseHandle(window->frame_timer);
    }

    if (window->dc) {
        ReleaseDC(window->handle, window->dc);
    }

    if (window->handle) {
        DestroyWindow(window->handle);
    }

    HeapFree(GetProcessHeap(), 0, window);
}

bool ui_window_poll_events(void) {
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            return false;
        }

        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

bool ui_window_is_visible(const ui_window* window) {
    return !IsIconic(window->handle);
}

bool ui_window_wait_frame(ui_window* window) {
    /* Carry fractional ticks so 144/240 FPS do not accumulate rounding drift. */
    window->frame_deadline += window->clock_frequency / window->target_fps;
    window->frame_remainder += (u32)(window->clock_frequency % window->target_fps);
    window->frame_deadline += window->frame_remainder / window->target_fps;
    window->frame_remainder %= window->target_fps;

    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return false;
    }

    u64 now = (u64)counter.QuadPart;
    if (now >= window->frame_deadline) {
        /* A slow frame or minimized pause must not trigger catch-up frames. */
        window->frame_deadline = now;
        window->frame_remainder = 0;

        return true;
    }

    while (now < window->frame_deadline) {
        u64 remaining = window->frame_deadline - now;
        LARGE_INTEGER due;
        /* Relative timer deadlines are negative, in 100-nanosecond units. */
        due.QuadPart = -(LONGLONG)((f64)remaining * 10000000.0 /
                                  (f64)window->clock_frequency + 1.0);
        if (!SetWaitableTimer(window->frame_timer, &due, 0, NULL, NULL, FALSE) ||
            WaitForSingleObject(window->frame_timer, INFINITE) != WAIT_OBJECT_0 ||
            !QueryPerformanceCounter(&counter)) {
            return false;
        }

        now = (u64)counter.QuadPart;
    }

    return true;
}

void ui_sleep_ms(u32 milliseconds) {
    Sleep(milliseconds);
}

u64 ui_time_ms(void) {
    LARGE_INTEGER counter, frequency;
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency)) {
        return GetTickCount64();
    }

    u64 ticks = (u64)counter.QuadPart;
    u64 hz = (u64)frequency.QuadPart;

    return ticks / hz * 1000 + ticks % hz * 1000 / hz;
}

i32 ui_report_error(const char* message) {
    MessageBoxA(NULL, message, "GreenC fullscreen example", MB_OK | MB_ICONERROR);

    return 1;
}

bool ui_framebuffer_init(ui_framebuffer* framebuffer, size_t width, size_t height) {
    if (!framebuffer || framebuffer->pixels || !width || !height ||
        width > INT_MAX || height > INT_MAX ||
        (size_t)width > SIZE_MAX / sizeof(u32) / height) {
        return false;
    }

    u32* pixels = VirtualAlloc(NULL, (size_t)width * height * sizeof(*pixels),
                                   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pixels) {
        return false;
    }

    framebuffer->pixels = pixels;
    framebuffer->width = width;
    framebuffer->height = height;

    return true;
}

void ui_framebuffer_destroy(ui_framebuffer* framebuffer) {
    if (!framebuffer) {
        return;
    }

    if (framebuffer->pixels) {
        VirtualFree(framebuffer->pixels, 0, MEM_RELEASE);
    }

    *framebuffer = (ui_framebuffer){0};
}

bool ui_framebuffer_present(ui_window* window, const ui_framebuffer* framebuffer) {
    RECT client;
    if (!GetClientRect(window->handle, &client)) {
        return false;
    }

    if (!client.right || !client.bottom) {
        return true;
    }

    BITMAPINFO bitmap = {0};
    bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth = (LONG)framebuffer->width;
    bitmap.bmiHeader.biHeight = -(LONG)framebuffer->height;
    bitmap.bmiHeader.biPlanes = 1;
    bitmap.bmiHeader.biBitCount = 32;
    bitmap.bmiHeader.biCompression = BI_RGB;

    int copied = StretchDIBits(window->dc, 0, 0, client.right, client.bottom,
                              0, 0, framebuffer->width, framebuffer->height,
                              framebuffer->pixels, &bitmap, DIB_RGB_COLORS, SRCCOPY);

    return copied != (int)GDI_ERROR && copied != 0;
}
