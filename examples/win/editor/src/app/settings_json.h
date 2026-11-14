/*
 ** settings_json.h - 2 functions.
 ** editor_settings_json_format - Serialize editable theme and debugger settings.
 ** editor_settings_json_parse - Validate a settings object before applying its values.
 ** See also: settings_json.c, settings_tab.c.
 */
#pragma once
#include "editor_settings.h"

bool editor_settings_json_format(const editor_theme* theme, const char* debugger, char* output, size_t capacity);
bool editor_settings_json_parse(const char* json, editor_theme* theme, char debugger[260]);
