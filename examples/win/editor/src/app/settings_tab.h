/*
 ** settings_tab.h - 3 functions.
 ** editor_switch_settings - Switch representations of the same settings tab.
 ** editor_apply_settings_json - Validate and save edited settings.
 ** editor_settings_tab - Activate or create the singleton settings tab.
 ** See also: settings_tab.c, event_dispatch.c.
 */
#pragma once
#include "editor_view.h"

bool editor_settings_tab(editor_view* view, editor_document* document, editor_settings* settings);
bool editor_switch_settings(editor_view* view, editor_document* doc, editor_settings* settings, size_t index);
bool editor_apply_settings_json(editor_view* view, editor_document* doc, editor_settings* settings);
