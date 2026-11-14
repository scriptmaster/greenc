/*
 ** theme_files.c - 3 functions.
 ** theme_value - Locate a configurable value in the theme.
 ** editor_theme_load - Validate a theme file before applying it.
 ** editor_theme_save - Serialize theme values using atomic replacement.
 ** See also: editor_themes.h, themes_internal.h.
 */
#include "themes_internal.h"

#define COLOR(field) {#field, offsetof(editor_theme, field), 0}
#define NUMBER(field, maximum) {#field, offsetof(editor_theme, field), maximum}
const editor_theme_field fields[] = {
    COLOR(background), COLOR(panel), COLOR(text), COLOR(heading), COLOR(muted), COLOR(caret), COLOR(selection),
    NUMBER(column_guide, 999), COLOR(button.primary), COLOR(button.secondary), COLOR(button.hover), NUMBER(button.hover_percent, 100),
    COLOR(button.press), NUMBER(button.press_percent, 100), NUMBER(button.radius, 24),
    COLOR(link.primary), COLOR(link.secondary), COLOR(link.hover), NUMBER(link.hover_percent, 100), COLOR(link.press), NUMBER(link.press_percent, 100),
    COLOR(breakpoint), COLOR(stopped), NUMBER(syntax.enabled, 1), COLOR(syntax.string), COLOR(syntax.number),
    COLOR(syntax.comment), COLOR(syntax.function), NUMBER(syntax.c_enabled, 1), COLOR(syntax.keyword), NUMBER(syntax.markdown_enabled, 1), COLOR(syntax.heading)
};

u32* theme_value(editor_theme* theme, size_t index) {
    return (u32*)((char*)theme + fields[index].offset);
}

bool editor_theme_load(editor_theme* theme, const char* path) {
    FILE* file = fopen(path, "r"); ///move
    if (!file) { return false; }

    editor_theme candidate = editor_settings_init().theme; ///stack
    char line[128];
    u32 seen = 0;
    bool valid = true;
    while (valid && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char* value = strchr(line, '=');
        if (!value) { valid = false; break; }
        *value++ = 0;
        size_t i = 0;
        while (i < FIELD_COUNT && strcmp(line, fields[i].name)) { ++i; }
        valid = i < FIELD_COUNT && !(seen & (1u << i));
        if (!valid) { break; }

        u32 parsed = 0;
        if (fields[i].maximum) {
            valid = *value && strlen(value) <= 3 && strspn(value, "0123456789") == strlen(value);
            parsed = (u32)strtoul(value, NULL, 10);
            valid = valid && parsed <= fields[i].maximum;
        } else {
            valid = *value == '#' && strlen(value + 1) == 6 && strspn(value + 1, "0123456789abcdefABCDEF") == 6;
            if (valid) { parsed = (u32)strtoul(value + 1, NULL, 16); }
        }

        if (valid) {
            *theme_value(&candidate, i) = parsed;
            seen |= 1u << i;
        }
    }

    valid = valid && !ferror(file) && (seen & 127u) == 127u;
    fclose(file);
    if (valid) { *theme = candidate; }

    return valid;
}

bool editor_theme_save(const editor_theme* theme, const char* path) {
    char text[1024];
    size_t used = 0;
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
        u32 color = *(const u32*)((const char*)theme + fields[i].offset);
        int length = snprintf(text + used, sizeof(text) - used, fields[i].maximum ? "%s=%u\n" : "%s=#%06X\n", fields[i].name, color);
        if (length < 0 || (size_t)length >= sizeof(text) - used) { return false; }
        used += (size_t)length;
    }

    return sys_file_write_atomic(path, text, used);
}
