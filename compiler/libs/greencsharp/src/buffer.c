/* Bounded emitted text and JSON string encoding. */
#include "internal.h"

bool gcs_append(gcs_compilation* c, gcs_buffer* buffer, const char* format, ...) {
    if (c->diagnostic[0]) { return false; }

    va_list arguments, measure;
    va_start(arguments, format);
    va_copy(measure, arguments);
    int count = vsnprintf(NULL, 0, format, measure);
    va_end(measure);

    if (count < 0 || (size_t)count > 16 * 1024 * 1024 - buffer->length) {
        va_end(arguments);
        gcs_error(c, NULL, 0, "generated artifact exceeds 16 MiB");

        return false;
    }

    size_t needed = buffer->length + (size_t)count + 1;

    if (needed > buffer->capacity) {
        size_t capacity = needed < 4096 ? 4096 : needed * 2;
        char* data = realloc(buffer->data, capacity);

        if (!data) {
            va_end(arguments);
            gcs_error(c, NULL, 0, "out of memory emitting output");

            return false;
        }

        buffer->data = data;
        buffer->capacity = capacity;
    }

    vsnprintf(buffer->data + buffer->length, needed - buffer->length, format, arguments);
    va_end(arguments);
    buffer->length += (size_t)count;

    return true;
}

bool gcs_json(gcs_compilation* c, gcs_buffer* buffer, const char* text) {
    gcs_append(c, buffer, "\"");

    for (const unsigned char* character = (const unsigned char*)text; *character; ++character) {
        if (*character == '"' || *character == '\\') {
            gcs_append(c, buffer, "\\%c", *character);
        } else if (*character < 32) {
            gcs_append(c, buffer, "\\u%04x", *character);
        } else {
            gcs_append(c, buffer, "%c", *character);
        }
    }

    return gcs_append(c, buffer, "\"");
}
