#include "ui_window_win32.h"
#include <limits.h>
#include <string.h>

static ui_key ui_key_from_native(WPARAM key) {
    if (key >= 'A' && key <= 'Z') {
        return (ui_key)(UI_KEY_A + key - 'A');
    }

    if (key >= VK_F1 && key <= VK_F12) {
        return (ui_key)(UI_KEY_F1 + key - VK_F1);
    }

    switch (key) {
        case VK_OEM_3: return UI_KEY_GRAVE;
        case VK_ESCAPE: return UI_KEY_ESCAPE;
        case VK_LEFT: return UI_KEY_LEFT;
        case VK_RIGHT: return UI_KEY_RIGHT;
        case VK_UP: return UI_KEY_UP;
        case VK_PRIOR: return UI_KEY_PAGE_UP;
        case VK_NEXT: return UI_KEY_PAGE_DOWN;
        case VK_DOWN: return UI_KEY_DOWN;
        case VK_HOME: return UI_KEY_HOME;
        case VK_END: return UI_KEY_END;
        case VK_DELETE: return UI_KEY_DELETE;
        default: return UI_KEY_NONE;
    }
}

static LRESULT CALLBACK ui_window_proc(HWND window, UINT message,
                                      WPARAM wparam, LPARAM lparam) {
    ui_window* state = (ui_window*)GetWindowLongPtrA(window, GWLP_USERDATA);
    if (message == WM_NCCREATE) {
        CREATESTRUCTA* creation = (CREATESTRUCTA*)lparam;
        state = creation->lpCreateParams;
        SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)state);
    }

    if (state && state->cursor && message == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT) {
        SetCursor(state->cursor);

        return TRUE;
    }

    if (state && message == WM_COMMAND && !lparam && state->input_count < 256) {
        size_t index = (state->input_start + state->input_count++) % 256;
        state->inputs[index] = (ui_input){.menu_command = LOWORD(wparam)};

        return 0;
    }

    if (state && message == WM_CLOSE) {
        if (state->close_button) {
            state->close_requested = true;
        }

        return 0;
    }

    bool drag = message == WM_MOUSEMOVE && GetCapture() == window;
    if (state && (message == WM_MOUSEWHEEL || message == WM_CHAR || message == WM_KEYDOWN || drag ||
                  message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONDOWN)) {
        ui_input input = {0};
        input.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        input.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (message == WM_RBUTTONUP || message == WM_MBUTTONDOWN) {
            input.x = (i16)LOWORD(lparam);
            input.y = (i16)HIWORD(lparam);
            input.mouse = message == WM_MBUTTONDOWN ? UI_MOUSE_MIDDLE : UI_MOUSE_CONTEXT;
        } else if (message == WM_MOUSEWHEEL) {
            POINT point = {(i16)LOWORD(lparam), (i16)HIWORD(lparam)};
            ScreenToClient(window, &point);
            input.x = point.x;
            input.y = point.y;
            input.wheel_delta = (i16)HIWORD(wparam);
            input.mouse = UI_MOUSE_WHEEL;
        } else if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || drag) {
            input.x = (i16)LOWORD(lparam);
            input.y = (i16)HIWORD(lparam);
            if (message == WM_LBUTTONDOWN) {
                SetFocus(window);
                SetCapture(window);
                input.mouse = UI_MOUSE_DOWN;
            } else if (drag) {
                input.mouse = UI_MOUSE_DRAG;
            } else {
                if (GetCapture() == window) {
                    ReleaseCapture();
                }

                input.mouse = UI_MOUSE_UP;
            }
        } else if (message == WM_CHAR && wparam < 128) {
            input.character = wparam == '\r' ? '\n' : (u32)wparam;
        } else if (message == WM_KEYDOWN) {
            input.key = ui_key_from_native(wparam);
        }

        if ((input.mouse || input.character || input.key != UI_KEY_NONE) && state->input_count < 256) {
            size_t index = (state->input_start + state->input_count) % 256;
            state->inputs[index] = input;
            ++state->input_count;
        }

        return 0;
    }

    return DefWindowProcA(window, message, wparam, lparam);
}

