/*
 ** project_libraries.h - 4 functions.
 ** greenc_library_source - Resolve a library-owned implementation relative to its header.
 ** greenc_library_manifest - Read source and link declarations owned by a library.
 ** greenc_library_header - Select a manifest or a conventional sibling implementation.
 ** greenc_libraries_discover - Follow active transitive dependencies and collect native link inputs.
 ** See also: project_dependencies.h, project_sources.h, project_build.h.
 */
#pragma once

static bool greenc_library_source(greenc_library_graph* graph, const char* header, const char* relative) {
    char* directory = greenc_dirname_alloc(header);
    char* joined = directory ? greenc_concat(directory, "/", relative) : NULL;
    char* source = joined ? greenc_project_path(joined) : NULL;
    bool success = source && greenc_library_contains(graph, source) &&
                   greenc_string_endswith(source, ".c") &&
                   greenc_project_exists(source, false) && greenc_dependency_add(graph, source);
    greenc_free(source);
    greenc_free(joined);
    greenc_free(directory);

    return success;
}

static bool greenc_library_manifest(greenc_library_graph* graph, const char* header, const char* manifest) {
    char* content = greenc_read_file(manifest);

    if (!content) { return false; }

    char* line = content;
    bool success = true;

    while (*line && success) {
        char* end = line;

        while (*end && *end != '\r' && *end != '\n') { ++end; }

        char ending = *end;
        *end = 0;

        if (greenc_string_startswith(line, "source ")) {
            success = greenc_library_source(graph, header, line + 7);
        } else if (greenc_string_startswith(line, "link ")) {
            success = greenc_project_append(&graph->link_arguments, line + 5) &&
                      greenc_project_append(&graph->link_arguments, "\n");
        } else if (*line && *line != '#') {
            print("greenc: invalid library source manifest\n");
            print(manifest);
            success = false;
        }

        if (!ending) { break; }

        line = end + 1;
    }

    greenc_free(content);

    return success;
}

static bool greenc_library_header(greenc_library_graph* graph, const char* header) {
    char* stem = greenc_copy(header);

    if (!stem) { return false; }

    stem[greenc_strlen(stem) - 2] = 0;
    char* manifest = greenc_concat(stem, ".sources");
    char* platform = greenc_concat(stem, "_win32.c");
    char* portable = greenc_concat(stem, ".c");
    bool success = manifest && platform && portable;

    if (success && greenc_project_exists(manifest, false)) {
        success = greenc_library_manifest(graph, header, manifest);
    } else if (success && greenc_project_exists(platform, false)) {
        success = greenc_dependency_add(graph, platform);
    } else if (success && greenc_project_exists(portable, false)) {
        success = greenc_dependency_add(graph, portable);
    }

    greenc_free(portable);
    greenc_free(platform);
    greenc_free(manifest);
    greenc_free(stem);

    return success;
}

static bool greenc_libraries_discover(greenc_library_graph* graph) {
    while (graph->processed < graph->count) {
        const char* path = graph->paths[graph->processed++];

        if (greenc_string_endswith(path, ".h")) {
            if (!greenc_library_header(graph, path)) { return false; }
        } else if (greenc_string_endswith(path, ".c")) {
            if (!greenc_dependencies_probe(graph, path) ||
                !greenc_project_quote(&graph->native_arguments, path)) { return false; }
        }
    }

    return true;
}
