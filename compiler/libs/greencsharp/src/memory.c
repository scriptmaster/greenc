/* Compilation arena, diagnostics and ownership cleanup. */
#include "internal.h"

struct gcs_allocation { gcs_allocation* next; max_align_t alignment; };

void gcs_error(gcs_compilation* c, const char* path, size_t line, const char* format, ...) {
    if (c->diagnostic[0]) { return; }

    int prefix = snprintf(c->diagnostic, sizeof(c->diagnostic), "%s:%zu: greencsharp: ", path ? path : "input", line);

    if (prefix < 0 || (size_t)prefix >= sizeof(c->diagnostic)) { return; }

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(c->diagnostic + prefix, sizeof(c->diagnostic) - (size_t)prefix, format, arguments);
    va_end(arguments);
}

void* gcs_allocate(gcs_compilation* c, size_t size) {
    if (size > SIZE_MAX - sizeof(gcs_allocation)) {
        gcs_error(c, NULL, 0, "allocation size overflow");

        return NULL;
    }

    gcs_allocation* allocation = calloc(1, sizeof(*allocation) + size);

    if (!allocation) {
        gcs_error(c, NULL, 0, "out of memory");

        return NULL;
    }

    allocation->next = c->allocations;
    c->allocations = allocation;

    return allocation + 1;
}

char* gcs_copy(gcs_compilation* c, const char* text, size_t length) {
    if (length == SIZE_MAX) { return NULL; }

    char* copy = gcs_allocate(c, length + 1);

    if (copy) { memcpy(copy, text, length); }

    return copy;
}

char* gcs_join(gcs_compilation* c, const char* left, const char* right) {
    size_t first = strlen(left), second = strlen(right);

    if (first > SIZE_MAX - second - 1) { return NULL; }

    char* joined = gcs_allocate(c, first + second + 1);

    if (joined) {
        memcpy(joined, left, first);
        memcpy(joined + first, right, second + 1);
    }

    return joined;
}

void gcs_release(gcs_compilation* c) {
    gcs_allocation* allocation = c->allocations;

    while (allocation) {
        gcs_allocation* next = allocation->next;
        free(allocation);
        allocation = next;
    }

    free(c->program.data);
    free(c->ast.data);
    free(c->dependencies.data);
    free(c->manifest.data);
    free(c);
}
