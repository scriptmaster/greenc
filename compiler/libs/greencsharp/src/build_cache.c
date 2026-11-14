/* Content fingerprints cover emitted code, actual include dependencies and toolchain profiles. */
#include "internal.h"

bool gcs_hash_file(gcs_compilation* c, const char* path, u64* hash) {
    FILE* file = fopen(path, "rb");

    if (!file) { return false; }

    u8 buffer[4096];
    size_t count;

    while ((count = fread(buffer, 1, sizeof(buffer), file)) != 0) { *hash = gcs_hash_bytes(*hash, buffer, count); }

    bool success = !ferror(file);
    fclose(file);

    if (!success) { gcs_error(c, path, 0, "cannot fingerprint build dependency"); }

    return success;
}

bool gcs_stamp_matches(gcs_compilation* c, const char* path, u64 hash) {
    if (!gcs_file_exists(path)) { return false; }

    char* text = gcs_read_data(c, path);
    char expected[32];
    snprintf(expected, sizeof(expected), "%016" PRIx64 "\n", hash);

    return text && !strcmp(text, expected);
}

bool gcs_stamp_write(gcs_compilation* c, const char* path, u64 hash) {
    char text[32];
    snprintf(text, sizeof(text), "%016" PRIx64 "\n", hash);

    return gcs_write_data(c, path, text);
}

static bool gcs_dependency_hash(gcs_compilation* c, const char* file, u64* hash) {
    if (!gcs_file_exists(file)) { return false; }

    char* text = gcs_read_data(c, file);

    if (!text) { return false; }

    char* cursor = strchr(text, ':');

    if (!cursor) { return false; }

    ++cursor;

    while (*cursor) {
        while (*cursor && (unsigned char)*cursor <= ' ') { ++cursor; }

        char* token = gcs_allocate(c, strlen(cursor) + 1);

        if (!token) { return false; }

        char* write = token;

        while (*cursor && (unsigned char)*cursor > ' ') {
            if (*cursor == '\\' && (cursor[1] == '\n' || (cursor[1] == '\r' && cursor[2] == '\n'))) {
                cursor += cursor[1] == '\r' ? 3 : 2;

                continue;
            }

            if ((*cursor == '\\' && (cursor[1] == ' ' || cursor[1] == '\t' || cursor[1] == '#' || cursor[1] == '\\')) ||
                (*cursor == '$' && cursor[1] == '$')) { ++cursor; }

            *write++ = *cursor++;
        }

        if (*token && !gcs_hash_file(c, token, hash)) { return false; }
    }

    return true;
}

bool gcs_object_build(gcs_compilation* c, const char* source, const char* object, const char* include, bool force) {
    char* dependency = gcs_join(c, object, ".dependencies.h");
    char* raw_dependency = gcs_join(c, object, ".dependency.tmp.h");
    char* stamp = gcs_join(c, object, ".cache.h");
    char* temporary = gcs_join(c, object, ".tmp.o");
    /* Two include arguments per project (at most 128), plus compiler options. */
    const char* arguments[300];
    size_t count = 0;
    arguments[count++] = c->compiler;
    arguments[count++] = "-std=c17";
    arguments[count++] = "-Wall";
    arguments[count++] = "-Wextra";
    arguments[count++] = "-Werror";
    bool debug = !strcmp(c->configuration, "Debug");
    arguments[count++] = debug ? "-O0" : "-Os";
    if (debug) { arguments[count++] = "-g"; }
    arguments[count++] = "-I";
    arguments[count++] = include;

    for (gcs_project* project = c->projects; project; project = project->next) {
        if (!gcs_project_layout(c, project)) { return false; }

        arguments[count++] = "-I";
        arguments[count++] = project->output_directory;
    }

    arguments[count++] = "-MMD";
    arguments[count++] = "-MT";
    arguments[count++] = "greencsharp";
    arguments[count++] = "-MF";
    arguments[count++] = raw_dependency;
    arguments[count++] = "-c";
    arguments[count++] = source;
    arguments[count++] = "-o";
    arguments[count++] = temporary;
    arguments[count] = NULL;
    gcs_buffer options = {0};

    for (size_t i = 0; i < count; ++i) { gcs_response_argument(c, &options, arguments[i]); }

    u64 base = gcs_hash_bytes(UINT64_C(14695981039346656037), c->toolchain, strlen(c->toolchain));
    base = gcs_hash_bytes(base, options.data, options.length);
    free(options.data);
    if (gcs_path_suffix(c->root_path, ".csproj") && !gcs_hash_file(c, c->root_path, &base)) { return false; }

    for (gcs_project* project = c->projects; project; project = project->next) {
        if (gcs_path_compare(project->path, c->root_path) && !gcs_hash_file(c, project->contract, &base)) { return false; }
    }

    bool source_valid = gcs_hash_file(c, source, &base);
    u64 fingerprint = base;
    bool cached = !force && source_valid && gcs_file_exists(object) && gcs_dependency_hash(c, dependency, &fingerprint) && gcs_stamp_matches(c, stamp, fingerprint);

    if (cached) { return true; }

    bool success = source_valid && gcs_make_directories(c, object) && gcs_tool(c, arguments, NULL);
    char* text = success ? gcs_read(c, raw_dependency) : NULL;
    success = success && text && gcs_write_data(c, dependency, text) && gcs_replace_file(c, temporary, object);
    bool removed = gcs_remove_file(c, raw_dependency);
    fingerprint = base;

    return success && removed && gcs_dependency_hash(c, dependency, &fingerprint) && gcs_stamp_write(c, stamp, fingerprint);
}
