/*
 ** terminal_panel.c - 3 functions.
 ** editor_terminal_size - Limit the terminal height to its available space.
 ** draw_console - Render visible console rows and the shell cursor.
 ** editor_terminal_draw - Compose the terminal strip, screen and profile picker.
 ** See also: terminal_panel.h, terminal_internal.h.
 */
#include "terminal_internal.h"

u32 editor_terminal_size(const editor_terminal* panel, u32 available) {
    if (!panel->visible) { return 0; }

    return panel->height < available / 2 ? panel->height : available / 2;
}

static void draw_console(editor_terminal* panel, ui_renderer* renderer, i32 top, u32 width, u32 height,
                         u32 cw, u32 ch, const editor_theme* theme) {
    char line[1025];
    u32 columns = width > 8 ? (width - 8) / cw : 0;
    if (columns >= sizeof(line)) { columns = sizeof(line) - 1; }

    sys_console* console = panel->sessions[panel->active].console;
    sys_console_view view;
    u32 rows = height / ch;
    if (!rows || !columns) { return; }
    if (!sys_console_running(console) || !sys_console_viewport(console, columns, rows, &view)) {
        ui_renderer_text(renderer, 4, top, sys_console_running(console) ? "Connecting to console..." : "Shell exited", theme->muted);

        return;
    }

    panel->sessions[panel->active].columns = columns;
    panel->sessions[panel->active].rows = rows;

    for (u32 row = 0; row < rows; ++row) {
        if (sys_console_row(console, view.left, view.top + row, line, columns)) {
            ui_renderer_text(renderer, 4, top + row * ch, line, theme->text);
        }
    }

    if (panel->focused && view.cursor_visible && (u32)view.cursor_x < columns && (u32)view.cursor_y < rows) {
        ui_renderer_rect(renderer, 4 + view.cursor_x * cw, top + view.cursor_y * ch, 2, ch, theme->caret);
    }
}

void editor_terminal_draw(editor_terminal* panel, ui_renderer* renderer, u32 width, u32 bottom,
                          const editor_settings* settings) {
    u32 height = editor_terminal_size(panel, bottom);
    if (!height) { return; }

    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    i32 top = (i32)(bottom - height);
    const editor_theme* theme = &settings->theme; ///read
    ui_renderer_rect(renderer, 0, top, width, height, theme->panel);
    ui_renderer_rect(renderer, 0, top, width, 1, theme->muted);

    u32 toolbar = DIVIDER_HEIGHT + settings->layout.header_height;
    if (height < toolbar) { return; }

    editor_tab_strip strip = editor_strip_layout(&panel->scroll, panel->count, panel->active, width, cw, true);
    editor_tab_label labels[EDITOR_TERMINAL_LIMIT];
    for (size_t i = 0; i < panel->count; ++i) {
        labels[i] = (editor_tab_label){.name = panel->sessions[i].name, .dirty = panel->sessions[i].changed};
    }

    editor_strip_draw(&strip, renderer, settings, 0, top + DIVIDER_HEIGHT, settings->layout.header_height, panel->active, labels);
    draw_console(panel, renderer, top + toolbar, width, height - toolbar, cw, ch, theme);
    if (panel->notice) {
        ui_renderer_text(renderer, 4, bottom - ch, panel->notice, theme->heading);
    }

    if (panel->choosing) {
        for (size_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
            i32 left = (i32)profile_menu_left(&strip, cw * PROFILE_CELLS);
            i32 y = top + toolbar + (i32)(i * ch);
            if (y + (i32)ch > (i32)bottom) { break; }
            ui_renderer_rect(renderer, left, y, cw * PROFILE_CELLS, ch, theme->selection);
            ui_renderer_text(renderer, left + 4, y, profiles[i], theme->heading);
        }
    }
}
