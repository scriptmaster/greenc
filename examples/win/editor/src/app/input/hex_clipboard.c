/*
 ** hex_clipboard.c - 2 functions.
 ** editor_hex_limit - Bound navigation and clipboard operations to displayed bytes.
 ** editor_hex_copy - Encode selected bytes as lossless, space-separated hex pairs.
 ** See also: editor_hex.h, hex_view.c.
 */
#include "editor_hex.h"
#include <stdlib.h>

size_t editor_hex_limit(const editor_document* doc) {
    return !doc->full_hex && doc->length > 256 ? 256 : doc->length;
}

void editor_hex_copy(editor_document* doc, ui_window* window) {
    size_t first, last;
    editor_selection(doc, &first, &last);

    if (last >= editor_hex_limit(doc)) { return; }

    size_t count = last - first + 1;
    char* output = count <= SIZE_MAX / 3 ? malloc(count * 3) : NULL;

    if (!output) {
        doc->status = "Insufficient memory to copy bytes";

        return;
    }

    static const char digits[] = "0123456789ABCDEF";
    for (size_t i = 0; i < count; ++i) {
        u8 byte = (u8)doc->text[first + i];
        output[3 * i] = digits[byte >> 4];
        output[3 * i + 1] = digits[byte & 15];
        output[3 * i + 2] = ' ';
    }

    if (!ui_clipboard_write(window, output, 3 * count - 1)) { doc->status = "Clipboard unavailable"; }
    free(output);
}
