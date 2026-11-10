#include "../ui_window.h"

int main(void) {
    /* Program setting: change to 128, 144, or another desired frame rate. */
    const u32 target_fps = 240;

    ui_window* window = ui_window_create_fullscreen("GreenC CPU interface - Escape to exit", target_fps);
    if (!window) {
        return ui_report_error("Cannot create the CPU window.");
    }

    ui_framebuffer framebuffer = {0};
    if (!ui_framebuffer_init(&framebuffer, 640, 360)) {
        ui_window_destroy(window);

        return ui_report_error("Cannot allocate the CPU framebuffer.");
    }

    u64 start = ui_time_ms();
    i32 result = 0;

    while (ui_window_poll_events()) {
        if (!ui_window_is_visible(window)) {
            ui_sleep_ms(50);

            continue;
        }

        u32 phase = (u32)(((ui_time_ms() - start) / 16) % 256);

        for (size_t y = 0; y < framebuffer.height; ++y) {
            for (size_t x = 0; x < framebuffer.width; ++x) {
                u8 r = (u8)((x + phase) & 255);
                u8 g = (u8)((u64)y * 255 / framebuffer.height);
                u8 b = ((x / 32 ^ y / 32) & 1) ? 160 : 48;
                framebuffer.pixels[y * framebuffer.width + x] = ((u32)r << 16) | ((u32)g << 8) | (u32)b;
            }
        }

        if (!ui_framebuffer_present(window, &framebuffer)) {
            result = 1;

            break;
        }

        if (!ui_window_wait_frame(window)) {
            result = 1;

            break;
        }
    }

    ui_framebuffer_destroy(&framebuffer);
    ui_window_destroy(window);

    return result ? ui_report_error("CPU rendering or frame pacing failed.") : 0;
}
