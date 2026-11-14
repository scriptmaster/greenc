/*
 ** editor_themes.h - 4 functions.
 ** editor_theme_load - Validate a theme file before applying it.
 ** editor_theme_save - Serialize theme values using atomic replacement.
 ** editor_themes_draw - Render the theme navigation, actions and editable fields.
 ** editor_themes_input - Apply settings navigation, edits and theme file commands.
 ** See also: theme_files.c, theme_page.c, theme_input.c, editor_control.h.
 */
#pragma once
#include "editor_control.h"

typedef struct {
    bool open, expanded, dirty, debug;
    char debug_path[260];
    bool debug_save_failed;
    u64 changed;
    i32 top;
    u32 bottom;
    i32 first, pointer_x, pointer_y;
    editor_control_input controls;
    editor_transition transitions[80];
    bool explorer_enabled, general;
    const char* explorer_status;
    bool keybindings;
    i32 group, first_keybinding;
    char path[260];
    const char* status;
} editor_themes;

bool editor_theme_load(editor_theme* theme, const char* path);
bool editor_theme_save(const editor_theme* theme, const char* path);
void editor_themes_draw(editor_themes* page, ui_renderer* renderer, u32 width, u32 height, const editor_theme* theme);
void editor_themes_input(editor_themes* page, ui_window* window, ui_input input, editor_theme* theme);
