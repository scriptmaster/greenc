/*
 ** editor_document_types.h - 0 functions.
 ** Define the shared data and contracts for this module.
 ** See also: document_edit.h, document_io.h, document_navigation.h, editor_document.h, editor_document_buffer.h.
 */
#pragma once
#include "sys/builtin/types_intrinsic.h"
#include <stdbool.h>

enum { EDITOR_CLIPBOARD_CAPACITY = 4096 };

typedef struct {
    u64 disk_bytes;
    i64 modified_at;
    char modified[32];
    bool binary, full_hex, low_nibble, byte_text;
    size_t hex_first;
    char* text;
    u8* folded; /* Line-start flags: bit 0 is folded, bit 1 is a breakpoint. */
    size_t capacity;
    u64 revision;
    const char* open_error;
    size_t animated_fold;
    u64 fold_changed;
    size_t length;
    size_t cursor;
    size_t anchor;
    bool selecting;
    struct editor_undo_entry* history;
    struct editor_undo_entry* redo_history;
    u64 edit_state, saved_state, next_state;
    bool dirty;
    const char* status;
} editor_document;

typedef enum {
    EDITOR_MOVE_LEFT, EDITOR_MOVE_RIGHT, EDITOR_MOVE_UP, EDITOR_MOVE_DOWN,
    EDITOR_MOVE_LINE_START, EDITOR_MOVE_LINE_END
} editor_movement;
