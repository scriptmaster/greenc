/*
 ** editor_folds.c - 2 functions.
 ** indent - Count leading spaces on a logical line.
 ** editor_fold_end - Find the end of an indented block.
 ** See also: document_navigation.h, editor_document.h.
 */
#include "editor_document.h"

static size_t indent(const editor_document* doc, size_t pos) {
    size_t start = pos;
    while (pos < doc->length && doc->text[pos] == ' ') { ++pos; }

    return pos - start;
}

/* Fold the indented body. Keep the next sibling or closing brace visible. */
size_t editor_fold_end(const editor_document* doc, size_t start) {
    size_t next = editor_line_end(doc, start) + 1;
    size_t base = indent(doc, start);
    if (start + base + 1 >= next) { return next; }

    size_t pos = next, finish = next;
    while (pos < doc->length) {
        size_t end = editor_line_end(doc, pos);
        size_t spaces = indent(doc, pos);
        if (pos + spaces < end) {
            if (spaces <= base) { break; }
            finish = end + 1;
        }

        pos = end + 1;
    }

    return finish;
}
