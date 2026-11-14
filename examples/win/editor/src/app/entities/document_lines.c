/*
 ** document_lines.c - 2 functions.
 ** editor_line_start - Find the beginning of the cursor's logical line.
 ** editor_line_end - Find the newline or document end following an offset.
 ** See also: document_navigation.h, editor_document.h.
 */
#include "editor_document.h"
#include <string.h>

size_t editor_line_start(const editor_document* document, size_t cursor) {
    while (cursor && document->text[cursor - 1] != '\n') {
        --cursor;
    }

    return cursor;
}

size_t editor_line_end(const editor_document* document, size_t cursor) {
    if (cursor >= document->length) { return document->length; }
    const char* newline = memchr(document->text + cursor, '\n', document->length - cursor);
    cursor = newline ? (size_t)(newline - document->text) : document->length;

    return cursor;
}