static bool ui_window_config_is_valid(const ui_window_config* config) {
    if (!config || !config->title || config->target_fps > 1000 ||
        config->mode < UI_WINDOW_WINDOWED || config->mode > UI_WINDOW_FULLSCREEN ||
        (config->mode != UI_WINDOW_FULLSCREEN &&
         (!config->width || !config->height || config->width > INT_MAX / 2 ||
          config->height > INT_MAX / 2)) ||
        (!config->title_bar && (config->minimize_button || config->maximize_button)) ||
        (config->maximize_button && !config->resizable)) {
        return false;
    }

    return true;
}

static bool ui_window_create_native(ui_window* window, const ui_window_config* config) {
    HINSTANCE instance = GetModuleHandleA(NULL);
    WNDCLASSA wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = ui_window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GreenCWindow";
    if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    POINT origin = {0, 0};
    MONITORINFO monitor = {0};
    monitor.cbSize = sizeof(monitor);
    if (!GetMonitorInfoA(MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY), &monitor)) {
        return false;
    }

    DWORD style = WS_POPUP;
    RECT bounds = monitor.rcMonitor;
    int x = bounds.left, y = bounds.top;
    if (config->mode != UI_WINDOW_FULLSCREEN) {
        if (config->title_bar) {
            style = WS_CAPTION | WS_SYSMENU;
            if (config->minimize_button) {
                style |= WS_MINIMIZEBOX;
            }

            if (config->maximize_button) {
                style |= WS_MAXIMIZEBOX;
            }
        }

        if (config->resizable) {
            style |= WS_THICKFRAME;
        }

        bounds = (RECT){0, 0, (LONG)config->width, (LONG)config->height};
        if (!AdjustWindowRectEx(&bounds, style, FALSE, 0)) {
            return false;
        }

        x = monitor.rcWork.left + ((monitor.rcWork.right - monitor.rcWork.left) -
                                  (bounds.right - bounds.left)) / 2;
        y = monitor.rcWork.top + ((monitor.rcWork.bottom - monitor.rcWork.top) -
                                 (bounds.bottom - bounds.top)) / 2;
    }

    window->close_button = config->close_button;
    window->handle = CreateWindowExA(0, wc.lpszClassName, config->title, style,
                                    x, y, bounds.right - bounds.left,
                                    bounds.bottom - bounds.top, NULL, NULL, instance, window);
    if (!window->handle) {
        return false;
    }

    return true;
}

