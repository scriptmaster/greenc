#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "directory.h"

bool sys_parent_directory(const char* path, char result[SYS_PATH_CAPACITY]) {
    char* file = NULL;
    DWORD length = GetFullPathNameA(path, SYS_PATH_CAPACITY, result, &file);
    if (!length || length >= SYS_PATH_CAPACITY || !file) { return false; }

    *file = 0;

    return true;
}

bool sys_directory_each(const char* path, sys_directory_visit visit, void* context) {
    char pattern[SYS_PATH_CAPACITY];
    if (snprintf(pattern, sizeof(pattern), "%s\\*", path) >= (int)sizeof(pattern)) { return false; }

    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) { return GetLastError() == ERROR_FILE_NOT_FOUND; }

    bool success = true;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) { continue; }

        if (!visit(context, entry.cFileName, (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0,
                   (entry.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0)) {
            success = false;
            break;
        }
    } while (FindNextFileA(search, &entry));

    success = success && GetLastError() == ERROR_NO_MORE_FILES;
    FindClose(search);

    return success;
}

/* Windows change-notification handles own no application state. */
sys_folder_watch* sys_folder_watch_create(const char* path) {
    HANDLE handle = FindFirstChangeNotificationA(path, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE);

    return handle == INVALID_HANDLE_VALUE ? NULL : (sys_folder_watch*)handle;
}

bool sys_folder_watch_changed(sys_folder_watch* watch) {
    if (!watch || WaitForSingleObject((HANDLE)watch, 0) != WAIT_OBJECT_0) { return false; }

    FindNextChangeNotification((HANDLE)watch);

    return true;
}

void sys_folder_watch_destroy(sys_folder_watch* watch) {
    if (watch) { FindCloseChangeNotification((HANDLE)watch); }
}
