/*
 ** terminal_toolbar.c - 2 functions.
 ** profile_menu_left - Anchor the profile picker within the panel.
 ** editor_terminal_click - Dispatch terminal tab and profile choices.
 ** See also: terminal_internal.h, terminal_toolbar.h.
 */
#include "terminal_internal.h"

u32 profile_menu_left(const editor_tab_strip* strip, u32 menu_width) {
    u32 anchor = strip->plus_x + strip->button;
    u32 maximum = strip->width > menu_width ? strip->width - menu_width : 0;

    return anchor < maximum ? anchor : maximum;
}

void editor_terminal_click(editor_terminal* panel, ui_renderer* renderer, u32 width, i32 x, i32 y,
                           const editor_settings* settings) {
    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    if (x < 0 || y < DIVIDER_HEIGHT) { return; }

    editor_tab_strip strip = editor_strip_layout(&panel->scroll, panel->count, panel->active, width, cw, true);
    u32 toolbar = DIVIDER_HEIGHT + settings->layout.header_height;
    if ((u32)y < toolbar) {
        size_t index;
        editor_tab_action action = editor_strip_click(&strip, &panel->scroll, x, &index);
        if (action == TAB_PICKER) {
            panel->choosing = !panel->choosing;

            return;
        }

        if (action == TAB_SELECT) {
            panel->active = index;
            panel->notice = NULL;
        } else if (action == TAB_CLOSE) {
            close_session(panel, index);
        } else if (action == TAB_ADD) {
            new_session(panel, settings->shell);
        }
    } else if (panel->choosing) {
        u32 left = profile_menu_left(&strip, cw * PROFILE_CELLS);
        size_t choice = ((u32)y - toolbar) / ch;
        if ((u32)x >= left && (u32)x < left + cw * PROFILE_CELLS && choice < sizeof(profiles) / sizeof(profiles[0])) {
            new_session(panel, profiles[choice]);
        }
    }

    panel->choosing = false;
}
