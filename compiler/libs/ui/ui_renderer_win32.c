#include "ui_window_win32.h"
#include <limits.h>
#include <string.h>

struct ui_renderer {
    ui_window* window; /* Borrowed. */
    ui_window surface; /* Isolated GPU child; parent DC stays GDI-compatible. */
    ui_gpu* gpu;
    HDC canvas;
    HBITMAP bitmap;
    HGDIOBJ original_bitmap, original_font;
    HFONT font;
    bool drawing_failed;
    u32 cell_width, cell_height;
    ui_framebuffer framebuffer; /* Pixels belong to bitmap, not VirtualAlloc. */
};

static void ui_renderer_release_gpu(ui_renderer* renderer) {
    ui_gpu_destroy(renderer->gpu);
    renderer->gpu = NULL;
    if (renderer->surface.dc) {
        ReleaseDC(renderer->surface.handle, renderer->surface.dc);
        renderer->surface.dc = NULL;
    }

    if (renderer->surface.handle) {
        DestroyWindow(renderer->surface.handle);
        renderer->surface.handle = NULL;
    }
}

static bool ui_renderer_init_canvas(ui_renderer* renderer, const ui_renderer_config* config) {
    renderer->canvas = CreateCompatibleDC(renderer->window->dc);
    renderer->font = CreateFontA((int)config->cell_height, (int)config->cell_width,
                                 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 config->antialias ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY, FIXED_PITCH, config->font_name);
    if (!renderer->canvas || !renderer->font) {
        return false;
    }

    renderer->original_font = SelectObject(renderer->canvas, renderer->font);
    if (!renderer->original_font || renderer->original_font == HGDI_ERROR) {
        renderer->original_font = NULL;

        return false;
    }

    TEXTMETRICA metrics;
    if (!GetTextMetricsA(renderer->canvas, &metrics) || metrics.tmAveCharWidth <= 0 ||
        metrics.tmHeight <= 0 || (metrics.tmPitchAndFamily & TMPF_FIXED_PITCH) ||
        !SetBkMode(renderer->canvas, TRANSPARENT)) {
        return false;
    }

    renderer->cell_width = (u32)metrics.tmAveCharWidth;
    renderer->cell_height = (u32)metrics.tmHeight;

    return true;
}

static void ui_renderer_try_gpu(ui_renderer* renderer) {
    /* OpenGL fixes a DC's pixel format for its lifetime. A disposable child
     * prevents failed GPU setup/presentation from poisoning the GDI parent. */
    renderer->surface.handle = CreateWindowExA(0, "GreenCWindow", "",
        WS_CHILD | WS_VISIBLE | WS_DISABLED, 0, 0, 1, 1,
        renderer->window->handle, NULL, GetModuleHandleA(NULL), NULL);
    if (renderer->surface.handle) {
        renderer->surface.dc = GetDC(renderer->surface.handle);
        if (renderer->surface.dc) {
            renderer->gpu = ui_gpu_create(&renderer->surface);
        }
    }

    if (!renderer->gpu) {
        ui_renderer_release_gpu(renderer);
    }
}

ui_renderer* ui_renderer_create(ui_window* window, const ui_renderer_config* config) {
    if (!window || !config || !config->font_name || !config->cell_width || !config->cell_height ||
        config->cell_width > INT_MAX || config->cell_height > INT_MAX ||
        (config->preference != UI_RENDERER_AUTO && config->preference != UI_RENDERER_CPU)) {
        return NULL;
    }

    ui_renderer* renderer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*renderer));
    if (!renderer) {
        return NULL;
    }

    renderer->window = window;
    if (!ui_renderer_init_canvas(renderer, config)) {
        ui_renderer_destroy(renderer);

        return NULL;
    }

    if (config->preference == UI_RENDERER_AUTO) {
        ui_renderer_try_gpu(renderer);
    }

    return renderer;
}

void ui_renderer_destroy(ui_renderer* renderer) {
    if (!renderer) {
        return;
    }

    ui_renderer_release_gpu(renderer);
    if (renderer->canvas) {
        if (renderer->original_bitmap) {
            SelectObject(renderer->canvas, renderer->original_bitmap);
        }

        if (renderer->original_font) {
            SelectObject(renderer->canvas, renderer->original_font);
        }

        DeleteDC(renderer->canvas);
    }

    if (renderer->bitmap) {
        DeleteObject(renderer->bitmap);
    }

    if (renderer->font) {
        DeleteObject(renderer->font);
    }

    HeapFree(GetProcessHeap(), 0, renderer);
}

