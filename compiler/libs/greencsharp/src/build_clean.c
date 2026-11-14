/* Build cleanup removes only recorded, normalized project-owned artifact files. */
#include "internal.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

bool gcs_merge_artifacts(gcs_compilation* c, const char* manifest) {
    if (!gcs_file_exists(manifest)) { return true; }

    char* previous = gcs_read_data(c, manifest);

    if (!previous) { return false; }

    for (char* cursor = previous; *cursor;) {
        char* end = strchr(cursor, '\n');

        if (end) { *end = 0; }
        size_t length = strlen(cursor);

        if (length && cursor[length - 1] == '\r') { cursor[length - 1] = 0; }
        if (*cursor) { gcs_record_artifact(c, cursor); }
        if (!end) { break; }

        cursor = end + 1;
    }

    return !c->diagnostic[0];
}

static bool gcs_plain_artifact_path(gcs_compilation* c, const char* path) {
    char* checked = gcs_copy(c, path, strlen(path));

    if (!checked) { return false; }

    for (char* p = checked + 1; *p; ++p) {
        if (*p != '/') { continue; }

        *p = 0;
#ifdef _WIN32
        DWORD attributes = GetFileAttributesA(checked);
        bool linked = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
        struct stat info;
        bool linked = lstat(checked, &info) == 0 && S_ISLNK(info.st_mode);
#endif
        *p = '/';

        if (linked) {
            gcs_error(c, path, 0, "refusing to clean through a directory link");

            return false;
        }
    }

    return true;
}

void gcs_record_artifact(gcs_compilation* c, const char* path) {
    if (!path) { return; }

    size_t length = strlen(path);
    const char* existing = c->manifest.data;

    while (existing && (existing = strstr(existing, path)) != NULL) {
        if ((existing == c->manifest.data || existing[-1] == '\n') && existing[length] == '\n') { return; }

        ++existing;
    }

    gcs_append(c, &c->manifest, "%s\n", path);
}

static bool gcs_owned_artifact(gcs_compilation* c, gcs_project* project, const char* path) {
    if (strchr(path, '\\') || strstr(path, "/../") || strstr(path, "/./") ||
        gcs_suffix(path, "/..") || gcs_suffix(path, "/.")) { return false; }

    size_t length = strlen(project->object_directory);
    bool own_object = strlen(path) > length && !strncmp(path, project->object_directory, length) && path[length] == '/';
    char* stamp = gcs_join(c, project->output, ".cache.h");
    bool own_output = !gcs_path_compare(path, project->output) || !gcs_path_compare(path, project->contract) ||
                      !gcs_path_compare(path, project->public_header) || (stamp && !gcs_path_compare(path, stamp));

    if ((own_object || own_output) && gcs_plain_artifact_path(c, path)) { return true; }

    gcs_error(c, path, 0, "refusing to clean an artifact outside the project output roots");

    return false;
}

bool gcs_clean_project(gcs_compilation* c, gcs_project* project) {
    const char* manifest = gcs_build_path(c, project->object_directory, "artifacts.h");

    if (!manifest) { return false; }
    if (!gcs_file_exists(manifest)) { return true; }

    char* text = gcs_read_data(c, manifest);

    if (!text) { return false; }

    size_t length = strlen(text);

    for (size_t i = 0; i < length; ++i) {
        if (text[i] == '\r' || text[i] == '\n') { text[i] = 0; }
    }

    /* Validate the complete manifest before removing any project artifacts. */
    for (size_t i = 0; i < length;) {
        const char* path = text + i;
        i += strlen(path) + 1;

        if (*path && !gcs_owned_artifact(c, project, path)) { return false; }
    }

    for (size_t i = 0; i < length;) {
        const char* path = text + i;
        i += strlen(path) + 1;

        if (*path && !gcs_remove_file(c, path)) { return false; }
    }

    return gcs_remove_file(c, manifest);
}
