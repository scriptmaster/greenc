/* Header-encoded build data retains semantic/cache state using ordinary valid C headers. */
#include "internal.h"

bool gcs_write_data(gcs_compilation* c, const char* path, const char* text) {
    static const char digits[] = "0123456789abcdef";
    gcs_buffer header = {0};
    gcs_append(c, &header, "/* greenc-data-v1\n");

    for (size_t i = 0; text[i]; ++i) {
        u8 value = (u8)text[i];
        gcs_append(c, &header, "%c%c", digits[value >> 4], digits[value & 15]);

        if (i % 64 == 63) { gcs_append(c, &header, "\n"); }
    }

    gcs_append(c, &header, "\n*/\n");
    bool success = !c->diagnostic[0] && gcs_write_text(c, path, header.data);
    free(header.data);

    return success;
}

char* gcs_read_data(gcs_compilation* c, const char* path) {
    const char* header = gcs_read(c, path);
    const char* prefix = "/* greenc-data-v1\n";

    if (!header) { return NULL; }

    if (strncmp(header, prefix, strlen(prefix))) {
        gcs_error(c, path, 0, "invalid GreenC metadata header");

        return NULL;
    }

    char* text = gcs_allocate(c, strlen(header) / 2 + 1);
    char* output = text;
    const char* cursor = header + strlen(prefix);

    if (!text) { return NULL; }

    while (*cursor) {
        if (*cursor == '\n' || *cursor == '\r') {
            ++cursor;

            continue;
        }

        if (!strcmp(cursor, "*/\n")) { return text; }

        const char* digits = "0123456789abcdef";
        const char* high = *cursor ? strchr(digits, cursor[0]) : NULL;
        const char* low = cursor[0] && cursor[1] ? strchr(digits, cursor[1]) : NULL;

        if (!high || !low || (high == digits && low == digits)) { break; }

        *output++ = (char)((high - digits) * 16 + (low - digits));
        cursor += 2;
    }

    gcs_error(c, path, 0, "malformed GreenC metadata header");

    return NULL;
}
