/* Native paths and bounded source reads. */
#include "internal.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

char* gcs_absolute(gcs_compilation* c, const char* path) {
#ifdef _WIN32
    char resolved[1024];
    DWORD length = GetFullPathNameA(path, sizeof(resolved), resolved, NULL);

    if (!length || length >= sizeof(resolved)) {
        gcs_error(c, path, 0, "cannot resolve path");

        return NULL;
    }

    for (size_t i = 0; i < length; ++i) {
        if (resolved[i] == '\\') { resolved[i] = '/'; }
    }

    return gcs_copy(c, resolved, length);
#else
    char* normalized = gcs_copy(c, path, strlen(path));

    if (!normalized) { return NULL; }

    for (char* p = normalized; *p; ++p) {
        if (*p == '\\') { *p = '/'; }
    }

    char* resolved = realpath(normalized, NULL);

    if (!resolved) {
        gcs_error(c, path, 0, "cannot resolve path: %s", strerror(errno));

        return NULL;
    }

    char* result = gcs_copy(c, resolved, strlen(resolved));
    free(resolved);

    return result;
#endif
}

const char* gcs_directory(gcs_compilation* c, const char* path) {
    const char* slash = strrchr(path, '/');

    return slash ? gcs_copy(c, path, slash == path ? 1 : (size_t)(slash - path)) : ".";
}

char* gcs_read(gcs_compilation* c, const char* path) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        gcs_error(c, path, 0, "cannot open file: %s", strerror(errno));

        return NULL;
    }

    bool measured = fseek(file, 0, SEEK_END) == 0;
    long length = measured ? ftell(file) : -1;

    if (length < 0 || length > 16 * 1024 * 1024 || fseek(file, 0, SEEK_SET)) {
        fclose(file);
        gcs_error(c, path, 0, "cannot read file or file exceeds 16 MiB");

        return NULL;
    }

    char* text = gcs_allocate(c, (size_t)length + 1);
    bool success = text && fread(text, 1, (size_t)length, file) == (size_t)length;
    fclose(file);

    if (!success) { gcs_error(c, path, 0, "cannot read complete file"); }
    if (success && memchr(text, 0, (size_t)length)) {
        gcs_error(c, path, 0, "embedded NUL or UTF-16 input is unsupported; use UTF-8");

        return NULL;
    }

    return success ? text : NULL;
}

bool gcs_suffix(const char* path, const char* suffix) {
    size_t length = strlen(path), suffix_length = strlen(suffix);

    return length >= suffix_length && !strcmp(path + length - suffix_length, suffix);
}
