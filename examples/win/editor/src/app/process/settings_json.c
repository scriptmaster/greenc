/*
 ** settings_json.c - 5 functions.
 ** editor_settings_json_format - Format a complete editable settings object.
 ** whitespace - Skip JSON whitespace between tokens.
 ** string_value - Decode a printable string with common JSON escapes.
 ** read_setting - Validate a named field and reject duplicates.
 ** editor_settings_json_parse - Apply a valid object as one transaction.
 ** See also: settings_json.h, theme_files.c.
 */
#include "settings_json.h"
#include "themes_internal.h"

bool editor_settings_json_format(const editor_theme* theme, const char* debugger, char* output, size_t capacity) {
    char escaped[520];
    size_t used = 0;

    for (const char* c = debugger; *c; ++c) {
        if ((u8)*c < 32 || used + 2 >= sizeof(escaped)) { return false; }
        if (*c == '\\' || *c == '"') { escaped[used++] = '\\'; }

        escaped[used++] = *c;
    }

    escaped[used] = 0;
    int written = snprintf(output, capacity, "{\n  \"debugger\": \"%s\"", escaped);

    if (written < 0 || (size_t)written >= capacity) { return false; }
    used = (size_t)written;

    for (size_t i = 0; i < FIELD_COUNT; ++i) {
        u32 value = *(const u32*)((const char*)theme + fields[i].offset);
        const char* format = fields[i].maximum ? ",\n  \"%s\": %u" : ",\n  \"%s\": \"#%06X\"";
        written = snprintf(output + used, capacity - used, format, fields[i].name, value);

        if (written < 0 || (size_t)written >= capacity - used) { return false; }

        used += (size_t)written;
    }

    if (capacity - used < 4) { return false; }
    memcpy(output + used, "\n}\n", 4);

    return true;
}

static const char* whitespace(const char* p) {
    return p + strspn(p, " \n\r\t");
}

static bool string_value(const char** input, char* output, size_t capacity) {
    const char* p = *input;
    size_t used = 0;

    if (*p++ != '"') { return false; }

    while (*p && *p != '"') {
        char c = *p++;

        if (c == '\\') {
            c = *p++;
            if (c != '\\' && c != '"' && c != '/') { return false; }
        }

        if ((u8)c < 32 || used + 1 >= capacity) { return false; }
        output[used++] = c;
    }

    if (*p != '"') { return false; }
    output[used] = 0;
    *input = p + 1;

    return true;
}

static bool read_setting(const char** input, editor_theme* theme, char debugger[260], bool seen[]) {
    const char* p = whitespace(*input);
    char key[64];

    if (!string_value(&p, key, sizeof(key))) { return false; }
    p = whitespace(p);
    if (*p++ != ':') { return false; }
    p = whitespace(p);

    size_t i = 0;
    while (i < FIELD_COUNT && strcmp(key, fields[i].name)) { ++i; }
    if (seen[i]) { return false; }
    seen[i] = true;

    if (i == FIELD_COUNT) {
        if (strcmp(key, "debugger") || !string_value(&p, debugger, 260)) { return false; }
    } else if (fields[i].maximum) {
        size_t digits = strspn(p, "0123456789");

        if (!digits || digits > 3 || (digits > 1 && *p == '0')) { return false; }
        u32 value = (u32)strtoul(p, NULL, 10);
        if (value > fields[i].maximum) { return false; }

        *theme_value(theme, i) = value;
        p += digits;
    } else {
        char color[8];

        if (!string_value(&p, color, sizeof(color)) || color[0] != '#' ||
            strspn(color + 1, "0123456789abcdefABCDEF") != 6) { return false; }

        *theme_value(theme, i) = (u32)strtoul(color + 1, NULL, 16);
    }

    *input = whitespace(p);

    return true;
}

bool editor_settings_json_parse(const char* json, editor_theme* theme, char debugger[260]) {
    editor_theme candidate = *theme;
    char path[260];
    bool seen[FIELD_COUNT + 1] = {0};
    const char* p = whitespace(json);
    strcpy(path, debugger);

    if (*p++ != '{') { return false; }
    p = whitespace(p);

    if (*p != '}') {
        do {
            if (!read_setting(&p, &candidate, path, seen)) { return false; }
            if (*p != ',') { break; }
            ++p;
        } while (true);
    }

    if (*p++ != '}' || *whitespace(p)) { return false; }
    *theme = candidate;
    strcpy(debugger, path);

    return true;
}
