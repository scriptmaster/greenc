/*
 ** row_index.c - 3 functions.
 ** reserve_row - Extend the index without overflowing allocation arithmetic.
 ** editor_index_rows - Record line boundaries and fold indicators in a single pass.
 ** editor_cursor_row - Find the visible row containing or covering a byte offset.
 ** See also: row_index.h, document_viewport.c.
 */
#include "row_index.h"
#include "byte_text.h"
#include "syntax.h"

static bool reserve_row(editor_view* view, size_t row) {
    if (row < view->row_capacity) { return true; }
    if (row >= SIZE_MAX / sizeof(editor_line_entry) / 2) { return false; }

    size_t capacity = row ? row * 2 : 128;
    editor_line_entry* entries = realloc(view->row_map, capacity * sizeof(*entries)); ///move
    if (!entries) { return false; }

    view->row_map = entries;
    view->row_capacity = capacity;

    return true;
}

bool editor_index_rows(editor_view* view, editor_document* document) {
    size_t logical = 0, rows = 0, previous = SIZE_MAX, previous_indent = 0;
    bool comment = false;
    view->longest_line = 0;
    for (size_t pos = 0; pos <= document->length;) {
        if (!reserve_row(view, rows)) { return false; }

        size_t end = editor_line_end(document, pos);
        if (end - pos > view->longest_line) { view->longest_line = end - pos; }

        size_t content = pos;
        while (content < end && document->text[content] == ' ') { ++content; }

        editor_line_entry* entry = &view->row_map[rows]; ///read
        *entry = (editor_line_entry){.offset = pos, .end = end, .logical = logical, .in_comment = comment};
        if (end > pos) { comment = syntax_comment_after(document->text + pos, end - pos, comment); }
        if (document->byte_text) {
            for (size_t i = content; i < end; ++i) {
                if (editor_byte_needs_marker((u8)document->text[i])) {
                    entry->has_unsupported_byte = true;

                    break;
                }
            }
        }

        if (content < end) {
            if (previous != SIZE_MAX && content - pos > previous_indent) { view->row_map[previous].foldable = true; }

            previous = rows;
            previous_indent = content - pos;
        }

        if (document->folded && (document->folded[pos] & 1)) {
            size_t finish = editor_fold_end(document, pos);
            if (document->cursor > end && document->cursor < finish) {
                document->folded[pos] &= ~1u;
            } else if (finish > end + 1) {
                entry->foldable = true;
                previous = SIZE_MAX;
                for (size_t hidden = end + 1; hidden < finish; ++logical) {
                    end = editor_line_end(document, hidden);
                    if (end > hidden) { comment = syntax_comment_after(document->text + hidden, end - hidden, comment); }
                    hidden = end + 1;
                    ++entry->hidden;
                }
            }
        }

        pos = end + 1;
        ++rows;
        ++logical;
    }

    view->mapped_rows = rows;
    view->mapped_revision = document->revision;

    return true;
}

size_t editor_cursor_row(const editor_view* view, size_t cursor) {
    size_t first = 0, last = view->mapped_rows;
    while (first + 1 < last) {
        size_t middle = first + (last - first) / 2;
        if (view->row_map[middle].offset <= cursor) {
            first = middle;
        } else {
            last = middle;
        }
    }

    return first;
}
