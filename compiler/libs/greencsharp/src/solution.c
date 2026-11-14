/* Visual Studio solution project records; configuration text never executes. */
#include "internal.h"
#include <ctype.h>

static const char* gcs_solution_string(gcs_compilation* c, const char* path, char** cursor) {
    while (isspace((unsigned char)**cursor)) { ++*cursor; }

    if (**cursor != '"') {
        gcs_error(c, path, 0, "expected quoted solution project field");

        return NULL;
    }

    const char* start = ++*cursor;

    while (**cursor && **cursor != '"') { ++*cursor; }

    if (**cursor != '"') {
        gcs_error(c, path, 0, "unterminated solution project field");

        return NULL;
    }

    const char* value = gcs_copy(c, start, (size_t)(*cursor - start));
    ++*cursor;

    return value;
}

static bool gcs_solution_guid(const char* text) {
    if (!text || strlen(text) != 38 || text[0] != '{' || text[37] != '}') { return false; }

    for (size_t i = 1; i < 37; ++i) {
        bool hyphen = i == 9 || i == 14 || i == 19 || i == 24;

        if ((hyphen && text[i] != '-') || (!hyphen && !isxdigit((unsigned char)text[i]))) { return false; }
    }

    return true;
}

static bool gcs_solution_project(gcs_compilation* c, const char* path, const char* directory, char* line) {
    char* cursor = line + 8;
    const char* kind = gcs_solution_string(c, path, &cursor);

    if (!kind || *cursor++ != ')') { return false; }

    while (isspace((unsigned char)*cursor)) { ++cursor; }

    if (*cursor++ != '=') { return false; }

    const char* name = gcs_solution_string(c, path, &cursor);

    while (isspace((unsigned char)*cursor)) { ++cursor; }

    if (!name || *cursor++ != ',') { return false; }

    const char* relative = gcs_solution_string(c, path, &cursor);

    if (!relative) { return false; }

    while (isspace((unsigned char)*cursor)) { ++cursor; }

    if (*cursor++ != ',') { return false; }

    const char* identity = gcs_solution_string(c, path, &cursor);

    while (isspace((unsigned char)*cursor)) { ++cursor; }

    if (*cursor || !gcs_solution_guid(kind) || !gcs_solution_guid(identity)) { return false; }

    char normalized[39];

    for (size_t i = 0; i < sizeof(normalized); ++i) { normalized[i] = (char)tolower((unsigned char)kind[i]); }

    if (!strcmp(normalized, "{2150e333-8fdc-42a3-9474-1a3956d46de8}")) { return true; }

    if (!gcs_path_suffix(relative, ".csproj")) {
        gcs_error(c, path, 0, "solution contains a non-C# project: %s", relative);

        return false;
    }

    char* project_path = gcs_resolve_path(c, directory, relative);

    return project_path && gcs_load_project(c, project_path, 0);
}

static bool gcs_solution_metadata(const char* line, const char* prefix) {
    return !strncmp(line, prefix, strlen(prefix)) && strchr(line, '=') != NULL;
}

bool gcs_read_solution(gcs_compilation* c, const char* path) {
    char* absolute = gcs_absolute(c, path);
    char* text = absolute ? gcs_read(c, absolute) : NULL;
    const char* directory = absolute ? gcs_directory(c, absolute) : NULL;

    if (!text || !directory) { return false; }

    if (!strncmp(text, "\xef\xbb\xbf", 3)) { text += 3; }

    const char* header = "Microsoft Visual Studio Solution File, Format Version 12.00";
    size_t header_length = strcspn(text, "\r\n");

    if (header_length != strlen(header) || strncmp(text, header, header_length)) {
        gcs_error(c, path, 0, "expected a Visual Studio format 12.00 solution");

        return false;
    }

    char* line = text + header_length;
    bool project_open = false, project_section = false, global_open = false, global_section = false, global_closed = false;

    while (*line && !c->diagnostic[0]) {
        char* end = line;

        while (*end && *end != '\r' && *end != '\n') { ++end; }

        char ending = *end;
        *end = 0;

        while (isspace((unsigned char)*line)) { ++line; }

        char* trailing = end;

        while (trailing > line && isspace((unsigned char)trailing[-1])) { *--trailing = 0; }

        bool valid = true;

        if (!*line || *line == '#') {
            /* Comments and empty records do not change the solution state. */
        } else if (project_section) {
            if (!strcmp(line, "EndProjectSection")) { project_section = false; }
            else { valid = strchr(line, '=') != NULL; }
        } else if (global_section) {
            if (!strcmp(line, "EndGlobalSection")) { global_section = false; }
            else { valid = strchr(line, '=') != NULL; }
        } else if (project_open) {
            if (!strcmp(line, "EndProject")) { project_open = false; }
            else if (gcs_solution_metadata(line, "ProjectSection(")) { project_section = true; }
            else { valid = false; }
        } else if (global_open) {
            if (!strcmp(line, "EndGlobal")) {
                global_open = false;
                global_closed = true;
            }
            else if (gcs_solution_metadata(line, "GlobalSection(")) { global_section = true; }
            else { valid = false; }
        } else if (!global_closed && !strncmp(line, "Project(", 8)) {
            valid = gcs_solution_project(c, absolute, directory, line);
            project_open = valid;
        } else if (!global_closed && !strcmp(line, "Global")) {
            global_open = true;
        } else if (!global_closed && (gcs_solution_metadata(line, "VisualStudioVersion ") ||
                   gcs_solution_metadata(line, "MinimumVisualStudioVersion "))) {
            /* Tool-version metadata does not select a native compilation target. */
        } else {
            valid = false;
        }

        if (!valid) { gcs_error(c, path, 0, "invalid or misplaced solution record: %s", line); }
        if (!ending) { break; }

        line = end + 1;
    }

    if (project_open || project_section || global_open || global_section || !global_closed) {
        gcs_error(c, path, 0, "unterminated solution project or global section");
    }

    return c->projects && !c->diagnostic[0];
}
