/*
 ** editor_settings.c - 4 functions.
 ** editor_settings_init - Construct the editor's default settings.
 ** editor_settings_are_valid - Validate application layout and rendering choices.
 ** editor_settings_parse - Apply command-line overrides and select the startup path.
 ** editor_next_frame_rate - Choose the next configured frame rate.
 ** See also: editor_settings.h.
 */
#include "editor_settings.h"
#include <stdlib.h>
#include <string.h>

editor_settings editor_settings_init(void) {
    editor_settings settings = {
        .window = {
            .title = "GreenC Editor", .width = 1100, .height = 720,
            .target_fps = 30, .mode = UI_WINDOW_MAXIMIZED,
            .title_bar = true, .resizable = true,
            .minimize_button = true, .maximize_button = true, .close_button = true
        },

        .renderer = {
            .preference = UI_RENDERER_CPU, .antialias = true, .font_name = "Consolas",
            .cell_width = 8, .cell_height = 16
        },

        .shell = "cmd.exe", .terminal_height = 220,
        .path = "notes.txt",
        .theme = {
            .column_guide = 100,
            .button = {.primary = 0x325779, .secondary = 0x344151, .hover = 0xffffff, .press = 0x000000,
                       .hover_percent = 18, .press_percent = 30, .radius = 5},
            .link = {.primary = 0x80c8f4, .secondary = 0xa4bddd, .hover = 0xffffff, .press = 0x59aee9,
                     .hover_percent = 40, .press_percent = 65},
            .syntax = {.string = 0xb4d996, .number = 0xe9b981, .function = 0x52c7be, .comment = 0x737982,
                       .keyword = 0xbfa4ed, .heading = 0x8ecafb, .enabled = 1, .c_enabled = 1, .markdown_enabled = 1},
            .background = 0x151a22, .panel = 0x222c3a,
            .text = 0xd4deec, .heading = 0xe3ebf5,
            .muted = 0x9cadc2, .caret = 0x67d5ae, .breakpoint = 0x54252c, .stopped = 0x665219, .selection = 0x345477
        },

        .layout = {
            .margin = 24, .header_height = 34, .footer_height = 28,
            .text_top = 46,
            .status_left = 16, .status_inset = 6,
            .caret_width = 1, .caret_blink_ms = 500
        },

        .frame_rates = {30, 60, 144, 240},
        .minimized_pause_ms = 50
    };

    return settings;
}

static bool editor_settings_are_valid(const editor_settings* settings) {
    const editor_layout* layout = &settings->layout; ///read
    if (!layout->caret_blink_ms || !layout->caret_width || !layout->footer_height ||
        layout->margin > INT32_MAX / 2 || layout->text_top < layout->header_height ||
        (u64)layout->text_top + layout->footer_height > INT32_MAX ||
        layout->header_height < 2 ||
        layout->status_inset >= layout->footer_height) {
        return false;
    }

    for (size_t i = 0; i < sizeof(settings->frame_rates) / sizeof(settings->frame_rates[0]); ++i) {
        if (settings->frame_rates[i] > 1000) {
            return false;
        }
    }

    return true;
}

bool editor_settings_parse(editor_settings* settings, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--shell=", 8) == 0 && argv[i][8]) {
            settings->shell = argv[i] + 8;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            settings->renderer.preference = UI_RENDERER_CPU;
        } else if (strcmp(argv[i], "--gpu") == 0) {
            settings->renderer.preference = UI_RENDERER_AUTO;
        } else if (strcmp(argv[i], "--windowed") == 0) {
            settings->window.mode = UI_WINDOW_WINDOWED;
        } else if (strcmp(argv[i], "--fullscreen") == 0) {
            settings->window.mode = UI_WINDOW_FULLSCREEN;
        } else if (strcmp(argv[i], "--borderless") == 0) {
            settings->window.mode = UI_WINDOW_WINDOWED;
            settings->window.title_bar = false;
            settings->window.minimize_button = false;
            settings->window.maximize_button = false;
        } else if (strncmp(argv[i], "--fps=", 6) == 0) {
            char* end;
            unsigned long fps = strtoul(argv[i] + 6, &end, 10);
            if (!argv[i][6] || *end || fps > 1000) {
                return false;
            }

            settings->window.target_fps = (u32)fps;
        } else if (!strcmp(argv[i], "--folder") && i + 1 < argc) {
            settings->launch_folder = argv[++i];
        } else if (strncmp(argv[i], "--", 2) == 0) {
            return false;
        } else {
            settings->path = argv[i];
            settings->explicit_file = true;
        }
    }

    return editor_settings_are_valid(settings);
}

u32 editor_next_frame_rate(const editor_settings* settings, u32 current) {
    const size_t count = sizeof(settings->frame_rates) / sizeof(settings->frame_rates[0]);
    for (size_t i = 0; i < count; ++i) {
        if (settings->frame_rates[i] == current) {
            return settings->frame_rates[(i + 1) % count];
        }
    }

    return settings->frame_rates[0];
}
