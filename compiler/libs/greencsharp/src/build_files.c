/* Owned build-artifact paths, content-stable writes and atomic publication. */
#include "internal.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

bool gcs_file_exists(const char* path) {
    FILE* file = fopen(path, "rb");

    if (!file) { return false; }

    fclose(file);

    return true;
}

bool gcs_make_directories(gcs_compilation* c, const char* path) {
    char* directory = gcs_copy(c, path, strlen(path));

    if (!directory) { return false; }

    for (char* p = directory + 1; *p; ++p) {
        if (*p != '/') { continue; }

        *p = 0;
#ifdef _WIN32
        bool success = CreateDirectoryA(directory, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
        bool success = mkdir(directory, 0755) == 0 || errno == EEXIST;
#endif
        *p = '/';

        if (!success) {
            gcs_error(c, path, 0, "cannot create build directory");

            return false;
        }
    }

    return true;
}

bool gcs_write_text(gcs_compilation* c, const char* path, const char* text) {
    if (gcs_file_exists(path)) {
        FILE* existing = fopen(path, "rb");
        size_t length = strlen(text), compared = 0;
        bool equal = existing != NULL;
        u8 buffer[4096];

        while (equal && compared < length) {
            size_t count = length - compared < sizeof(buffer) ? length - compared : sizeof(buffer);
            equal = fread(buffer, 1, count, existing) == count && !memcmp(buffer, text + compared, count);
            compared += count;
        }

        equal = equal && fgetc(existing) == EOF && !ferror(existing);

        if (existing) { fclose(existing); }
        if (equal) { return true; }
    }

    if (!gcs_make_directories(c, path)) { return false; }

    char* temporary = gcs_join(c, path, ".write.tmp");
    FILE* file = temporary ? fopen(temporary, "wb") : NULL;

    if (!file) {
        gcs_error(c, path, 0, "cannot write build artifact");

        return false;
    }

    size_t length = strlen(text);
    bool success = fwrite(text, 1, length, file) == length;

    if (fclose(file)) { success = false; }
    if (!success) { gcs_error(c, path, 0, "cannot complete build artifact"); }

    return success && gcs_replace_file(c, temporary, path);
}

bool gcs_replace_file(gcs_compilation* c, const char* temporary, const char* destination) {
#ifdef _WIN32
    bool success = MoveFileExA(temporary, destination, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    DWORD error = success ? ERROR_SUCCESS : GetLastError();

    /* A just-exited executable or scanner can briefly retain an image/file handle. */
    for (size_t attempt = 0; !success && attempt < 20 &&
         (error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION); ++attempt) {
        Sleep(25);
        success = MoveFileExA(temporary, destination, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        error = success ? ERROR_SUCCESS : GetLastError();
    }
#else
    bool success = rename(temporary, destination) == 0;
#endif

    if (!success) { gcs_error(c, destination, 0, "cannot publish completed build artifact; check file locks and output permissions"); }

    return success;
}

bool gcs_remove_file(gcs_compilation* c, const char* path) {
    if (remove(path) == 0 || errno == ENOENT) { return true; }

    gcs_error(c, path, 0, "cannot remove owned build artifact");

    return false;
}
