#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "entries.h"

bool sys_child_path(const char* folder, const char* name, char path[SYS_PATH_CAPACITY]) {
    size_t length = strlen(name), parent = strlen(folder);
    if (!parent || !length || name[length - 1] == '.' || name[length - 1] == ' ' ||
        strpbrk(name, "\\/:\"<>|?*")) { return false; }

    for (const unsigned char* c = (const unsigned char*)name; *c; ++c) {
        if (*c < 32) { return false; }
    }

    size_t stem = strcspn(name, ".");
    while (stem && name[stem - 1] == ' ') { --stem; }
    static const char* devices[] = {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"};
    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i) {
        if (stem == strlen(devices[i]) && !_strnicmp(name, devices[i], stem)) { return false; }
    }

    if (stem == 4 && (!_strnicmp(name, "COM", 3) || !_strnicmp(name, "LPT", 3)) &&
        name[3] >= '0' && name[3] <= '9') { return false; }

    i32 written = snprintf(path, SYS_PATH_CAPACITY, "%s%s%s", folder, folder[parent - 1] == '\\' ? "" : "\\", name);

    return written > 0 && written < SYS_PATH_CAPACITY;
}

bool sys_file_create_empty(const char* path) {
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    return file != INVALID_HANDLE_VALUE && CloseHandle(file);
}

bool sys_folder_create(const char* path) { return CreateDirectoryA(path, NULL); }
bool sys_file_copy_new(const char* source, const char* target) { return CopyFileA(source, target, TRUE); }
bool sys_entry_rename(const char* source, const char* target) { return MoveFileA(source, target); }
bool sys_file_delete(const char* path) { return DeleteFileA(path); }

bool sys_file_details(const char* path, u64* bytes, i64* modified) {
    HANDLE file = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (file == INVALID_HANDLE_VALUE) { return false; }

    BY_HANDLE_FILE_INFORMATION info;
    bool read = GetFileInformationByHandle(file, &info);
    CloseHandle(file);
    if (!read) { return false; }

    *bytes = ((u64)info.nFileSizeHigh << 32) | info.nFileSizeLow;
    if (modified) {
        u64 ticks = ((u64)info.ftLastWriteTime.dwHighDateTime << 32) | info.ftLastWriteTime.dwLowDateTime;
        *modified = (i64)(ticks / 10000000) - 11644473600LL;
    }

    return true;
}

bool sys_file_size(const char* path, u64* bytes) {
    return sys_file_details(path, bytes, NULL);
}
