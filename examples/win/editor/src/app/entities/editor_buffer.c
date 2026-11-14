/*
 ** editor_buffer.c - 3 functions.
 ** editor_document_changed - Invalidate cached document layout after editing or folding.
 ** editor_document_release - Release the document's text and optional fold storage.
 ** editor_document_reserve - Grow owned text and fold storage without changing existing content.
 ** See also: editor_document_buffer.h, editor_document.h.
 */
#include "editor_document.h"
#include "editor_undo.h"
#include <stdlib.h>
#include <string.h>

void editor_document_changed(editor_document* document) {
    static u64 next_revision;
    document->revision = ++next_revision;
}

void editor_document_release(editor_document* document) {
    editor_history_release(document);
    free(document->folded);
    free(document->text);
    *document = (editor_document){0};
}

bool editor_document_reserve(editor_document* document, size_t required) {
    if (required <= document->capacity) { return true; }
    size_t capacity = document->capacity;
    if (capacity <= SIZE_MAX / 2) { capacity *= 2; }
    if (capacity < required) { capacity = required; }
    if (capacity < 4096) { capacity = 4096; }

    if (document->folded) {
        u8* flags = realloc(document->folded, capacity); ///move
        if (!flags) { return false; }

        memset(flags + document->capacity, 0, capacity - document->capacity);
        document->folded = flags;
    }

    char* text = realloc(document->text, capacity); ///move
    if (!text) { return false; }

    text[document->length] = 0;
    document->text = text;
    document->capacity = capacity;

    return true;
}
