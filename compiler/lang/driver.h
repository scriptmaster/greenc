#pragma once

#include "libgreenc.h"

static const char* greenc_path_basename(const char* path) {
    const char* last = path;

    for (const char* q = path; *q; q++) {
        if (*q == '/' || *q == '\\') {
            last = q + 1;
        }
    }

    return last;
}

static char* greenc_dirname_alloc(const char* path) {
    const char* base = greenc_path_basename(path);

    if (base == path) {
        return greenc_concat(".", "");
    }

    size_t len = (size_t)(base - path);

    while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        len--;
    }

    if (len == 0) {
        return greenc_concat(".", "");
    }

    char* d = (char*)greenc_malloc(len + 1);

    if (!d) {
        return NULL;
    }

    greenc_memcpy(d, path, len);
    d[len] = '\0';
    return d;
}

static char* greenc_replace_suffix_with_c(
    const char* output_file,
    size_t suffix_len
) {
    size_t n = greenc_strlen(output_file);
    size_t base = n - suffix_len;
    char* out = (char*)greenc_malloc(base + 3);

    if (!out) {
        return NULL;
    }

    greenc_memcpy(out, output_file, base);
    greenc_memcpy(out + base, ".c", 3);
    return out;
}

static char* greenc_transpile_c_path_for_output(const char* output_file) {
    if (greenc_string_endswith(output_file, ".exe")) {
        return greenc_replace_suffix_with_c(output_file, 4);
    }

    if (greenc_string_endswith(output_file, ".out")) {
        return greenc_replace_suffix_with_c(output_file, 4);
    }

    return greenc_concat(output_file, ".c");
}
