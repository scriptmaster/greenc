/*
 ** syntax.c - 4 functions.
 ** language_for_path - Select extra rules for recognized filename extensions.
 ** keyword - Recognize complete C and GreenC keywords.
 ** token_length - Classify a token without reading beyond the visible prefix.
 ** editor_draw_syntax - Paint clipped token runs on the existing text grid.
 ** See also: syntax.h, document_rows.c.
 */
#include "syntax.h"
#include <ctype.h>

enum language { GENERAL, C_SOURCE, MARKDOWN, C_STYLE, HASH_STYLE };

static enum language language_for_path(const char* path) {
    const char* ext = strrchr(path, '.');

    if (!ext) { return GENERAL; }
    if (!_stricmp(ext, ".md") || !_stricmp(ext, ".markdown")) { return MARKDOWN; }
    if (!_stricmp(ext, ".c") || !_stricmp(ext, ".h") || !_stricmp(ext, ".gc")) { return C_SOURCE; }

    const char* c_style[] = {".cpp", ".hpp", ".cc", ".cxx", ".js", ".ts", ".java", ".cs", ".rs", ".go"};
    for (size_t i = 0; i < sizeof(c_style) / sizeof(c_style[0]); ++i) {
        if (!_stricmp(ext, c_style[i])) { return C_STYLE; }
    }

    if (!_stricmp(ext, ".py") || !_stricmp(ext, ".sh") || !_stricmp(ext, ".ps1")) { return HASH_STYLE; }

    return GENERAL;
}

static bool keyword(const char* word, size_t length) {
    const char* words[] = {"if", "else", "while", "for", "return", "break", "continue", "switch", "case",
                           "struct", "enum", "typedef", "static", "const", "void", "bool", "int", "char", "size_t"};

    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        if (strlen(words[i]) == length && !memcmp(word, words[i], length)) { return true; }
    }

    return false;
}

static size_t token_length(const char* line, size_t n, enum language lang, const editor_theme* theme, u32* color, bool* comment) {
    char c = *line;
    size_t i = 1;
    *color = theme->text;

    bool slash_comments = lang == C_SOURCE || lang == C_STYLE;
    if (slash_comments && (*comment || (n >= 2 && c == '/' && line[1] == '*'))) {
        i = *comment ? 0 : 2;
        *comment = true;
        *color = theme->syntax.comment;

        while (i + 1 < n) {
            if (line[i] == '*' && line[i + 1] == '/') {
                *comment = false;

                return i + 2;
            }

            ++i;
        }

        return n;
    }

    if (c == '"' || c == '\'') {
        *color = theme->syntax.string;

        while (i < n) {
            char next = line[i++];
            if (next == c) { break; }
            if (next == '\\' && i < n) { ++i; }
        }
    } else if ( (slash_comments && n >= 2 && c == '/' && line[1] == '/') || (lang == HASH_STYLE && c == '#')) {
        *color = theme->syntax.comment;

        return n;
    } else if (lang == C_SOURCE && c == '#') {
        while (i < n && (line[i] == ' ' || line[i] == '\t')) { ++i; }
        while (i < n && isalpha((u8)line[i])) { ++i; }
        *color = theme->syntax.keyword;
    } else if (isalnum((u8)c) || c == '_') {
        while (i < n && (isalnum((u8)line[i]) || line[i] == '_')) { ++i; }

        if (isdigit((u8)c)) { *color = theme->syntax.number; }
        else if ((slash_comments || lang == HASH_STYLE) && keyword(line, i)) { *color = theme->syntax.keyword; }
        else if (lang != MARKDOWN) {
            size_t next = i;
            while (next < n && isspace((u8)line[next])) { ++next; }
            if (next < n && line[next] == '(') { *color = theme->syntax.function; }
        }
    } else if (isspace((u8)c)) {
        while (i < n && isspace((u8)line[i])) { ++i; }
    }

    return i;
}

void editor_draw_syntax(ui_renderer* renderer, const editor_settings* settings, const char* line,
                        size_t line_length, bool comment, size_t skip, size_t count, i32 left, i32 y, u32 cell_width) {
    const editor_theme* theme = &settings->theme;
    enum language lang = language_for_path(settings->path);
    size_t end = skip + count;
    if (lang == C_SOURCE && !theme->syntax.c_enabled) { lang = GENERAL; }

    bool heading = lang == MARKDOWN && theme->syntax.markdown_enabled && end && line[0] == '#';

    for (size_t pos = theme->syntax.enabled ? 0 : skip; pos < end;) {
        u32 color = heading && theme->syntax.enabled ? theme->syntax.heading : theme->text;
        size_t length = theme->syntax.enabled && !heading ? token_length(line + pos, line_length - pos, lang, theme, &color, &comment) : end - pos;
        size_t next = length > end - pos ? end : pos + length;

        for (size_t start = pos < skip ? skip : pos; start < next;) {
            char run[256];
            size_t chunk = next - start < sizeof(run) ? next - start : sizeof(run) - 1;
            for (size_t i = 0; i < chunk; ++i) {
                u8 byte = (u8)line[start + i];
                run[i] = byte >= 32 && byte <= 126 ? (char)byte : ' ';
            }
            run[chunk] = 0;
            ui_renderer_text(renderer, left + (i32)((start - skip) * cell_width), y, run, color);
            start += chunk;
        }

        pos = next;
    }
}
