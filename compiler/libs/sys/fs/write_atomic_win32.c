#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "write_atomic.h"

bool sys_file_write_atomic(const char* path, const char* data, size_t length) {
    if (!path || (!data && length)) {
        return false;
    }

    char full_path[MAX_PATH], directory[MAX_PATH], temporary[MAX_PATH];
    char* filename = NULL;
    DWORD count = GetFullPathNameA(path, MAX_PATH, full_path, &filename);
    if (!count || count >= MAX_PATH || !filename || !*filename) {
        return false;
    }

    size_t directory_length = (size_t)(filename - full_path);
    memcpy(directory, full_path, directory_length);
    directory[directory_length] = '\0';
    if (!GetTempFileNameA(directory, "gce", 0, temporary)) {
        return false;
    }

    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    bool success = file != INVALID_HANDLE_VALUE;
    size_t offset = 0;
    while (success && offset < length) {
        DWORD written = 0;
        DWORD chunk = length - offset > MAXDWORD ? MAXDWORD : (DWORD)(length - offset);
        success = WriteFile(file, data + offset, chunk, &written, NULL) && written != 0;
        offset += written;
    }

    if (file != INVALID_HANDLE_VALUE) {
        if (!FlushFileBuffers(file)) {
            success = false;
        }

        if (!CloseHandle(file)) {
            success = false;
        }
    }

    if (success) {
        success = MoveFileExA(temporary, full_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }

    if (!success) {
        DeleteFileA(temporary);
    }

    return success;
}
