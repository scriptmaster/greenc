/* Filesystem path identity and project-relative resolution stay outside language name lookup. */
#include "internal.h"

i32 gcs_path_compare(const char* left, const char* right) {
#ifdef _WIN32
    return (i32)_stricmp(left, right);
#else
    return (i32)strcmp(left, right);
#endif
}

bool gcs_path_suffix(const char* path, const char* suffix) {
    size_t length = strlen(path), suffix_length = strlen(suffix);

    return length >= suffix_length && !gcs_path_compare(path + length - suffix_length, suffix);
}

char* gcs_resolve_path(gcs_compilation* c, const char* directory, const char* path) {
    if (*path == '/' || *path == '\\' || (path[0] && path[1] == ':')) { return gcs_absolute(c, path); }

    char* prefix = gcs_join(c, directory, "/");
    char* joined = prefix ? gcs_join(c, prefix, path) : NULL;

    return joined ? gcs_absolute(c, joined) : NULL;
}