ui_renderer_backend ui_renderer_get_backend(const ui_renderer* renderer) {
    return renderer->gpu ? UI_BACKEND_GPU : UI_BACKEND_CPU;
}

void ui_renderer_cell_size(const ui_renderer* renderer, u32* width, u32* height) {
    *width = renderer->cell_width;
    *height = renderer->cell_height;
}

static bool ui_renderer_resize_canvas(ui_renderer* renderer, u32 width, u32 height) {
    if (width > INT_MAX || height > INT_MAX || (size_t)width > SIZE_MAX / 4 / height) {
        return false;
    }

    BITMAPINFO info = {0};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = (LONG)width;
    info.bmiHeader.biHeight = -(LONG)height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(renderer->canvas, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
    if (!bitmap) {
        return false;
    }

    HGDIOBJ previous = SelectObject(renderer->canvas, bitmap);
    if (!previous || previous == HGDI_ERROR) {
        DeleteObject(bitmap);

        return false;
    }

    if (!renderer->original_bitmap) {
        renderer->original_bitmap = previous;
    }

    if (renderer->bitmap) {
        DeleteObject(renderer->bitmap);
    }

    renderer->bitmap = bitmap;
    renderer->framebuffer = (ui_framebuffer){pixels, width, height};

    return true;
}

bool ui_renderer_begin(ui_renderer* renderer, u32 background) {
    renderer->drawing_failed = false;
    u32 width, height;
    if (!ui_window_client_size(renderer->window, &width, &height)) {
        return false;
    }

    if (!width || !height) {
        return true;
    }

    if ((renderer->framebuffer.width != width || renderer->framebuffer.height != height) &&
        !ui_renderer_resize_canvas(renderer, width, height)) {
        return false;
    }

    ui_renderer_rect(renderer, 0, 0, width, height, background);

    return true;
}

void ui_renderer_rect(ui_renderer* renderer, i32 x, i32 y, u32 width, u32 height, u32 color) {
    i64 right = (i64)x + width, bottom = (i64)y + height;
    i64 canvas_width = (i64)renderer->framebuffer.width;
    i64 canvas_height = (i64)renderer->framebuffer.height;
    if (x < 0) {
        x = 0;
    }

    if (y < 0) {
        y = 0;
    }

    if (right > canvas_width) {
        right = canvas_width;
    }

    if (bottom > canvas_height) {
        bottom = canvas_height;
    }

    if (right <= x || bottom <= y) {
        return;
    }

    RECT rect = {x, y, (LONG)right, (LONG)bottom};
    SetDCBrushColor(renderer->canvas, RGB((color >> 16) & 255, (color >> 8) & 255, color & 255));
    if (!FillRect(renderer->canvas, &rect, GetStockObject(DC_BRUSH))) {
        renderer->drawing_failed = true;
    }
}

void ui_renderer_text(ui_renderer* renderer, i32 x, i32 y, const char* text, u32 color) {
    SetTextColor(renderer->canvas, RGB((color >> 16) & 255, (color >> 8) & 255, color & 255));
    while (*text && y < (i64)renderer->framebuffer.height) {
        const char* end = strchr(text, '\n');
        size_t length = end ? (size_t)(end - text) : strlen(text);
        if (length <= INT_MAX) {
            if (!TextOutA(renderer->canvas, x, y, text, (int)length)) {
                renderer->drawing_failed = true;
            }
        } else {
            renderer->drawing_failed = true;
        }

        if (!end || (i64)y + renderer->cell_height > INT_MAX) {
            break;
        }

        text = end + 1;
        y += (i32)renderer->cell_height;
    }
}

bool ui_renderer_present(ui_renderer* renderer) {
    if (renderer->drawing_failed) {
        return false;
    }

    u32 width, height;
    if (!ui_window_client_size(renderer->window, &width, &height)) {
        return false;
    }

    if (!width || !height || !renderer->bitmap) {
        return true;
    }

    if (!GdiFlush()) { /* Finish GDI writes before OpenGL reads the DIB memory. */

        return false;
    }

    if (renderer->gpu) {
        if (MoveWindow(renderer->surface.handle, 0, 0, (int)width, (int)height, FALSE) &&
            ui_gpu_present_framebuffer(renderer->gpu, &renderer->framebuffer)) {
            return true;
        }

        ui_renderer_release_gpu(renderer);
    }

    return ui_framebuffer_present(renderer->window, &renderer->framebuffer);
}
