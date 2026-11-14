/*
 ** tree_panel.c - 4 functions.
 ** editor_tree_width - Clamp the preferred file panel width.
 ** editor_tree_divider - Test the draggable file panel boundary.
 ** editor_tree_resize - Update panel width during a captured drag.
 ** editor_tree_draw - Render folder rows, selection and inline names.
 ** See also: tree_panel.h, tree_internal.h.
 */
#include "tree_internal.h"

u32 editor_tree_width(const editor_tree* tree, u32 width) {
    if (!tree->visible) { return 0; }

    u32 preferred = tree->preferred_width ? tree->preferred_width : (width / 3 < 280 ? width / 3 : 280);
    u32 maximum = width > 160 ? width - 160 : width / 2;

    return preferred < maximum ? preferred : maximum;
}

bool editor_tree_divider(const editor_tree* tree, u32 width, i32 bottom, i32 x, i32 y) {
    i32 divider = (i32)(width - editor_tree_width(tree, width));

    return tree->visible && y >= 0 && y < bottom && x >= divider - 4 && x <= divider + 4;
}

bool editor_tree_resize(editor_tree* tree, u32 width, i32 bottom, ui_input input) {
    i32 divider = (i32)(width - editor_tree_width(tree, width));
    if (!tree->resizing) {
        if (input.mouse != UI_MOUSE_DOWN || !editor_tree_divider(tree, width, bottom, input.x, input.y)) {
            return false;
        }

        tree->grip_offset = input.x - divider;
        tree->resizing = true;
    }

    i32 requested = (i32)width - input.x + tree->grip_offset;
    tree->preferred_width = requested > 96 ? (u32)requested : 96;
    tree->preferred_width = editor_tree_width(tree, width);
    if (input.mouse == UI_MOUSE_UP) { tree->resizing = false; }

    return true;
}

void editor_tree_draw(editor_tree* tree, ui_renderer* renderer, u32 width, u32 bottom,
                      const editor_settings* settings) {
    if (!tree->visible) { return; }

    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    u32 panel = editor_tree_width(tree, width);
    i32 x = (i32)(width - panel);
    ui_renderer_rect(renderer, x, 0, panel, bottom, settings->theme.panel);
    ui_renderer_rect(renderer, x, 0, 1, bottom, settings->theme.muted);

    ui_renderer_text(renderer, x + 8, 0, tree->naming ? "Name (Enter / Esc)" : "Files", settings->theme.heading);
    ui_renderer_text(renderer, x + 8, ch, tree->naming ? tree->name : tree->root, settings->theme.muted);
    size_t visible = bottom > 2 * ch ? (bottom - 2 * ch) / ch : 0;
    size_t maximum = tree->count > visible ? tree->count - visible : 0;
    if (tree->scroll > maximum) { tree->scroll = maximum; }

    for (size_t i = 0; i < visible && tree->scroll + i < tree->count; ++i) {
        editor_tree_row* row = &tree->rows[tree->scroll + i]; ///read
        i32 left = x + 8 + (i32)(row->depth * cw);
        i32 top = (i32)((i + 2) * ch);
        u32 color = row->hidden ? settings->theme.muted : settings->theme.text;
        u32 background = settings->theme.panel;
        if (!_stricmp(row->path, tree->selected)) {
            background = settings->theme.selection;
            ui_renderer_rect(renderer, x + 1, top, panel - 1, ch, background);
        }

        if (row->folder && !row->empty) {
            editor_disclosure(renderer, left + 5, top + ch / 2, row->expanded, row->changed, color, background);
        }

        ui_renderer_text(renderer, left + 16, top, row->path + row->name_offset, color);
    }
}
