/*
 ** editor_tab_strip.h - 3 functions.
 ** editor_strip_layout - Measure the shared tab strip and clamp its scroll position.
 ** editor_strip_click - Resolve a strip click into an action and tab index.
 ** editor_strip_draw - Paint scroll arrows, visible tabs and trailing actions.
 ** See also: editor_tab_strip.c, editor_settings.h.
 */
#pragma once
#include "editor_settings.h"

typedef struct {
    size_t first;
    bool reveal_active;
} editor_tab_scroll;

typedef struct {
    u32 width, button, tab, plus_x;
    size_t first, visible, last_first;
    bool picker;
} editor_tab_strip;

typedef enum { TAB_NONE, TAB_SELECT, TAB_CLOSE, TAB_ADD, TAB_PICKER } editor_tab_action;
typedef struct { const char* name; bool dirty; } editor_tab_label;

editor_tab_strip editor_strip_layout(editor_tab_scroll* scroll, size_t count, size_t active,
                                    u32 width, u32 cell, bool picker);
editor_tab_action editor_strip_click(const editor_tab_strip* strip, editor_tab_scroll* scroll, i32 x, size_t* index);
void editor_strip_draw(const editor_tab_strip* strip, ui_renderer* renderer, const editor_settings* settings,
                       i32 left, i32 top, u32 height, size_t active, const editor_tab_label* labels);
