/*
 ** document_io.c - 5 functions.
 ** append_chunk - Validate and normalize a block, carrying CRLF state across reads.
 ** read_document - Read and normalize a document before replacing its contents.
 ** editor_load - Load the startup path, allowing a new document if missing.
 ** editor_load_existing - Open an existing file and retain the precise failure reason.
 ** editor_save - Replace the destination atomically with the current document.
 ** See also: document_io.h, editor_document.h.
 */
#include "file_details.h"
#include "sys/fs/write_atomic.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char* append_chunk(editor_document* doc, const u8* bytes, size_t count, bool* after_cr, bool* binary) {
    size_t tabs = 0;
    for (size_t i = 0; i < count; ++i) {
        u8 c = bytes[i];
        if (c == '\t') { ++tabs; }
        if ((c < 32 && c != '\r' && c != '\n' && c != '\t') || c > 126) {
            *binary = true;

            return NULL;
        }
    }

    size_t needed = count + 3 * tabs;
    if (needed >= SIZE_MAX - doc->length || !editor_document_reserve(doc, doc->length + needed + 1)) {
        return "Insufficient memory to load file";
    }

    char* dest = doc->text + doc->length;
    for (size_t i = 0; i < count; ++i) {
        u8 c = bytes[i];
        if (c == '\t') {
            memcpy(dest, "    ", 4);
            dest += 4;
        } else if (c != '\n' || !*after_cr) {
            *dest++ = c == '\r' ? '\n' : (char)c;
        }

        *after_cr = c == '\r';
    }

    doc->length = (size_t)(dest - doc->text);
    *dest = 0;

    return NULL;
}

static bool read_document(editor_document* document, const char* path, bool allow_missing) {
    FILE* file = fopen(path, "rb"); ///move
    if (!file) {
        if (errno != ENOENT || !allow_missing) {
            document->open_error = errno == ENOENT ? "File no longer exists" : strerror(errno);

            return false;
        }

        editor_document_release(document);
        document->status = "New file";

        return true;
    }

    editor_document loaded = {.status = "Ready"};
    u8 block[64 * 1024];
    bool after_cr = false, valid = true, binary = false;
    size_t count;
    size_t request = 256;

    while ((count = fread(block, 1, request, file)) != 0) {
        request = sizeof(block);
        const char* error = append_chunk(&loaded, block, count, &after_cr, &binary);
        if (binary || error) {
            valid = editor_read_binary(&loaded, file);
            if (!valid) { document->open_error = "Could not load raw file bytes"; }

            break;
        }
    }

    if (valid && ferror(file) && !loaded.binary) {
        clearerr(file);
        valid = editor_read_binary(&loaded, file);
        if (!valid) { document->open_error = "Read error before end of file"; }
    }
    valid = valid && !ferror(file);
    if (fclose(file) != 0) {
        document->open_error = "File close failed after reading";
        valid = false;
    }

    if (loaded.text) { loaded.text[loaded.length] = '\0'; }

    if (valid) {
        editor_document_release(document);
        *document = loaded;
        editor_file_details(document, path);
        editor_document_changed(document);
    } else {
        editor_document_release(&loaded);
    }

    return valid;
}

bool editor_load(editor_document* document, const char* path) {
    return read_document(document, path, true);
}

bool editor_load_existing(editor_document* document, const char* path) {
    return read_document(document, path, false);
}

void editor_save(editor_document* document, const char* path) {
    /* Write a sibling temporary file first so failed writes preserve the original.
     * C rename cannot replace files portably; replacement is a library mechanism. */
    if (sys_file_write_atomic(path, document->text ? document->text : "", document->length)) {
        editor_file_details(document, path);
        document->saved_state = document->edit_state;
        document->dirty = false;
        document->status = "Saved";
    } else {
        document->status = "Save failed; original file preserved";
    }
}
