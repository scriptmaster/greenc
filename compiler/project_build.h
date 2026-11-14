/*
 ** project_build.h - 3 functions.
 ** greenc_project_entry - Find main.c at a supplied file or project directory.
 ** greenc_project_compile - Discover, check, emit and link a hosted application.
 ** greenc_build_project - Run a project build with explicit ownership and cleanup.
 ** See also: main.c, project_paths.h, project_sources.h, project_options.h, project_link.h.
 */
#pragma once

#ifdef _WIN32
#include "project_paths.h"
#include "project_dependencies.h"
#include "project_libraries.h"
#include "project_sources.h"
#include "project_options.h"
#include "project_link.h"

static char* greenc_project_entry(const char* input) {
    if (greenc_project_exists(input, false)) { return greenc_project_path(input); }

    char* direct = greenc_concat(input, "/main.c");
    char* nested = greenc_concat(input, "/src/main.c");
    char* entry = direct && greenc_project_exists(direct, false) ? greenc_project_path(direct) :
                  nested && greenc_project_exists(nested, false) ? greenc_project_path(nested) : NULL;
    greenc_free(direct);
    greenc_free(nested);

    return entry;
}

static bool greenc_project_compile(greenc_project* project, const char* entry, const char* libraries) {
    char* output = greenc_project_output();

    if (!output) { return false; }

    greenc_ensure_output_dir(output);
    greenc_ensure_output_dir("build/greenc/link.rsp");
    bool success = greenc_project_append(&project->arguments, "-Os -std=c17 -Wall -Wextra -Werror\n-I") &&
                   greenc_project_quote(&project->arguments, libraries) &&
                   greenc_project_scan(project, project->source_root, 0) &&
                   greenc_project_sources(project) &&
                   greenc_project_options(project, entry, libraries) &&
                   greenc_libraries_discover(project->libraries) &&
                   greenc_project_link(project, output);

    if (success) { print(output); }

    greenc_free(output);

    return success;
}

static int greenc_build_project(const char* input) {
    char previous_directory[1024];
    DWORD length = GetCurrentDirectoryA(sizeof(previous_directory), previous_directory);
    char* entry = greenc_project_entry(input);
    char* source_root = entry ? greenc_dirname_alloc(entry) : NULL;
    char* parent = source_root ? greenc_dirname_alloc(source_root) : NULL;
    char* libraries = greenc_project_libraries();
    greenc_project* project = (greenc_project*)greenc_malloc(sizeof(*project));
    greenc_library_graph* graph = (greenc_library_graph*)greenc_malloc(sizeof(*graph));

    if (graph) {
        graph->root = libraries;
        graph->compiler_arguments = NULL;
        graph->count = 0;
        graph->processed = 0;
        graph->native_arguments = NULL;
        graph->link_arguments = NULL;
    }

    if (project) {
        project->count = 0;
        project->arguments = NULL;
        project->compile_arguments = NULL;
        project->link_arguments = NULL;
        project->contracts_path = NULL;
        project->contracts = NULL;
        project->source_root = source_root;
        project->resource_compiled = false;
        project->libraries = graph;
    }

    size_t source_length = greenc_strlen(source_root);
    bool in_source_directory = source_length >= 4 && lstrcmpiA(source_root + source_length - 4, "/src") == 0;
    const char* root = in_source_directory ? parent : source_root;
    bool changed_directory = length && length < sizeof(previous_directory) && root &&
                             SetCurrentDirectoryA(root);
    bool success = changed_directory && libraries && project && graph && greenc_project_compile(project, entry, libraries);

    if (changed_directory && !SetCurrentDirectoryA(previous_directory)) { success = false; }

    for (size_t i = 0; project && i < project->count; ++i) {
        greenc_free(project->paths[i]);
    }

    if (project) {
        greenc_free(project->arguments);
        greenc_free(project->compile_arguments);
        greenc_free(project->link_arguments);
        greenc_free(project->contracts_path);
        greenc_free(project->contracts);
        greenc_free(project);
    }

    for (size_t i = 0; graph && i < graph->count; ++i) {
        greenc_free(graph->paths[i]);
    }

    if (graph) {
        greenc_free(graph->native_arguments);
        greenc_free(graph->link_arguments);
        greenc_free(graph);
    }

    greenc_free(libraries);
    greenc_free(parent);
    greenc_free(source_root);
    greenc_free(entry);

    if (!success) { print("greenc: project build failed (requires main.c or src/main.c and native tools on PATH)\n"); }

    return success ? 0 : 1;
}
#endif
