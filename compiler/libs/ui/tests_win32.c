/* Native integration checks; private handles are inspected only in this test. */
#include "ui_window_win32.h"
#include "../sys/fs/write_atomic.h"
#include <GL/gl.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool fail_gpu_creation, fail_gpu_presentation;
ui_gpu* __real_ui_gpu_create(ui_window* window);
bool __real_ui_gpu_present_framebuffer(ui_gpu* gpu, const ui_framebuffer* framebuffer);

ui_gpu* __wrap_ui_gpu_create(ui_window* window) {
    return fail_gpu_creation ? NULL : __real_ui_gpu_create(window);
}

bool __wrap_ui_gpu_present_framebuffer(ui_gpu* gpu, const ui_framebuffer* framebuffer) {
    return !fail_gpu_presentation && __real_ui_gpu_present_framebuffer(gpu, framebuffer);
}

static void ui_test_canvas(ui_renderer* renderer, ui_window* window) {
    /* Present GDI into a real compatible bitmap so desktop occlusion cannot
     * alter pixel readback. The GL child still presents to its own window. */
    HDC display = window->dc;
    HDC target = CreateCompatibleDC(display);
    u32 width, height;
    assert(ui_window_client_size(window, &width, &height));
    BITMAPINFO info = {.bmiHeader = {.biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = (LONG)width, .biHeight = -(LONG)height, .biPlanes = 1,
        .biBitCount = 32, .biCompression = BI_RGB}};
    void* pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(display, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
    assert(target && bitmap);
    HGDIOBJ original = SelectObject(target, bitmap);
    assert(original && original != HGDI_ERROR);
    window->dc = target;
    assert(ui_renderer_begin(renderer, 0x151a22));
    ui_renderer_rect(renderer, 20, 20, 100, 100, 0x123456);
    ui_renderer_text(renderer, 24, 140, "ASCII canvas", 0xffffff);
    assert(ui_renderer_present(renderer));
    if (ui_renderer_get_backend(renderer) == UI_BACKEND_CPU) {
        /* GDI may dither a solid DC brush. Verify its color over a tile. */
        unsigned red = 0, green = 0, blue = 0;
        for (int y = 32; y < 96; ++y) {
            for (int x = 32; x < 96; ++x) {
                COLORREF pixel = GetPixel(target, x, y);
                assert(pixel != CLR_INVALID);
                red += GetRValue(pixel);
                green += GetGValue(pixel);
                blue += GetBValue(pixel);
            }
        }

        assert(abs((int)(red / (64 * 64)) - 0x12) <= 2);
        assert(abs((int)(green / (64 * 64)) - 0x34) <= 2);
        assert(abs((int)(blue / (64 * 64)) - 0x56) <= 2);
    } else {
        u32 width, height;
        assert(ui_window_client_size(window, &width, &height));
        u8 pixel[4] = {0};
        glReadBuffer(GL_FRONT);
        glReadPixels(100, (GLint)height - 101, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
        assert(glGetError() == GL_NO_ERROR);
        assert(pixel[0] == 0x12 && pixel[1] == 0x34 && pixel[2] == 0x56);
    }

    window->dc = display;
    assert(SelectObject(target, original));
    assert(DeleteObject(bitmap));
    assert(DeleteDC(target));
}

int main(void) {
    ui_window_config config = {
        .title = "GreenC UI integration checks", .width = 640, .height = 360,
        .target_fps = 60, .mode = UI_WINDOW_WINDOWED, .title_bar = true,
        .resizable = true, .minimize_button = true, .maximize_button = true,
        .close_button = false
    };
    ui_window_config invalid = config;
    invalid.target_fps = 1001;
    assert(!ui_window_create(&invalid));
    invalid = config;
    invalid.width = 0;
    assert(!ui_window_create(&invalid));
    invalid = config;
    invalid.title_bar = false;
    assert(!ui_window_create(&invalid));

    ui_window* window = ui_window_create(&config);
    assert(window);
    u32 width, height;
    assert(ui_window_client_size(window, &width, &height));
    assert(width == 640 && height == 360);
    ui_window_set_cursor(window, UI_CURSOR_RESIZE_EW);
    SendMessageA(window->handle, WM_SETCURSOR, (WPARAM)window->handle, HTCLIENT);
    assert(GetCursor() == LoadCursorA(NULL, IDC_SIZEWE));
    ui_window_set_cursor(window, UI_CURSOR_RESIZE_NS);
    SendMessageA(window->handle, WM_SETCURSOR, (WPARAM)window->handle, HTCLIENT);
    assert(GetCursor() == LoadCursorA(NULL, IDC_SIZENS));
    ui_window_set_cursor(window, UI_CURSOR_ARROW);
    SendMessageA(window->handle, WM_SETCURSOR, (WPARAM)window->handle, HTCLIENT);
    assert(GetCursor() == LoadCursorA(NULL, IDC_ARROW));

    LONG_PTR style = GetWindowLongPtrA(window->handle, GWL_STYLE);
    assert((style & (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) ==
           (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX));
    assert(GetMenuState(GetSystemMenu(window->handle, FALSE), SC_CLOSE, MF_BYCOMMAND) & MF_GRAYED);
    SendMessageA(window->handle, WM_CLOSE, 0, 0);
    assert(ui_window_poll_events());
    SendMessageA(window->handle, WM_CHAR, 'A', 0);
    SendMessageA(window->handle, WM_KEYDOWN, VK_F6, 0);
    SendMessageA(window->handle, WM_KEYDOWN, VK_ESCAPE, 0);
    SendMessageA(window->handle, WM_RBUTTONUP, 0, MAKELPARAM(18, 32));
    SendMessageA(window->handle, WM_MBUTTONDOWN, 0, MAKELPARAM(22, 48));
    ui_input input;
    assert(ui_window_next_input(window, &input) && input.character == 'A');
    assert(ui_window_next_input(window, &input) && input.key == UI_KEY_F6);
    assert(ui_window_next_input(window, &input) && input.key == UI_KEY_ESCAPE);
    assert(ui_window_next_input(window, &input) && input.mouse == UI_MOUSE_CONTEXT && input.x == 18 && input.y == 32);
    assert(ui_window_next_input(window, &input) && input.mouse == UI_MOUSE_MIDDLE && input.x == 22 && input.y == 48);
    assert(!ui_window_next_input(window, &input));
    assert(ui_window_poll_events()); /* Escape is application policy. */
    assert(!ui_window_set_target_fps(window, 1001));
    assert(ui_window_target_fps(window) == 60);
    assert(ui_window_set_target_fps(window, 144));
    u64 start = ui_time_ms();
    for (size_t i = 0; i < 6; ++i) {
        assert(ui_window_wait_frame(window));
    }

    assert(ui_time_ms() - start >= 35);
    assert(ui_window_set_target_fps(window, 0));
    assert(ui_window_wait_frame(window));

    ui_renderer_config rendering = {
        .preference = UI_RENDERER_CPU, .font_name = "Consolas",
        .cell_width = 8, .cell_height = 16
    };
    ui_renderer* renderer = ui_renderer_create(window, &rendering);
    assert(renderer && ui_renderer_get_backend(renderer) == UI_BACKEND_CPU);
    ui_test_canvas(renderer, window);
    assert(MoveWindow(window->handle, 100, 100, 800, 500, TRUE));
    ui_test_canvas(renderer, window);
    ui_renderer_destroy(renderer);

    fail_gpu_creation = true;
    rendering.preference = UI_RENDERER_AUTO;
    renderer = ui_renderer_create(window, &rendering);
    assert(renderer && ui_renderer_get_backend(renderer) == UI_BACKEND_CPU);
    ui_test_canvas(renderer, window);
    ui_renderer_destroy(renderer);
    fail_gpu_creation = false;

    rendering.preference = UI_RENDERER_AUTO;
    renderer = ui_renderer_create(window, &rendering);
    assert(renderer);
    bool hardware = ui_renderer_get_backend(renderer) == UI_BACKEND_GPU;
    ui_test_canvas(renderer, window);
    fail_gpu_presentation = true;
    ui_test_canvas(renderer, window);
    assert(ui_renderer_get_backend(renderer) == UI_BACKEND_CPU);
    ui_renderer_destroy(renderer);
    fail_gpu_presentation = false;
    ui_window_destroy(window);

    config.mode = UI_WINDOW_MAXIMIZED;
    window = ui_window_create(&config);
    assert(window);
    WINDOWPLACEMENT placement = {.length = sizeof(placement)};
    assert(GetWindowPlacement(window->handle, &placement));
    assert(placement.showCmd == SW_SHOWMAXIMIZED);
    ui_window_destroy(window);

    config.mode = UI_WINDOW_WINDOWED;
    config.title_bar = false;
    config.resizable = false;
    config.minimize_button = false;
    config.maximize_button = false;
    config.close_button = true;
    window = ui_window_create(&config);
    assert(window);
    style = GetWindowLongPtrA(window->handle, GWL_STYLE);
    assert(!(style & (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)));
    ui_window_destroy(window);

    config.mode = UI_WINDOW_FULLSCREEN;
    config.width = config.height = 0;
    window = ui_window_create(&config);
    assert(window);
    RECT bounds;
    MONITORINFO monitor = {.cbSize = sizeof(monitor)};
    assert(GetWindowRect(window->handle, &bounds));
    assert(GetMonitorInfoA(MonitorFromWindow(window->handle, MONITOR_DEFAULTTOPRIMARY), &monitor));
    assert(EqualRect(&bounds, &monitor.rcMonitor));
    SendMessageA(window->handle, WM_CLOSE, 0, 0);
    assert(ui_window_poll_events());
    assert(ui_window_next_input(window, &input) && input.close_requested);
    assert(ui_window_poll_events()); /* Program can veto a native close. */
    ui_window_request_close(window);
    assert(!ui_window_poll_events());
    ui_window_destroy(window);

    char directory[MAX_PATH], path[MAX_PATH], text[16] = {0};
    assert(GetTempPathA(MAX_PATH, directory));
    assert(GetTempFileNameA(directory, "gct", 0, path));
    assert(sys_file_write_atomic(path, "original", 8));
    assert(sys_file_write_atomic(path, "replacement", 11));
    FILE* file = fopen(path, "rb");
    assert(file && fread(text, 1, sizeof(text), file) == 11);
    assert(fclose(file) == 0 && strcmp(text, "replacement") == 0);
    HANDLE locked = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    assert(locked != INVALID_HANDLE_VALUE);
    assert(!sys_file_write_atomic(path, "lost", 4));
    assert(CloseHandle(locked));
    file = fopen(path, "rb");
    memset(text, 0, sizeof(text));
    assert(file && fread(text, 1, sizeof(text), file) == 11);
    assert(fclose(file) == 0 && strcmp(text, "replacement") == 0);
    assert(DeleteFileA(path));
    printf("PASS: windows, input, dynamic FPS, resize, CPU, GPU setup/runtime fallback, atomic save.\n");
    printf("Hardware presentation verified: %s\n", hardware ? "yes" : "unavailable (CPU fallback verified)");

    return 0;
}
