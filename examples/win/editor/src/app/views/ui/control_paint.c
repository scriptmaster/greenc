/*
 ** control_paint.c - 4 functions.
 ** blend - Mix two theme colors by percentage.
 ** animated_color - Interpolate a control's color over time.
 ** rounded_rect - Paint a rectangle with the requested corner radius.
 ** editor_control_draw - Render a themed button or link in its current pointer state.
 ** See also: editor_control.h, control_internal.h.
 */
#include "control_internal.h"

static u32 blend(u32 base, u32 overlay, u32 percent) {
    u32 result = 0;
    for (u32 shift = 0; shift < 24; shift += 8) {
        u32 a = (base >> shift) & 255, b = (overlay >> shift) & 255;
        result |= ((a * (100 - percent) + b * percent + 50) / 100) << shift;
    }

    return result;
}

static u32 animated_color(editor_transition* transition, u32 target) {
    u64 now = ui_time_ms();
    if (!transition->initialized) {
        *transition = (editor_transition){target, target, now, true};
    }

    u64 elapsed = now - transition->changed;
    u32 current = elapsed >= 100 ? transition->target : blend(transition->start, transition->target, (u32)elapsed);
    if (target != transition->target) {
        *transition = (editor_transition){current, target, now, true};
    }

    return current;
}

static void rounded_rect(ui_renderer* renderer, editor_bounds b, i32 radius, u32 color) {
    if (radius > b.height / 2) { radius = b.height / 2; }
    if (radius > b.width / 2) { radius = b.width / 2; }
    for (i32 y = 0; y < b.height; ++y) {
        i32 edge = y < b.height / 2 ? y : b.height - 1 - y;
        i32 inset = 0;
        if (edge < radius) {
            f32 dy = radius - edge - 0.5f;
            inset = (i32)(radius - sqrtf(radius * radius - dy * dy) + 0.5f);
        }

        ui_renderer_rect(renderer, b.x + inset, b.y + y, b.width - 2 * inset, 1, color);
    }
}

void editor_control_draw(ui_renderer* renderer, editor_bounds b, const char* label, editor_control_kind kind,
                         const editor_theme* theme, bool hover, bool pressed, editor_transition* transition) {
    bool link = kind == EDITOR_LINK || kind == EDITOR_SECONDARY_LINK;
    const editor_control_style* style = link ? &theme->link : &theme->button;
    u32 color = (kind == EDITOR_SECONDARY || kind == EDITOR_SECONDARY_LINK) ? style->secondary : style->primary;
    if (pressed && hover) {
        color = blend(color, style->press, style->press_percent);
    } else if (hover) {
        color = blend(color, style->hover, style->hover_percent);
    }

    color = animated_color(transition, color);
    if (!link) { rounded_rect(renderer, b, (i32)style->radius, color); }

    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    ui_renderer_text(renderer, b.x + 8, b.y + (b.height - (i32)ch) / 2, label, link ? color : theme->text);
}
