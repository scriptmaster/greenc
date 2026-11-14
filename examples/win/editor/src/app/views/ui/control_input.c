/*
 ** control_input.c - 2 functions.
 ** editor_contains - Test whether a pointer lies inside a control.
 ** editor_control_activate - Activate on release inside the original pressed control.
 ** See also: editor_control.h, control_internal.h.
 */
#include "control_internal.h"

bool editor_contains(editor_bounds b, i32 x, i32 y) {
    return x >= b.x && y >= b.y && x < b.x + b.width && y < b.y + b.height;
}

bool editor_control_activate(editor_control_input* state, ui_input input, i32 target) {
    if (input.mouse == UI_MOUSE_DOWN) { state->pressed = target; }
    if (input.mouse != UI_MOUSE_UP) { return false; }

    bool activate = target && target == state->pressed;
    state->pressed = 0;

    return activate;
}
