/* Source-tree traversal excludes generated directories and never follows directory links. */
#include "internal.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static bool gcs_scan_entry(gcs_compilation* c, gcs_project* project, const char* directory,
                           const char* name, bool folder, size_t depth) {
    if (name[0] == '.' || !strcmp(name, "bin") || !strcmp(name, "obj") || !strcmp(name, "build")) { return true; }

    char* prefix = gcs_join(c, directory, "/");
    char* path = prefix ? gcs_join(c, prefix, name) : NULL;

    if (!path) { return false; }
    if (folder) { return gcs_scan_sources(c, project, path, depth + 1); }

    return !gcs_path_suffix(name, ".cs") || gcs_add_source(c, project, path);
}

bool gcs_scan_sources(gcs_compilation* c, gcs_project* project, const char* directory, size_t depth) {
    if (depth > 64) {
        gcs_error(c, directory, 0, "source directory depth exceeds 64");

        return false;
    }

    bool success = true;
#ifdef _WIN32
    char* pattern = gcs_join(c, directory, "/*");
    WIN32_FIND_DATAA entry;
    HANDLE search = pattern ? FindFirstFileA(pattern, &entry) : INVALID_HANDLE_VALUE;

    if (search == INVALID_HANDLE_VALUE) { return false; }

    do {
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) { continue; }

        success = gcs_scan_entry(c, project, directory, entry.cFileName,
                                (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0, depth);

        if (!success) { break; }
    } while (FindNextFileA(search, &entry));

    if (success && GetLastError() != ERROR_NO_MORE_FILES) { success = false; }

    FindClose(search);
#else
    DIR* search = opendir(directory);

    if (!search) { return false; }

    struct dirent* entry;

    while ((entry = readdir(search)) != NULL) {
        char* prefix = gcs_join(c, directory, "/");
        char* path = prefix ? gcs_join(c, prefix, entry->d_name) : NULL;
        struct stat attributes;

        if (!path || lstat(path, &attributes)) {
            success = false;

            break;
        }
        if (S_ISLNK(attributes.st_mode)) { continue; }

        success = gcs_scan_entry(c, project, directory, entry->d_name, S_ISDIR(attributes.st_mode), depth);

        if (!success) { break; }
    }

    closedir(search);
#endif
    if (!success) { gcs_error(c, directory, 0, "cannot enumerate source directory"); }

    return success;
}
