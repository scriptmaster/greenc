/*
 ** editor_status.c - 5 functions.
 ** sample_usage - Refresh working-set and normalized CPU utilization at one-second intervals.
 ** editor_marquee_offset - Compute the path offset with a pause at each end.
 ** editor_status_hit - Test the footer's clickable FPS region.
 ** draw_path - Scroll the path label within its allotted width.
 ** editor_status_draw - Arrange the path, error, statistics and FPS control.
 ** See also: editor_status.h.
 */
#include "editor_status.h"
#include "file_size.h"
#include "modified_time.h"
#include <stdio.h>
#include <string.h>

u32 editor_marquee_offset(u32 overflow, u64 elapsed_ms) {
    if (!overflow) { return 0; }

    /* Pause at both ends so the directory and basename can each be read. */
    u64 travel = (u64)overflow * 32;
    u64 half = travel + 1200;
    u64 phase = elapsed_ms % (2 * half);
    bool returning = phase >= half;
    phase %= half;
    u32 distance = phase <= 1200 ? 0 : (u32)((phase - 1200) / 32);

    return returning ? overflow - distance : distance;
}

bool editor_status_hit(const editor_status_bar* bar, i32 x, i32 y) {
    return x >= bar->fps_left && x < bar->fps_right && y >= bar->top && y < bar->bottom;
}

static void draw_path(editor_status_bar* bar, ui_renderer* renderer, const editor_settings* settings,
                      bool dirty, i32 left, i32 right, i32 y, u32 cell) {
    u32 available = right > left ? (u32)(right - left) : 0;
    if (!available) { return; }

    u64 now = ui_time_ms();
    if (strcmp(settings->path, bar->path) || available != bar->path_width) {
        snprintf(bar->path, sizeof(bar->path), "%s", settings->path);
        bar->started = now;
        bar->path_width = available;
    }

    char label[SYS_PATH_CAPACITY + 3];
    snprintf(label, sizeof(label), "%s%s", bar->path, dirty ? " *" : "");
    u32 pixels = (u32)strlen(label) * cell;
    u32 offset = editor_marquee_offset(pixels > available ? pixels - available : 0, now - bar->started);
    ui_renderer_text(renderer, left - (i32)offset, y, label, settings->theme.muted);
}

static void sample_usage(editor_status_bar* bar) {
    u64 now = ui_time_ms();
    if (bar->sampled && now - bar->sampled < 1000) { return; }

    sys_process_stats usage;
    if (sys_process_sample(&usage)) {
        if (bar->sampled && usage.processors && now > bar->sampled) {
            bar->cpu_percent = (f64)(usage.cpu_100ns - bar->usage.cpu_100ns) /
                               (100.0 * (now - bar->sampled) * usage.processors);
        }

        bar->usage = usage;
    }

    bar->sampled = now;
}

void editor_status_draw(editor_status_bar* bar, ui_renderer* renderer, ui_window* window,
                        const editor_document* document, const editor_settings* settings,
                        u32 width, u32 height, size_t line, size_t column) {
    u32 cell, ch;
    ui_renderer_cell_size(renderer, &cell, &ch);
    i32 inset = (i32)settings->layout.status_left;
    i32 right = (i32)width - inset;
    sample_usage(bar);
    char fps[64], resources[112], details[100];
    u32 rate = ui_window_target_fps(window);
    if (ui_renderer_get_backend(renderer) == UI_BACKEND_CPU) {
        snprintf(fps, sizeof(fps), "CPU %.1f%%  %u FPS", bar->cpu_percent, rate);
    } else {
        snprintf(fps, sizeof(fps), "GPU %u FPS", rate);
    }

    char memory[32] = "unavailable";
    if (bar->usage.private_resident_bytes) {
        snprintf(memory, sizeof(memory), "%.1f MB", bar->usage.private_resident_bytes / 1048576.0);
    }

    snprintf(resources, sizeof(resources), "Mem %s | Ln %zu Col %zu | ", memory, line, column);
    char size[16];
    editor_format_size(size, document->modified[0] ? document->disk_bytes : document->length);
    char age[64];
    editor_modified_label(age, (time_t)document->modified_at, time(NULL));
    snprintf(details, sizeof(details), " | %s | %s%s", size,
             document->modified[0] ? "Modified: " : "", document->modified[0] ? age : "Unsaved");
    bar->top = (i32)(height - settings->layout.footer_height);
    bar->bottom = (i32)height;
    bar->fps_right = right;
    bar->fps_left = right - (i32)(strlen(fps) * cell);
    i32 stats_x = bar->fps_left - (i32)(strlen(resources) * cell);
    i32 meta_width = (i32)(strlen(details) * cell);
    i32 room = stats_x - inset - meta_width - (i32)cell;
    i32 path_width = (i32)((strlen(settings->path) + (document->dirty ? 2 : 0)) * cell);
    if (path_width > room) { path_width = room > 0 ? room : 0; }

    i32 meta_x = inset + path_width;
    i32 y = bar->top + (i32)settings->layout.status_inset;
    u32 footer = settings->layout.footer_height;
    ui_renderer_rect(renderer, 0, bar->top, width, footer, settings->theme.panel);
    draw_path(bar, renderer, settings, document->dirty, inset, meta_x, y, cell);
    ui_renderer_rect(renderer, 0, bar->top, inset, footer, settings->theme.panel);
    if ((u32)meta_x < width) { ui_renderer_rect(renderer, meta_x, bar->top, width - meta_x, footer, settings->theme.panel); }
    ui_renderer_text(renderer, meta_x, y, details, settings->theme.muted);
    i32 clear = stats_x > 0 ? stats_x : 0;
    ui_renderer_rect(renderer, clear, bar->top, width - clear, footer, settings->theme.panel);
    ui_renderer_text(renderer, stats_x, y, resources, settings->theme.muted);
    ui_renderer_text(renderer, bar->fps_left, y, fps, settings->theme.caret);
    i32 pointer_x, pointer_y;
    i32 modified_left = meta_x + (i32)((strlen(size) + 6) * cell);
    if (document->modified[0] && ui_window_pointer(window, &pointer_x, &pointer_y) &&
        pointer_y >= bar->top && pointer_y < bar->bottom && pointer_x >= modified_left &&
        pointer_x < meta_x + meta_width && pointer_x < stats_x) {
        u32 tip_width = (u32)strlen(document->modified) * cell + 12;
        i32 tip_x = modified_left;
        if (tip_x + (i32)tip_width > (i32)width) { tip_x = width > tip_width ? (i32)(width - tip_width) : 0; }
        i32 tip_y = bar->top - (i32)ch - 8;
        ui_renderer_rect(renderer, tip_x, tip_y, tip_width, ch + 8, settings->theme.panel);
        ui_renderer_text(renderer, tip_x + 6, tip_y + 4, document->modified, settings->theme.text);
    } else if (document->open_error) {
        ui_renderer_rect(renderer, 0, bar->top - (i32)ch, width, ch, settings->theme.panel);
        ui_renderer_text(renderer, inset, bar->top - (i32)ch, document->open_error, 0xff6666);
    } else if (document->status && strcmp(document->status, "Ready") && strcmp(document->status, "New file") &&
               strcmp(document->status, "Saved")) {
        ui_renderer_text(renderer, inset, bar->top - (i32)ch, document->status, settings->theme.heading);
    }
}
