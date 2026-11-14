#include "../../../compiler/libs/ui/ui_window.h"

int main(void) {
    /* Program setting: change to 128, 144, or another desired frame rate. */
    const u32 target_fps = 240;

    /* Equilateral, centered at its centroid: x = sqrt(3) * radius / 2. */
    const ui_vertex2 triangle[3] = {
        { 0.0f,        0.7f,  {1.0f, 0.2f, 0.3f, 1.0f}},
        {-0.60621778f, -0.35f, {0.2f, 1.0f, 0.4f, 1.0f}},
        { 0.60621778f, -0.35f, {0.2f, 0.4f, 1.0f, 1.0f}}
    };
    const ui_color background = {0.025f, 0.04f, 0.08f, 1.0f};

    const ui_window_config config = {
        .title = "GreenC GPU interface - Escape to exit",
        .target_fps = target_fps,
        .mode = UI_WINDOW_FULLSCREEN,
        .close_button = true
    };
    ui_window* window = ui_window_create(&config);
    if (!window) {
        return ui_report_error("Cannot create the GPU window.");
    }

    ui_gpu* gpu = ui_gpu_create(window);
    if (!gpu) {
        ui_window_destroy(window);

        return ui_report_error("Cannot initialize hardware graphics. Check the GPU driver.");
    }

    u64 start = ui_time_ms();
    i32 result = 0;

    while (ui_window_poll_events()) {
        ui_input input;
        while (ui_window_next_input(window, &input)) {
            if (input.close_requested || input.key == UI_KEY_ESCAPE) {
                ui_window_request_close(window);
            }
        }

        if (!ui_window_is_visible(window)) {
            ui_sleep_ms(50);

            continue;
        }

        f32 angle = (f32)((ui_time_ms() - start) % 12000) * 0.03f;
        if (!ui_gpu_draw_triangle(gpu, triangle, angle, background)) {
            result = 1;

            break;
        }

        if (!ui_window_wait_frame(window)) {
            result = 1;

            break;
        }
    }

    ui_gpu_destroy(gpu);
    ui_window_destroy(window);

    return result ? ui_report_error("GPU rendering or frame pacing failed.") : 0;
}
