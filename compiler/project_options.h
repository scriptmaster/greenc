/*
 ** project_options.h - 4 functions.
 ** greenc_project_libraries - Locate bundled native library sources beside the compiler tree.
 ** greenc_project_publish - Atomically replace a prepared companion artifact.
 ** greenc_project_option - Apply an entry-point build annotation.
 ** greenc_project_options - Read build annotations from the entry point.
 ** See also: project_build.h, project_sources.h, project_paths.h.
 */
#pragma once

static char* greenc_project_libraries(void) {
    char executable[1024];
    DWORD length = GetModuleFileNameA(NULL, executable, sizeof(executable));

    if (!length || length >= sizeof(executable)) { return NULL; }

    char* directory = greenc_dirname_alloc(executable);
    char* libraries = NULL;

    for (size_t depth = 0; directory && depth < 16; ++depth) {
        char* candidate = greenc_concat(directory, "/compiler/libs");

        if (candidate && greenc_project_exists(candidate, true)) {
            libraries = greenc_project_path(candidate);
            greenc_free(candidate);

            break;
        }

        greenc_free(candidate);
        char* parent = greenc_dirname_alloc(directory);
        bool unchanged = parent && greenc_streq(parent, directory);
        greenc_free(directory);
        directory = parent;

        if (unchanged) { break; }
    }

    greenc_free(directory);

    return libraries;
}

static bool greenc_project_publish(const char* paths) {
    char* source = greenc_copy(paths);

    if (!source) { return false; }

    char* separator = source;

    while (*separator && *separator != '|') { ++separator; }

    bool success = false;

    if (*separator) {
        *separator = 0;
        success = MoveFileExA(source, separator + 1, MOVEFILE_REPLACE_EXISTING) != 0;
    }

    greenc_free(source);

    return success;
}

static bool greenc_project_option(greenc_project* project, const char* build_option, const char* libraries) {
    if (greenc_string_startswith(build_option, "native ")) {
        char* joined = greenc_concat(libraries, "/", build_option + 7);
        char* path = joined ? greenc_project_path(joined) : NULL;
        bool success = path && greenc_library_contains(project->libraries, path) &&
                       greenc_string_endswith(path, ".c") && greenc_project_exists(path, false) &&
                       greenc_dependency_add(project->libraries, path);
        greenc_free(path);
        greenc_free(joined);

        return success;
    }

    if (greenc_string_startswith(build_option, "link ")) {
        return greenc_project_append(&project->link_arguments, build_option + 5) &&
               greenc_project_append(&project->link_arguments, "\n");
    }

    if (greenc_string_startswith(build_option, "prepare ")) {
        return greenc_project_run(build_option + 8) == 0;
    }

    if (greenc_string_startswith(build_option, "publish ")) {
        return greenc_project_publish(build_option + 8);
    }

    if (greenc_string_startswith(build_option, "resource ")) {
        if (project->resource_compiled) {
            print("greenc: only one resource annotation per application is supported\n");

            return false;
        }

        char* command = greenc_concat("windres -I . -i \"", build_option + 9,
                                     "\" -o build/greenc/resource.o");
        bool success = command && greenc_project_run(command) == 0 &&
                       greenc_project_quote(&project->arguments, "build/greenc/resource.o");
        greenc_free(command);
        project->resource_compiled = success;

        return success;
    }

    print("greenc: unknown entry-point build annotation\n");
    print(build_option);

    return false;
}

static bool greenc_project_options(greenc_project* project, const char* entry, const char* libraries) {
    char* source = greenc_read_file(entry);

    if (!source) { return false; }

    bool success = true;
    char* line = source;
    while (*line && success) {
        char* end = line;

        while (*end && *end != '\n' && *end != '\r') { ++end; }

        char ending = *end;
        *end = 0;

        if (greenc_string_startswith(line, "// greenc: ")) {
            success = greenc_project_option(project, line + 11, libraries);
        }

        if (!ending) { break; }

        line = end + 1;
    }

    greenc_free(source);

    return success;
}
