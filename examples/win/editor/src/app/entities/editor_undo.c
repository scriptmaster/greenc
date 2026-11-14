/*
 ** editor_undo.c - 5 functions.
 ** history_apply - Capture the inverse patch and restore one history entry.
 ** editor_redo - Reapply the next undone edit.
 ** editor_record_edit - Save a reversible patch before modifying the buffer.
 ** editor_undo - Restore bytes, line flags, caret, selection and saved-state identity.
 ** editor_history_release - Release each patch owned by the document.
 ** See also: editor_undo.h, editor_buffer.c.
 */
#include "history_internal.h"
#include "editor_document.h"
#include <stdlib.h>
#include <string.h>

bool editor_record_edit(editor_document* doc, size_t offset, size_t removed, size_t inserted) {
    editor_undo_entry* entry = history_capture(doc, offset, removed, inserted);

    if (!entry) { return false; }

    history_clear(&doc->redo_history);
    entry->previous = doc->history;
    doc->history = entry;
    doc->edit_state = ++doc->next_state;

    return true;
}

static void history_apply(editor_document* doc, editor_undo_entry** source, editor_undo_entry** destination) {
    editor_undo_entry* entry = *source;

    if (!entry) { return; }

    editor_undo_entry* inverse = history_capture(doc, entry->offset, entry->inserted, entry->removed);

    if (!inverse) { return; }

    size_t offset = entry->offset;
    size_t tail = doc->length - offset - entry->inserted;
    memmove(doc->text + offset + entry->removed, doc->text + offset + entry->inserted, tail + 1);
    memcpy(doc->text + offset, entry->bytes, entry->removed);

    if (doc->folded) {
        memmove(doc->folded + offset + entry->removed, doc->folded + offset + entry->inserted, tail + 1);
        if (entry->flags) { memcpy(doc->folded + offset, entry->bytes + entry->removed, entry->removed); }
        else { memset(doc->folded + offset, 0, entry->removed); }
    }

    doc->length = offset + entry->removed + tail;
    doc->cursor = entry->cursor;
    doc->anchor = entry->anchor;
    doc->selecting = entry->selecting;
    doc->low_nibble = entry->low_nibble;
    doc->hex_first = entry->hex_first;
    doc->edit_state = entry->state;
    doc->dirty = doc->edit_state != doc->saved_state;
    *source = entry->previous;
    inverse->previous = *destination;
    *destination = inverse;
    free(entry);
    editor_document_changed(doc);
}

void editor_undo(editor_document* doc) {
    history_apply(doc, &doc->history, &doc->redo_history);
}

void editor_redo(editor_document* doc) {
    history_apply(doc, &doc->redo_history, &doc->history);
}

void editor_history_release(editor_document* doc) {
    history_clear(&doc->history);
    history_clear(&doc->redo_history);
}
