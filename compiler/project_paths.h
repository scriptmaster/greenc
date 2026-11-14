/*
 ** project_paths.h - 5 functions.
 ** greenc_project_append - Append text while retaining ownership on allocation failure.
 ** greenc_project_quote - Append a quoted native compiler argument.
 ** greenc_project_path - Resolve and normalize a Windows path.
 ** greenc_project_exists - Test whether a path has the requested directory kind.
 ** greenc_project_run - Run a native tool and propagate its failure.
 ** See also: project_build.h, project_sources.h, project_options.h.
 */
#pragma once

static bool greenc_project_append(char** text, const char* addition) {
    char* combined = greenc_concat(*text ? *text : "", addition);

    if (!combined) { return false; }

    greenc_free(*text);
    *text = combined;

    return true;
}

static bool greenc_project_quote(char** text, const char* argument) {
    for (const char* c = argument; *c; ++c) {
        if (*c == '"' || *c == '\r' || *c == '\n') { return false; }
    }

    return greenc_project_append(text, "\"") &&
           greenc_project_append(text, argument) && greenc_project_append(text, "\"\n");
}

static char* greenc_project_path(const char* input) {
    char absolute[1024];
    DWORD length = GetFullPathNameA(input, sizeof(absolute), absolute, NULL);

    if (!length || length >= sizeof(absolute)) { return NULL; }

    for (DWORD i = 0; i < length; ++i) {
        if (absolute[i] == '"' || absolute[i] == '\r' || absolute[i] == '\n') { return NULL; }

        if (absolute[i] == '\\') { absolute[i] = '/'; }
    }

    return greenc_copy(absolute);
}

static bool greenc_project_exists(const char* path, bool directory) {
    DWORD attributes = GetFileAttributesA(path);

    return attributes != INVALID_FILE_ATTRIBUTES &&
           !!(attributes & FILE_ATTRIBUTE_DIRECTORY) == directory;
}

static int greenc_project_run(const char* command) {
    print(command);
    int result = greenc_exec_wait(command);

    if (result == -1) {
        print("greenc: cannot start native build tool; check PATH\n");

        return 1;
    }

    return result;
}
