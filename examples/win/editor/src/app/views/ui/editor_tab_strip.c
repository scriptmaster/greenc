/*
 ** editor_tab_strip.c - 3 functions.
 ** editor_strip_layout - Measure the shared tab strip and clamp its scroll position.
 ** editor_strip_click - Resolve a strip click into an action and tab index.
 ** editor_strip_draw - Paint scroll arrows, visible tabs and trailing actions.
 ** See also: editor_tab_strip.h, editor_icons.h.
 */
#include "editor_tab_strip.h"
#include "editor_icons.h"
#include <stdio.h>
#include <string.h>

editor_tab_strip editor_strip_layout(editor_tab_scroll* scroll, size_t count, size_t active,
                                    u32 width, u32 cell, bool picker) {
    u32 buttons = picker ? 4 : 3;
    u32 button = width / (buttons + 1);
    if (button > 22) { button = 22; }

    u32 space = width - buttons * button;
    u32 tab = 16 * cell;
    if (tab > space) { tab = space; }

    size_t capacity = tab ? space / tab : 0;
    size_t visible = capacity < count ? capacity : count;
    size_t last_first = count - visible;
    if (scroll->reveal_active && visible) {
        if (active < scroll->first) { scroll->first = active; }
        if (active >= scroll->first + visible) { scroll->first = active - visible + 1; }
        scroll->reveal_active = false;
    }

    if (scroll->first > last_first) { scroll->first = last_first; }

    return (editor_tab_strip){width, button, tab, 2 * button + (u32)visible * tab,
                              scroll->first, visible, last_first, picker};
}

editor_tab_action editor_strip_click(const editor_tab_strip* strip, editor_tab_scroll* scroll, i32 x, size_t* index) {
    if (x < 0 || (u32)x >= strip->width) { return TAB_NONE; }

    u32 pos = (u32)x;
    if (pos < 2 * strip->button) {
        if (pos < strip->button && scroll->first) { --scroll->first; }
        if (pos >= strip->button && scroll->first < strip->last_first) { ++scroll->first; }

        return TAB_NONE;
    }

    if (pos < strip->plus_x) {
        *index = strip->first + (pos - 2 * strip->button) / strip->tab;

        u32 within = (pos - 2 * strip->button) % strip->tab;
        if (strip->tab >= 32 && within >= strip->tab - 20) { return TAB_CLOSE; }

        return TAB_SELECT;
    }

    if (pos < strip->plus_x + strip->button) { return TAB_ADD; }
    if (strip->picker && pos < strip->plus_x + 2 * strip->button) { return TAB_PICKER; }

    return TAB_NONE;
}

void editor_strip_draw(const editor_tab_strip* strip, ui_renderer* renderer, const editor_settings* settings,
                       i32 left, i32 top, u32 height, size_t active, const editor_tab_label* labels) {
    const editor_theme* theme = &settings->theme; ///read
    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    i32 middle = top + (i32)height / 2;
    ui_renderer_rect(renderer, left, top, strip->width, height, theme->panel);
    if (strip->button >= 12) {
        editor_triangle(renderer, left + strip->button / 2, middle, 0.5f, 0.75f,
                        strip->first ? theme->text : theme->muted, theme->panel);
        editor_triangle(renderer, left + strip->button + strip->button / 2, middle, 0, 0.75f,
                        strip->first < strip->last_first ? theme->text : theme->muted, theme->panel);
        editor_plus(renderer, left + strip->plus_x + strip->button / 2, middle, theme->heading, theme->panel);
        if (strip->picker) {
            editor_triangle(renderer, left + strip->plus_x + strip->button + strip->button / 2, middle,
                            0.25f, 0.75f, theme->heading, theme->panel);
        }
    }

    for (size_t slot = 0; slot < strip->visible; ++slot) {
        size_t index = strip->first + slot;
        i32 x = left + (i32)(2 * strip->button + slot * strip->tab);
        if (index == active) {
            ui_renderer_rect(renderer, x, top, strip->tab, height, theme->background);
            if (height >= 2) { ui_renderer_rect(renderer, x, top + height - 2, strip->tab, 2, theme->caret); }
        }

        u32 padding = 12;
        if (strip->tab >= 32) {
            padding += 20;
            i32 center = x + (i32)strip->tab - 10;
            u32 ink = index == active ? theme->heading : theme->muted;
            for (i32 step = -3; step <= 3; ++step) {
                ui_renderer_rect(renderer, center + step, middle + step, 1, 1, ink);
                ui_renderer_rect(renderer, center - step, middle + step, 1, 1, ink);
            }
        }

        i32 chars = strip->tab > padding ? (i32)((strip->tab - padding) / cw) : 0;
        if (!chars) { continue; }

        char text[32];
        u32 dot = labels[index].dirty ? theme->caret : theme->muted;
        ui_renderer_rect(renderer, x + 7, middle - 1, 3, 3, dot);
        snprintf(text, sizeof(text), "%.*s", chars - 1, labels[index].name);
        ui_renderer_text(renderer, x + 6 + (i32)cw, middle - (i32)ch / 2, text, index == active ? theme->heading : theme->muted);
    }
}
