#pragma once
#include "../../compiler/libs/sys/builtin/types_intrinsic.h"
#include <stdbool.h>

/* Library fallback; applications pass their own target to window creation. */
#define UI_DEFAULT_TARGET_FPS 240

/* Opaque backend resources. Destroy renderers before their window. */
typedef struct ui_window ui_window;
typedef struct ui_gpu ui_gpu;

/* target_fps: 1..1000; zero selects UI_DEFAULT_TARGET_FPS. */
ui_window* ui_window_create_fullscreen(const char* title, u32 target_fps);
void ui_window_destroy(ui_window* window);
/* Pumps input; returns zero on a close request. */
bool ui_window_poll_events(void);
bool ui_window_is_visible(const ui_window* window);
/* Waits for the next frame deadline, accounting for time spent rendering. */
bool ui_window_wait_frame(ui_window* window);
void ui_sleep_ms(u32 milliseconds);
u64 ui_time_ms(void);
i32 ui_report_error(const char* message);

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
