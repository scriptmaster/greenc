/*
 ** project_sources.h - 5 functions.
 ** greenc_project_scan - Collect source files and header search directories recursively.
 ** greenc_project_contracts - Load the source tree's ownership contract file.
 ** greenc_project_compile_source - Compile one lowered source with its original quote-include directory.
 ** greenc_project_emit - Check and lower one application source for the native compiler.
 ** greenc_project_sources - Emit the collected sources in deterministic path order.
 ** See also: project_build.h, project_paths.h, lang/hosted_module.h.
 */
#pragma once

typedef struct {
    char* paths[1024];
    size_t count;
    char* arguments;
    char* compile_arguments;
    char* link_arguments;
    char* contracts_path;
    char* contracts;
    const char* source_root;
    bool resource_compiled;
    greenc_library_graph* libraries;
} greenc_project;

static bool greenc_project_scan(greenc_project* project, const char* directory, size_t depth) {
    if (depth > 64) { return false; }

    if (!greenc_project_append(&project->arguments, "-I") ||
        !greenc_project_quote(&project->arguments, directory)) { return false; }

    char* pattern = greenc_concat(directory, "/*");

    if (!pattern) { return false; }

    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(pattern, &entry);
    greenc_free(pattern);

    if (search == INVALID_HANDLE_VALUE) { return GetLastError() == ERROR_FILE_NOT_FOUND; }

    bool success = true;
    do {
        if (entry.cFileName[0] == '.' || (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) { continue; }

        char* path = greenc_concat(directory, "/", entry.cFileName);

        if (!path) {
            success = false;

            break;
        }

        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!greenc_streq(entry.cFileName, "bin") && !greenc_streq(entry.cFileName, "build") &&
                !greenc_streq(entry.cFileName, "test") && !greenc_streq(entry.cFileName, "tests")) {
                success = greenc_project_scan(project, path, depth + 1);
            }

            greenc_free(path);
        } else if (greenc_string_endswith(path, ".c") || greenc_string_endswith(path, ".gc")) {
            if (project->count == 1024) {
                greenc_free(path);
                success = false;
            } else {
                project->paths[project->count++] = path;
            }
        } else if (greenc_string_endswith(path, ".ownership")) {
            if (project->contracts_path) {
                print("greenc: only one ownership contract file per source tree is supported\n");
                greenc_free(path);
                success = false;
            } else {
                project->contracts_path = path;
            }
        } else {
            greenc_free(path);
        }

        if (!success) { break; }
    } while (FindNextFileA(search, &entry));

    if (success && GetLastError() != ERROR_NO_MORE_FILES) { success = false; }

    FindClose(search);

    return success;
}

static bool greenc_project_contracts(greenc_project* project) {
    if (!project->contracts_path) { return true; }

    project->contracts = greenc_read_file(project->contracts_path);

    return project->contracts && contract_validate(project->contracts);
}

static bool greenc_project_compile_source(greenc_project* project, const char* source, const char* generated) {
    char* directory = greenc_dirname_alloc(source);
    char* object = greenc_concat(generated, ".o");
    char* arguments = greenc_copy(project->compile_arguments);
    bool success = directory && object && arguments &&
                   greenc_project_append(&arguments, "-iquote\n") &&
                   greenc_project_quote(&arguments, directory) &&
                   greenc_project_append(&arguments, "-MMD -MT greenc -MF build/greenc/dependencies.d\n-c\n") &&
                   greenc_project_quote(&arguments, generated) &&
                   greenc_project_append(&arguments, "-o\n") &&
                   greenc_project_quote(&arguments, object) &&
                   greenc_write_file("build/greenc/compile.rsp", arguments) == 0 &&
                   greenc_exec_wait("gcc @build/greenc/compile.rsp") == 0 &&
                   greenc_dependencies_read(project->libraries) &&
                   greenc_project_quote(&project->arguments, object);

    if (!success) { print(source); }

    greenc_free(arguments);
    greenc_free(object);
    greenc_free(directory);

    return success;
}

static bool greenc_project_emit(greenc_project* project, const char* path) {
    char* source = greenc_read_file(path);

    if (!source) { return false; }

    bool valid = greenc_check_hosted_source(source, path, project->contracts);
    char* output = valid ? greenc_concat("build/greenc/", path + greenc_strlen(project->source_root) + 1, ".c") : NULL;
    char* mapped = output ? greenc_concat("#line 1 \"", path, "\"\n", source) : NULL;
    greenc_free(source);

    if (output) { greenc_ensure_output_dir(output); }

    bool success = mapped && greenc_write_file(output, mapped) == 0 &&
                   greenc_project_compile_source(project, path, output);
    greenc_free(mapped);
    greenc_free(output);

    return success;
}

static bool greenc_project_sources(greenc_project* project) {
    if (!project->count || !greenc_project_contracts(project)) { return false; }

    project->compile_arguments = greenc_copy(project->arguments);

    if (!project->compile_arguments) { return false; }

    project->libraries->compiler_arguments = project->compile_arguments;

    for (size_t i = 1; i < project->count; ++i) {
        char* path = project->paths[i];
        size_t position = i;

        while (position && greenc_strcmp(project->paths[position - 1], path) > 0) {
            project->paths[position] = project->paths[position - 1];
            --position;
        }

        project->paths[position] = path;
    }

    for (size_t i = 0; i < project->count; ++i) {
        if (!greenc_project_emit(project, project->paths[i])) { return false; }
    }

    return true;
}
