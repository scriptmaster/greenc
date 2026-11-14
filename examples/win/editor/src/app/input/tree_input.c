/*
 ** tree_input.c - 1 function.
 ** editor_tree_input - Dispatch tree scrolling, folder expansion and file selection.
 ** See also: tree_input.h, tree_internal.h.
 */
#include "tree_internal.h"

void editor_tree_input(editor_tree* tree, ui_renderer* renderer, ui_input input,
                       editor_document* document) {
    if (input.mouse == UI_MOUSE_DOWN) { editor_tree_select_at(tree, renderer, input.y); }
    if (input.mouse == UI_MOUSE_WHEEL) {
        if (input.wheel_delta < 0) { tree->scroll += 3; }
        if (input.wheel_delta > 0) { tree->scroll = tree->scroll > 3 ? tree->scroll - 3 : 0; }

        return;
    }

    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);
    if (input.mouse != UI_MOUSE_DOWN || input.y < (i32)(2 * ch)) { return; }

    size_t index = tree->scroll + (size_t)input.y / ch - 2;
    if (index >= tree->count) { return; }

    editor_tree_row* row = &tree->rows[index]; ///read
    if (!row->folder) {
        strcpy(tree->current_path, row->path);

        return;
    }

    row->changed = ui_time_ms();
    if (row->expanded) {
        size_t end = index + 1;
        while (end < tree->count && tree->rows[end].depth > row->depth) { ++end; }
        memmove(row + 1, tree->rows + end, (tree->count - end) * sizeof(*row));
        tree->count -= end - index - 1;
        row->expanded = false;
    } else if (!row->empty) {
        size_t before = tree->count;
        if (!insert_children(tree, index + 1, row->path, row->depth + 1)) {
            document->status = "Cannot list directory";
            return;
        }

        tree->rows[index].expanded = tree->count > before;
        tree->rows[index].empty = tree->count == before;
    }
}