ui_window* ui_window_create(const ui_window_config* config) {
    if (!ui_window_config_is_valid(config)) {
        return NULL;
    }

    ui_window* window = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*window));
    if (!window) {
        return NULL;
    }

    if (!ui_window_create_native(window, config)) {
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
    window->target_fps = config->target_fps;
    window->frame_deadline = (u64)now.QuadPart;
    window->frame_timer = CreateWaitableTimerExW(NULL, NULL,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);
    if (!window->frame_timer) {
        ui_window_destroy(window);

        return NULL;
    }

    if (!config->close_button && config->title_bar) {
        EnableMenuItem(GetSystemMenu(window->handle, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    }

    ShowWindow(window->handle, config->mode == UI_WINDOW_MAXIMIZED ? SW_SHOWMAXIMIZED : SW_SHOW);

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

bool ui_window_set_target_fps(ui_window* window, u32 target_fps) {
    LARGE_INTEGER counter;
    if (!window || target_fps > 1000 || !QueryPerformanceCounter(&counter)) {
        return false;
    }

    window->target_fps = target_fps;
    window->frame_deadline = (u64)counter.QuadPart;
    window->frame_remainder = 0;

    return true;
}

u32 ui_window_target_fps(const ui_window* window) {
    return window->target_fps;
}

void ui_window_request_close(ui_window* window) {
    /* An explicit application request is independent of caption-button policy. */
    (void)window;
    PostQuitMessage(0);
}

bool ui_window_pointer(const ui_window* window, i32* x, i32* y) {
    POINT point;
    RECT client;
    if (!window || !x || !y || !GetCursorPos(&point)) { return false; }

    HWND target = WindowFromPoint(point);
    bool captured = GetCapture() == window->handle;
    if (!captured && target != window->handle && !IsChild(window->handle, target)) { return false; }
    if (!ScreenToClient(window->handle, &point) || !GetClientRect(window->handle, &client)) { return false; }
    if (!captured && !PtInRect(&client, point)) { return false; }

    *x = point.x;
    *y = point.y;

    return true;
}

void ui_window_set_cursor(ui_window* window, ui_cursor cursor) {
    if (!window) { return; }

    LPCSTR name = cursor == UI_CURSOR_RESIZE_EW ? IDC_SIZEWE :
                  cursor == UI_CURSOR_RESIZE_NS ? IDC_SIZENS : IDC_ARROW;
    HCURSOR native = LoadCursorA(NULL, name);
    if (!native || native == window->cursor) { return; }

    window->cursor = native;
    i32 x, y;
    if (ui_window_pointer(window, &x, &y)) { SetCursor(native); }
}

bool ui_window_client_size(const ui_window* window, u32* width, u32* height) {
    RECT client;
    if (!GetClientRect(window->handle, &client)) {
        return false;
    }

    *width = (u32)client.right;
    *height = (u32)client.bottom;

    return true;
}

bool ui_window_next_input(ui_window* window, ui_input* input) {
    if (window->close_requested) {
        window->close_requested = false;
        *input = (ui_input){.close_requested = true};

        return true;
    }

    if (!window->input_count) {
        return false;
    }

    *input = window->inputs[window->input_start];
    window->input_start = (window->input_start + 1) % 256;
    --window->input_count;

    return true;
}

bool ui_window_wait_frame(ui_window* window) {
    if (!window->target_fps) {
        return true;
    }

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
    MessageBoxA(NULL, message, "GreenC UI", MB_OK | MB_ICONERROR);

    return 1;
}

bool ui_confirm(const char* title, const char* message) {
    return MessageBoxA(NULL, message, title, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
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

bool ui_clipboard_write(ui_window* window, const char* text, size_t length) {
    if (length >= SIZE_MAX / 2) { return false; }

    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, length * 2 + 1);
    char* output = memory ? GlobalLock(memory) : NULL;
    if (!output) {
        GlobalFree(memory);

        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        if (text[i] == '\n') { *output++ = '\r'; }
        *output++ = text[i];
    }

    *output = 0;
    GlobalUnlock(memory);
    bool copied = false;
    if (OpenClipboard(window->handle)) {
        copied = EmptyClipboard() && SetClipboardData(CF_TEXT, memory);
        CloseClipboard();
    }

    if (!copied) { GlobalFree(memory); }

    return copied;
}

bool ui_clipboard_read(ui_window* window, char* text, size_t capacity) {
    if (!OpenClipboard(window->handle)) { return false; }

    HANDLE memory = GetClipboardData(CF_UNICODETEXT);
    const WCHAR* source = memory ? GlobalLock(memory) : NULL;
    size_t limit = memory ? GlobalSize(memory) / sizeof(WCHAR) : 0;
    size_t i = 0;
    while (source && i < limit && i < capacity && source[i] && source[i] < 128) {
        text[i] = (char)source[i];
        ++i;
    }

    bool complete = source && i < capacity && i < limit && !source[i];
    if (complete) { text[i] = 0; }
    if (source) { GlobalUnlock(memory); }
    CloseClipboard();

    return complete;
}

bool ui_window_set_menu(ui_window* window, const ui_menu_item* items, size_t count) {
    HMENU bar = CreateMenu(), popup = NULL;
    if (!bar) { return false; }

    for (size_t i = 0; i < count; ++i) {
        if (!i || strcmp(items[i].group, items[i - 1].group)) {
            popup = CreatePopupMenu();
            if (!popup || !AppendMenuA(bar, MF_POPUP, (UINT_PTR)popup, items[i].group)) {
                if (popup) { DestroyMenu(popup); }
                goto failure;
            }
        }

        if (!AppendMenuA(popup, MF_STRING, items[i].command, items[i].label)) { goto failure; }
    }

    HMENU old = GetMenu(window->handle);
    if (!SetMenu(window->handle, bar)) { goto failure; }
    if (old) { DestroyMenu(old); }
    DrawMenuBar(window->handle);

    return true;

failure:
    DestroyMenu(bar);

    return false;
}
