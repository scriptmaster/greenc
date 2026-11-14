/*
 ** document_edit.c - 5 functions.
 ** replace_range - Record and apply a buffer edit at explicit offsets.
 ** editor_selection - Return the ordered selection endpoints.
 ** editor_replace_selection - Replace the selected range and update its caret and fold state.
 ** editor_insert_character - Insert a supported character or expand a tab.
 ** editor_delete_character - Remove the selection or adjacent character.
 ** See also: document_edit.h, editor_document.h.
 */
#include "editor_document.h"
#include <string.h>
#include "editor_undo.h"

void editor_selection(const editor_document* document, size_t* start, size_t* end) {
    size_t anchor = document->selecting ? document->anchor : document->cursor;
    *start = anchor < document->cursor ? anchor : document->cursor;
    *end = anchor > document->cursor ? anchor : document->cursor;
}

static bool replace_range(editor_document* document, size_t start, size_t end, const char* text, size_t count) {
    size_t remaining = document->length - (end - start);
    if (count >= SIZE_MAX - remaining || !editor_document_reserve(document, remaining + count + 1)) {
        document->status = "Insufficient memory for this edit";

        return false;
    }

    if (!editor_record_edit(document, start, end - start, count)) { return false; }

    if (document->folded) {
        memmove(document->folded + start + count, document->folded + end, document->length - end + 1);
        memset(document->folded + start, 0, count);
    }

    memmove(document->text + start + count, document->text + end, document->length - end + 1);
    memcpy(document->text + start, text, count);
    document->length = remaining + count;
    document->cursor = start + count;
    document->selecting = false;
    document->fold_changed = 0;
    document->open_error = NULL;
    editor_document_changed(document);
    document->dirty = true;

    return true;
}

bool editor_replace_selection(editor_document* document, const char* text, size_t count) {
    size_t start, end;
    editor_selection(document, &start, &end);

    return replace_range(document, start, end, text, count);
}

void editor_insert_character(editor_document* document, u32 character) {
    if (character != '\n' && character != '\t' && (character < 32 || character > 126)) {
        return;
    }

    char letter = (char)character;
    if (character == '\t') {
        editor_replace_selection(document, "    ", 4);
    } else {
        editor_replace_selection(document, &letter, 1);
    }
}

void editor_delete_character(editor_document* document, bool backward) {
    if (document->selecting && document->anchor != document->cursor) {
        editor_replace_selection(document, "", 0);

        return;
    }

    if ((backward && !document->cursor) ||
        (!backward && document->cursor == document->length)) {
        return;
    }

    size_t start = document->cursor - (backward ? 1 : 0);
    replace_range(document, start, start + 1, "", 0);
}
