/*
 ** editor_icons.c - 4 functions.
 ** blend_coverage - Blend an icon pixel with its background using sample coverage.
 ** editor_triangle - Rasterize a rotated triangle with antialiased edges.
 ** editor_disclosure - Draw the animated expansion indicator.
 ** editor_plus - Paint an add button glyph.
 ** See also: editor_icons.h.
 */
#include "editor_icons.h"
#include <math.h>

static u32 blend_coverage(u32 foreground, u32 background, u32 coverage) {
    u32 color = 0;
    for (u32 shift = 0; shift <= 16; shift += 8) {
        u32 front = (foreground >> shift) & 255;
        u32 back = (background >> shift) & 255;
        color |= ((front * coverage + back * (16 - coverage) + 8) / 16) << shift;
    }

    return color;
}

void editor_triangle(ui_renderer* renderer, i32 x, i32 y, f32 turns, f32 scale, u32 color, u32 background) {
    f32 angle = turns * 6.2831853f;
    f32 c = cosf(angle), s = sinf(angle);
    for (i32 row = -6; row <= 6; ++row) {
        for (i32 col = -6; col <= 6; ++col) {
            u32 coverage = 0;
            for (i32 sample = 0; sample < 16; ++sample) {
                f32 dx = col + (sample % 4 - 1.5f) / 4;
                f32 dy = row + (sample / 4 - 1.5f) / 4;
                f32 along = dx * c + dy * s;
                f32 across = dy * c - dx * s;
                if (along >= -3 * scale && along + 1.6f * fabsf(across) <= 5 * scale) { ++coverage; }
            }

            if (coverage) {
                ui_renderer_rect(renderer, x + col, y + row, 1, 1, blend_coverage(color, background, coverage));
            }
        }
    }
}

void editor_disclosure(ui_renderer* renderer, i32 x, i32 y, bool expanded, u64 changed, u32 color, u32 background) {
    u64 elapsed = ui_time_ms() - changed;
    f32 progress = changed && elapsed < 140 ? (f32)elapsed / 140 : 1;
    editor_triangle(renderer, x, y, 0.25f * (expanded ? progress : 1 - progress), 1, color, background);
}

void editor_plus(ui_renderer* renderer, i32 x, i32 y, u32 color, u32 background) {
    for (i32 step = -4; step <= 4; ++step) {
        u32 ink = blend_coverage(color, background, step == -4 || step == 4 ? 8 : 16);
        ui_renderer_rect(renderer, x + step, y, 1, 1, ink);
        ui_renderer_rect(renderer, x, y + step, 1, 1, ink);
    }
}
