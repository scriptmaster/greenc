/*
 ** editor_settings.h - 3 functions.
 ** editor_settings_init - Construct the editor's default settings.
 ** editor_settings_parse - Apply command-line overrides and select the startup path.
 ** editor_next_frame_rate - Choose the next configured frame rate.
 ** See also: editor_settings.c.
 */
#pragma once
#include "ui/ui_window.h"

typedef struct {
    u32 primary, secondary, hover, press, hover_percent, press_percent, radius;
} editor_control_style;

typedef struct {
    editor_control_style button, link;
    u32 column_guide;
    struct { u32 string, number, function, comment, keyword, heading, enabled, c_enabled, markdown_enabled; } syntax;
    u32 background, panel, text, heading, muted, caret, selection, breakpoint, stopped;
} editor_theme;

typedef struct {
    u32 margin, header_height, footer_height, text_top;
    u32 status_left, status_inset;
    u32 caret_width, caret_blink_ms;
} editor_layout;

typedef struct {
    ui_window_config window;
    ui_renderer_config renderer;
    const char* shell;
    u32 terminal_height;
    const char* path;
    bool explicit_file;
    const char* launch_folder;
    editor_theme theme;
    editor_layout layout;
    u32 frame_rates[4];
    u32 minimized_pause_ms;
} editor_settings;

editor_settings editor_settings_init(void);
bool editor_settings_parse(editor_settings* settings, int argc, char** argv);
u32 editor_next_frame_rate(const editor_settings* settings, u32 current);
