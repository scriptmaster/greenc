/*
 ** scroll_benchmark.c - 4 functions.
 ** milliseconds - Convert the Windows performance counter to milliseconds.
 ** time_scroll - Measure complete wheel-input and render frames.
 ** time_jumps - Measure random caret jumps and visible-row lookup.
 ** main - Open the supplied file, time its first frame, and report the results.
 ** See also: row_index.c, document_viewport.c, editor_frame.c.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "view_internal.h"
#include <assert.h>

static f64 milliseconds(void) {
    LARGE_INTEGER now, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&now);

    return 1000.0 * now.QuadPart / frequency.QuadPart;
}

static f64 time_scroll(editor_view* view, editor_document* doc, ui_renderer* renderer, ui_window* window,
                       editor_settings* settings, f64* worst) {
    ui_input wheel = {.mouse = UI_MOUSE_WHEEL, .x = 220, .y = 150, .wheel_delta = -120};
    f64 start = milliseconds();
    for (u32 i = 0; i < 200; ++i) {
        f64 frame = milliseconds();
        assert(editor_scroll_input(view, doc, renderer, window, settings, wheel));
        assert(editor_draw(renderer, window, doc, view, settings));
        f64 elapsed = milliseconds() - frame;
        if (elapsed > *worst) { *worst = elapsed; }
    }

    assert(view->first_line == 600);

    return (milliseconds() - start) / 200;
}

static f64 time_jumps(editor_view* view, editor_document* doc, ui_renderer* renderer, ui_window* window,
                      editor_settings* settings) {
    u32 random = 17;
    f64 start = milliseconds();
    for (u32 i = 0; i < 10000; ++i) {
        random = random * 1664525u + 1013904223u;
        doc->cursor = (size_t)(((f64)random / UINT32_MAX) * doc->length);
        editor_viewport viewport = {0};
        assert(editor_measure_viewport(renderer, window, doc, settings, view, &viewport));
        assert(viewport.map[viewport.cursor_line].offset + viewport.cursor_column == doc->cursor);
    }

    return (milliseconds() - start) / 10000;
}

int main(int argc, char** argv) {
    if (argc != 2) { return 2; }

    editor_document doc = {0};
    editor_view view = {0};
    editor_settings settings = editor_settings_init();
    settings.window.mode = UI_WINDOW_WINDOWED;
    settings.renderer.preference = UI_RENDERER_CPU;
    settings.path = argv[1];
    ui_window* window = ui_window_create(&settings.window);
    assert(window);
    ui_renderer* renderer = ui_renderer_create(window, &settings.renderer);
    assert(renderer);

    f64 start = milliseconds();
    assert(editor_load_existing(&doc, argv[1]));
    f64 loaded = milliseconds();
    assert(editor_tabs_open(&view.tabs, &doc, &settings, argv[1]));
    assert(editor_draw(renderer, window, &doc, &view, &settings));
    f64 painted = milliseconds();
    assert(view.mapped_rows > 1000);

    f64 worst = 0;
    f64 average = time_scroll(&view, &doc, renderer, window, &settings, &worst);
    f64 jumps = time_jumps(&view, &doc, renderer, window, &settings);
    printf("%zu bytes, %zu rows: load %.2f ms, first index+frame %.2f ms, wheel+frame mean %.3f ms max %.3f ms, cursor lookup %.3f us\n",
           doc.length, view.mapped_rows, loaded - start, painted - loaded, average, worst, jumps * 1000);

    free(view.row_map);
    editor_document_release(&doc);
    ui_renderer_destroy(renderer);
    ui_window_destroy(window);

    return 0;
}
