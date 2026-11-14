/*
 ** document_rows.c - 3 functions.
 ** editor_fold_label - Place the expansion link within the visible text columns.
 ** editor_draw_document - Paint document rows, line numbers, folds and selections.
 ** editor_draw_caret - Paint the caret during its visible blink phase.
 ** See also: view_internal.h.
 */
#include "view_internal.h"
#include "syntax.h"
#include "byte_text.h"

i32 editor_fold_label(const editor_viewport* viewport, const editor_view* view, size_t row, char label[48]) {
    const editor_line_entry* entry = &viewport->map[row];
    snprintf(label, 48, "... %zu more line%s", entry->hidden, entry->hidden == 1 ? "" : "s");
    size_t cells = strlen(label);
    if (cells > viewport->columns) { return -1; }
    size_t length = entry->end - entry->offset;
    size_t column = length > view->first_column ? length - view->first_column + 1 : 1;
    if (column > viewport->columns - cells) { column = viewport->columns - cells; }

    return (i32)(viewport->text_left + column * viewport->cell_width);
}

void editor_draw_document(ui_renderer* renderer, const editor_document* document,
                                 const editor_view* view, const editor_viewport* viewport,
                                 const editor_settings* settings) {
    for (size_t row = 0; row < viewport->rows && view->first_line + row < viewport->visible_lines; ++row) {
        size_t index = view->first_line + row;
        size_t offset = viewport->map[index].offset;
        size_t end = viewport->map[index].end;
        size_t length = end - offset;
        size_t skip = view->first_column < length ? view->first_column : length;
        size_t count = length - skip;
        if (count > viewport->columns) {
            count = viewport->columns;
        }

        i32 y = (i32)settings->layout.text_top + (i32)(row * viewport->cell_height);
        bool stopped = view->debug.paused && view->debug.line == viewport->map[index].logical + 1 &&
                       !_stricmp(view->debug.source, settings->path);

        if (stopped || (document->folded && (document->folded[offset] & 2))) {
            ui_renderer_rect(renderer, 0, y, viewport->width, viewport->cell_height,
                             stopped ? settings->theme.stopped : settings->theme.breakpoint);
        }

        char number[24];
        snprintf(number, sizeof(number), "%4zu", viewport->map[index].logical + 1);
        u32 number_color = index == viewport->cursor_line ? settings->theme.caret : settings->theme.muted;

        if (viewport->map[index].has_unsupported_byte) { number_color = 0xff6666; }

        ui_renderer_text(renderer, (i32)settings->layout.margin, y, number, number_color);

        if (viewport->map[index].foldable) {
            editor_disclosure(renderer, viewport->text_left - 12, y + viewport->cell_height / 2,
                              !(document->folded && (document->folded[offset] & 1)), document->animated_fold == offset ? document->fold_changed : 0,
                              settings->theme.muted, settings->theme.background);
        }

        size_t start, finish;
        editor_selection(document, &start, &finish);
        size_t visible = offset + view->first_column;
        size_t last = end + (end < document->length);
        if (start < visible) { start = visible; }
        if (finish > last) { finish = last; }
        if (finish > visible + viewport->columns) { finish = visible + viewport->columns; }

        if (finish > start) {
            i32 x = (i32)(viewport->text_left + (start - visible) * viewport->cell_width);
            ui_renderer_rect(renderer, x, y, (u32)(finish - start) * viewport->cell_width,
                             viewport->cell_height, settings->theme.selection);
        }

        if (count) {
            editor_draw_syntax(renderer, settings, document->text + offset, length, viewport->map[index].in_comment, skip, count,
                               (i32)viewport->text_left, y, viewport->cell_width);
            if (document->byte_text) {
                editor_draw_byte_marks(renderer, document->text + offset + skip, count, viewport, y, settings->theme.muted);
            }
        }

        if (document->folded && (document->folded[offset] & 2)) {
            i32 cy = y + (i32)viewport->cell_height / 2;
            for (i32 dy = -4; dy <= 4; ++dy) {
                i32 half = abs(dy) == 4 ? 1 : abs(dy) == 3 ? 3 : 4;
                ui_renderer_rect(renderer, 10 - half, cy + dy, 2 * half + 1, 1, 0xd95b55);
            }
        }

        if (viewport->map[index].hidden) {
            char label[48];
            i32 left = editor_fold_label(viewport, view, index, label);
            if (left >= 0) {
                ui_renderer_rect(renderer, left, y, (u32)strlen(label) * viewport->cell_width, viewport->cell_height, settings->theme.background);
                ui_renderer_text(renderer, left, y, label, settings->theme.link.primary);
            }
        }

        if (end == document->length) {
            break;
        }
    }
}

void editor_draw_caret(ui_renderer* renderer, const editor_view* view,
                              const editor_viewport* viewport, const editor_settings* settings) {
    if (viewport->cursor_line < view->first_line || viewport->cursor_line - view->first_line >= viewport->rows) { return; }

    if (viewport->cursor_column < view->first_column || viewport->cursor_column - view->first_column >= viewport->columns) { return; }

    u64 idle_ms = ui_time_ms() - view->caret_visible_since_ms;
    if ((idle_ms / settings->layout.caret_blink_ms) % 2 != 0) {
        return;
    }

    i32 x = (i32)viewport->text_left +
            (i32)((viewport->cursor_column - view->first_column) * viewport->cell_width);
    i32 y = (i32)settings->layout.text_top +
            (i32)((viewport->cursor_line - view->first_line) * viewport->cell_height);
    ui_renderer_rect(renderer, x, y, settings->layout.caret_width,
                      viewport->cell_height, settings->theme.caret);
}
