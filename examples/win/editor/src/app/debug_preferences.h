/*
 ** debug_preferences.h - 4 functions.
 ** preference_path - Locate a named preference in local application data.
 ** editor_theme_preference - Locate the per-user active theme.
 ** editor_debug_load - Restore the per-user debugger executable path.
 ** editor_debug_save - Store the selected debugger executable atomically.
 ** See also: debug_preferences.c, theme_input.c.
 */
#pragma once
#include <stdbool.h>

bool preference_path(char result[260], const char* filename);
void editor_debug_load(char result[260]);
bool editor_theme_preference(char result[260]);
bool editor_debug_save(const char* path);
