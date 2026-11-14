/*
 ** editor_control.h - 3 functions.
 ** editor_contains - Test whether a pointer lies inside a control.
 ** editor_control_activate - Activate on release inside the original pressed control.
 ** editor_control_draw - Render a themed button or link in its current pointer state.
 ** See also: control_input.c, control_paint.c, editor_settings.h.
 */
#pragma once
#include "editor_settings.h"

typedef struct { i32 x, y, width, height; } editor_bounds;
typedef struct { u32 start, target; u64 changed; bool initialized; } editor_transition;
typedef struct { i32 pressed; } editor_control_input;
typedef enum { EDITOR_PRIMARY, EDITOR_SECONDARY, EDITOR_LINK, EDITOR_SECONDARY_LINK } editor_control_kind;

bool editor_contains(editor_bounds bounds, i32 x, i32 y);
bool editor_control_activate(editor_control_input* state, ui_input input, i32 target);
void editor_control_draw(ui_renderer* renderer, editor_bounds bounds, const char* label, editor_control_kind kind,
                         const editor_theme* theme, bool hover, bool pressed, editor_transition* transition);
