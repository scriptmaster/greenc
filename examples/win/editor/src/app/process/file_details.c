/*
 ** file_details.c - 2 functions.
 ** editor_file_details - Record the file size and format its local timestamp.
 ** editor_read_binary - Replace partially normalized text with byte-exact input.
 ** See also: file_details.h.
 */
#include "file_details.h"
#include "sys/fs/entries.h"
#include <time.h>
#include <string.h>

void editor_file_details(editor_document* document, const char* path) {
    i64 modified;
    if (!sys_file_details(path, &document->disk_bytes, &modified)) { return; }

    document->modified_at = modified;
    time_t stamp = (time_t)modified;
    struct tm* local = localtime(&stamp);
    if (local) { strftime(document->modified, sizeof(document->modified), "%Y-%m-%d %I:%M:%S %p", local); }
}

bool editor_read_binary(editor_document* document, FILE* file) {
    if (_fseeki64(file, 0, SEEK_SET) != 0) { return false; }

    document->length = 0;
    u8 bytes[65536];
    size_t count;
    while ((count = fread(bytes, 1, sizeof(bytes), file))) {
        if (count >= SIZE_MAX - document->length ||
            !editor_document_reserve(document, document->length + count + 1)) { return false; }

        memcpy(document->text + document->length, bytes, count);
        document->length += count;
    }

    document->binary = true;
    document->full_hex = document->length <= 256;

    return !ferror(file);
}
