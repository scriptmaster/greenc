#pragma once
#include "../sys/builtin/types_intrinsic.h"
#include <stdbool.h>

/* Opaque backend resources. Destroy renderers before their window. */
typedef struct ui_window ui_window;
typedef struct ui_gpu ui_gpu;

typedef enum { UI_WINDOW_WINDOWED, UI_WINDOW_MAXIMIZED, UI_WINDOW_FULLSCREEN } ui_window_mode;

typedef struct {
    const char* title; /* Borrowed during creation only. */
    u32 width, height; /* Initial client size; required except in fullscreen. */
    u32 target_fps; /* 0: unpaced; 1..1000: application frame deadline. */
    ui_window_mode mode;
    bool title_bar, resizable, minimize_button, maximize_button, close_button;
} ui_window_config;

/* No application settings are selected by the library. Fullscreen is borderless.
 * Caption buttons require title_bar; maximize also requires resizable. */
ui_window* ui_window_create(const ui_window_config* config);
bool ui_window_set_target_fps(ui_window* window, u32 target_fps);
u32 ui_window_target_fps(const ui_window* window);
void ui_window_request_close(ui_window* window);
typedef enum { UI_CURSOR_ARROW, UI_CURSOR_RESIZE_EW, UI_CURSOR_RESIZE_NS } ui_cursor;
/* Client coordinates, including outside positions during pointer capture. */
bool ui_window_pointer(const ui_window* window, i32* x, i32* y);
void ui_window_set_cursor(ui_window* window, ui_cursor cursor);
bool ui_window_client_size(const ui_window* window, u32* width, u32* height);
typedef struct {
    const char* group;
    const char* label;
    u16 command;
} ui_menu_item;

/* Consecutive items in the same group share a native dropdown. */
bool ui_window_set_menu(ui_window* window, const ui_menu_item* items, size_t count);
void ui_window_destroy(ui_window* window);
/* Pumps input; false after an explicit ui_window_request_close. Native close
 * becomes an input event so programs can apply unsaved-document policy. */
bool ui_window_poll_events(void);
bool ui_window_is_visible(const ui_window* window);
/* Waits for the next frame deadline, accounting for time spent rendering. */
bool ui_window_wait_frame(ui_window* window);
void ui_sleep_ms(u32 milliseconds);
u64 ui_time_ms(void);
i32 ui_report_error(const char* message);
bool ui_confirm(const char* title, const char* message);

typedef enum {
    UI_KEY_NONE, UI_KEY_ESCAPE, UI_KEY_LEFT, UI_KEY_RIGHT, UI_KEY_UP, UI_KEY_DOWN,
    UI_KEY_HOME, UI_KEY_END, UI_KEY_DELETE,
    UI_KEY_A, UI_KEY_B, UI_KEY_C, UI_KEY_D, UI_KEY_E, UI_KEY_F, UI_KEY_G,
    UI_KEY_H, UI_KEY_I, UI_KEY_J, UI_KEY_K, UI_KEY_L, UI_KEY_M, UI_KEY_N,
    UI_KEY_O, UI_KEY_P, UI_KEY_Q, UI_KEY_R, UI_KEY_S, UI_KEY_T, UI_KEY_U,
    UI_KEY_V, UI_KEY_W, UI_KEY_X, UI_KEY_Y, UI_KEY_Z,
    UI_KEY_F1, UI_KEY_F2, UI_KEY_F3, UI_KEY_F4, UI_KEY_F5, UI_KEY_F6,
    UI_KEY_F7, UI_KEY_F8, UI_KEY_F9, UI_KEY_F10, UI_KEY_F11, UI_KEY_F12, UI_KEY_GRAVE, UI_KEY_PAGE_UP, UI_KEY_PAGE_DOWN
} ui_key;
typedef struct {
    ui_key key;
    u32 character; /* ASCII WM_CHAR input, including tab/newline/backspace. */
    u16 menu_command;
    bool control, shift;
    enum { UI_MOUSE_NONE, UI_MOUSE_DOWN, UI_MOUSE_DRAG, UI_MOUSE_UP, UI_MOUSE_WHEEL, UI_MOUSE_CONTEXT, UI_MOUSE_MIDDLE } mouse;
    i32 x, y, wheel_delta;
    bool close_requested; /* Not dropped when the character queue is full. */
} ui_input;
/* Drain after poll_events. Bounded queue: newest input is dropped on overflow.
 * All calls must run on the creating thread; one top-level window per loop. */
/* ASCII clipboard text. Read fails if the complete string cannot fit. */
bool ui_clipboard_write(ui_window* window, const char* text, size_t length);
bool ui_clipboard_read(ui_window* window, char* text, size_t capacity);
bool ui_window_next_input(ui_window* window, ui_input* input);

typedef struct {
    u32* pixels;
    size_t width;
    size_t height;
} ui_framebuffer;

/* Owned top-down pixels in 0x00RRGGBB format; initialize an empty value. */
bool ui_framebuffer_init(ui_framebuffer* framebuffer, size_t width, size_t height);
void ui_framebuffer_destroy(ui_framebuffer* framebuffer);
bool ui_framebuffer_present(ui_window* window, const ui_framebuffer* framebuffer);

ui_gpu* ui_gpu_create(ui_window* window);
void ui_gpu_destroy(ui_gpu* gpu);

typedef struct {
    f32 r, g, b, a;
} ui_color;

typedef struct {
    f32 x, y;
    ui_color color;
} ui_vertex2;

/* Clears, rotates caller-owned vertices about the origin, draws and presents.
 * Coordinates use equal units on both axes; [-1, 1] fits the shorter axis.
 * Colors use [0, 1]. Vertices are borrowed only for this call. */
bool ui_gpu_draw_triangle(ui_gpu* gpu, const ui_vertex2 vertices[3],
                          f32 angle_degrees, ui_color background);

/* Common canvas: CPU rasterization, optionally GPU-accelerated presentation.
 * AUTO tries hardware OpenGL and falls back to GDI, also on presentation failure.
 * CPU never attempts OpenGL. Renderer borrows window; destroy it first. */
typedef enum { UI_RENDERER_AUTO, UI_RENDERER_CPU } ui_renderer_preference;
typedef enum { UI_BACKEND_CPU, UI_BACKEND_GPU } ui_renderer_backend;
typedef struct ui_renderer ui_renderer;
typedef struct {
    ui_renderer_preference preference;
    bool antialias;
    const char* font_name; /* Borrowed during creation. Choose a monospaced face. */
    u32 cell_width, cell_height; /* Requested font dimensions in client pixels. */
} ui_renderer_config;

ui_renderer* ui_renderer_create(ui_window* window, const ui_renderer_config* config);
void ui_renderer_destroy(ui_renderer* renderer);
ui_renderer_backend ui_renderer_get_backend(const ui_renderer* renderer);
void ui_renderer_cell_size(const ui_renderer* renderer, u32* width, u32* height);
/* Begin resizes the owned canvas to the current client size and clears it.
 * Rectangles/text are opaque RGB; coordinates are client pixels, clipped. */
bool ui_renderer_begin(ui_renderer* renderer, u32 background);
void ui_renderer_rect(ui_renderer* renderer, i32 x, i32 y, u32 width, u32 height, u32 color);
/* Monospaced ASCII cells; query actual font metrics with cell_size.
 * Newlines advance one row; text is borrowed for the call. */
void ui_renderer_text(ui_renderer* renderer, i32 x, i32 y, const char* text, u32 color);
bool ui_renderer_present(ui_renderer* renderer);
