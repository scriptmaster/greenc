/*
 ** project_dependencies.h - 5 functions.
 ** greenc_library_contains - Check a normalized path against the bundled library boundary.
 ** greenc_dependency_add - Remember a normalized bundled-library dependency once.
 ** greenc_dependency_token - Decode a filename from native Make-format dependency output.
 ** greenc_dependencies_read - Queue active library includes reported by the preprocessor.
 ** greenc_dependencies_probe - Find transitive includes of a selected native implementation.
 ** See also: project_libraries.h, project_sources.h, project_paths.h.
 */
#pragma once

typedef struct {
    const char* root;
    const char* compiler_arguments;
    char* paths[2048];
    size_t count;
    size_t processed;
    char* native_arguments;
    char* link_arguments;
} greenc_library_graph;

static bool greenc_library_contains(const greenc_library_graph* graph, const char* path) {
    size_t root_length = greenc_strlen(graph->root);

    return greenc_strlen(path) > root_length && path[root_length] == '/' &&
           CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, path, (int)root_length,
                          graph->root, (int)root_length) == CSTR_EQUAL;
}

static bool greenc_dependency_add(greenc_library_graph* graph, const char* input) {
    char* path = greenc_project_path(input);

    if (!path) { return false; }

    if (!greenc_library_contains(graph, path)) {
        greenc_free(path);

        return true;
    }

    for (size_t i = 0; i < graph->count; ++i) {
        if (!lstrcmpiA(graph->paths[i], path)) {
            greenc_free(path);

            return true;
        }
    }

    if (graph->count == 2048) {
        print("greenc: too many library dependencies\n");
        greenc_free(path);

        return false;
    }

    graph->paths[graph->count++] = path;

    return true;
}

static char* greenc_dependency_token(char** cursor) {
    char* read = *cursor;

    while (*read && (unsigned char)*read <= ' ') { ++read; }

    char* token = (char*)greenc_malloc(greenc_strlen(read) + 1);

    if (!token) { return NULL; }

    char* write = token;
    while (*read && (unsigned char)*read > ' ') {
        if (*read == '\\' && read[1] == '\n') {
            read += 2;

            continue;
        }

        if (*read == '\\' && read[1] == '\r' && read[2] == '\n') {
            read += 3;

            continue;
        }

        if (*read == '\\' && (read[1] == ' ' || read[1] == '\t' || read[1] == '#' || read[1] == '\\')) {
            ++read;
        } else if (*read == '$' && read[1] == '$') {
            ++read;
        }

        *write++ = *read++;
    }

    *write = 0;
    *cursor = read;

    return token;
}

static bool greenc_dependencies_read(greenc_library_graph* graph) {
    char* dependencies = greenc_read_file("build/greenc/dependencies.d");

    if (!dependencies) { return false; }

    char* cursor = dependencies;

    while (*cursor && *cursor != ':') { ++cursor; }

    bool success = *cursor == ':';

    if (success) { ++cursor; }

    while (*cursor && success) {
        char* token = greenc_dependency_token(&cursor);
        success = token && (!*token || greenc_dependency_add(graph, token));
        greenc_free(token);
    }

    greenc_free(dependencies);

    return success;
}

static bool greenc_dependencies_probe(greenc_library_graph* graph, const char* source) {
    char* arguments = greenc_copy(graph->compiler_arguments);
    bool success = arguments &&
                   greenc_project_append(&arguments, "-MM -MT greenc -MF build/greenc/dependencies.d\n") &&
                   greenc_project_quote(&arguments, source) &&
                   greenc_write_file("build/greenc/dependencies.rsp", arguments) == 0 &&
                   greenc_project_run("gcc @build/greenc/dependencies.rsp") == 0 &&
                   greenc_dependencies_read(graph);
    greenc_free(arguments);

    return success;
}
