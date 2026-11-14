/*
 ** themes_internal.h - 5 functions.
 ** theme_value - Locate a configurable value in the theme.
 ** visible_rows - Count theme fields that fit in the settings pane.
 ** scroll_fields - Clamp the first visible theme field.
 ** control_bounds - Compute a settings control's hit and draw rectangle.
 ** control_visible - Determine whether a settings control is displayed.
 ** See also: theme_files.c, theme_layout.c, editor_themes.h, editor_icons.h.
 */
#pragma once
#include "editor_themes.h"
#include "editor_icons.h"
#include "ui/ui_dialog.h"
#include "sys/fs/write_atomic.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char* name; size_t offset; u32 maximum; } editor_theme_field;
extern const editor_theme_field fields[32];
enum { FIELD_COUNT = sizeof(fields) / sizeof(fields[0]), SIDEBAR = 144, LEFT = 160, ROW_TOP = 148, ROW_HEIGHT = 32 };
enum { CATEGORY = 1, THEMES, DONE, BROWSE, SAVE, PREVIOUS, NEXT, DEBUGGER, KEYS, GROUP, GENERAL, EXPLORER, INSTALL_ADMIN, REMOVE_ADMIN, FIRST_FIELD };
enum { KEYBINDING_ROWS = 13, KEYBINDING_HEIGHT = 22 };
extern const char* const theme_groups[6];
extern const i32 group_edges[7];

u32* theme_value(editor_theme* theme, size_t index);
i32 visible_rows(u32 height);
void scroll_fields(editor_themes* page, i32 rows, i32 step);
editor_bounds control_bounds(i32 id, i32 first);
bool control_visible(const editor_themes* page, i32 id, i32 rows);
