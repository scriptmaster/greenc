/*
 ** debug_preferences.c - 4 functions.
 ** editor_theme_preference - Locate the per-user active theme.
 ** preference_path - Locate the debugger preference in local application data.
 ** editor_debug_load - Read a complete saved executable path.
 ** editor_debug_save - Atomically persist the user's choice.
 ** See also: debug_preferences.h, theme_input.c.
 */
#include "debug_preferences.h"
#include "sys/fs/write_atomic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool preference_path(char result[260], const char* filename) {
    const char* directory = getenv("LOCALAPPDATA");
    if (!directory || !*directory) { return false; }
    int length = snprintf(result, 260, "%s\\%s", directory, filename);

    return length > 0 && length < 260;
}

void editor_debug_load(char result[260]) {
    char location[260];
    if (!preference_path(location, "GreenC-editor-debugger.txt")) { return; }
    FILE* file = fopen(location, "rb"); ///move
    if (!file) { return; }
    char bytes[260];
    size_t length = fread(bytes, 1, sizeof(bytes), file);
    bool valid = length && length < sizeof(bytes) && !ferror(file) && !memchr(bytes, 0, length);
    fclose(file);
    if (valid) {
        memcpy(result, bytes, length);
        result[length] = 0;
    }
}

bool editor_debug_save(const char* path) {
    char location[260];

    return preference_path(location, "GreenC-editor-debugger.txt") && sys_file_write_atomic(location, path, strlen(path));
}

bool editor_theme_preference(char result[260]) {
    return preference_path(result, "GreenC-editor.theme");
}
