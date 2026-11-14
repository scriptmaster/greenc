/*
 ** history_storage.c - 2 functions.
 ** history_capture - Allocate a range patch and remember its caret and saved-state identity.
 ** history_clear - Dispose of every patch in one stack.
 ** See also: history_internal.h, editor_undo.c.
 */
#include "history_internal.h"
#include <stdlib.h>
#include <string.h>

editor_undo_entry* history_capture(editor_document* doc, size_t offset, size_t removed, size_t inserted) {
    size_t copies = doc->folded ? 2 : 1;
    editor_undo_entry* entry = NULL;

    if (removed <= (SIZE_MAX - sizeof(*entry)) / copies) {
        entry = malloc(sizeof(*entry) + removed * copies);
    }

    if (!entry) {
        doc->status = "Insufficient memory for undo history";

        return NULL;
    }

    *entry = (editor_undo_entry){.offset = offset, .removed = removed,
        .inserted = inserted, .cursor = doc->cursor, .anchor = doc->anchor, .state = doc->edit_state,
        .hex_first = doc->hex_first, .selecting = doc->selecting, .low_nibble = doc->low_nibble,
        .flags = doc->folded != NULL};

    if (removed) {
        memcpy(entry->bytes, doc->text + offset, removed);
        if (entry->flags) { memcpy(entry->bytes + removed, doc->folded + offset, removed); }
    }

    return entry;
}

void history_clear(editor_undo_entry** head) {
    while (*head) {
        editor_undo_entry* entry = *head;
        *head = entry->previous;
        free(entry);
    }
}
