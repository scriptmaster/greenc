/*
 ** history_internal.h - 2 functions.
 ** history_capture - Copy the bytes and editor state needed to reverse a change.
 ** history_clear - Release a chain of document edits.
 ** See also: history_storage.c, editor_undo.c.
 */
#pragma once
#include "editor_undo.h"

typedef struct editor_undo_entry {
    struct editor_undo_entry* previous;
    size_t offset, removed, inserted, cursor, anchor, hex_first;
    u64 state;
    bool selecting, low_nibble, flags;
    char bytes[];
} editor_undo_entry;

editor_undo_entry* history_capture(editor_document* doc, size_t offset, size_t removed, size_t inserted);
void history_clear(editor_undo_entry** head);
