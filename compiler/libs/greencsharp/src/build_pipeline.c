/* Native project build entry: toolchain probe, dependency order and explicit output selection. */
#include "internal.h"
#include <ctype.h>

static bool gcs_toolchain_probe(gcs_compilation* c, const char* directory) {
    const char* compiler = getenv("GREENC_CSHARP_CC");
    c->compiler = compiler && *compiler ? compiler : getenv("CC");

    if (!c->compiler || !*c->compiler) { c->compiler = "gcc"; }

    const char* capture = gcs_build_path(c, directory, "obj/greencsharp-toolchain-query.txt");

    if (!capture || !gcs_make_directories(c, capture)) { return false; }

    const char* target_args[] = {c->compiler, "-dumpmachine", NULL};

    if (!gcs_tool(c, target_args, capture)) { return false; }

    char* target = gcs_read(c, capture);

    if (!target) { return false; }

    target[strcspn(target, "\r\n")] = 0;

    if (!*target) { return false; }

    for (const unsigned char* p = (const unsigned char*)target; *p; ++p) {
        if (!isalnum(*p) && *p != '-' && *p != '_' && *p != '.') { return false; }
    }

    c->target = target;
    const char* version_args[] = {c->compiler, "--version", NULL};

    if (!gcs_tool(c, version_args, capture)) { return false; }

    char* version = gcs_read(c, capture);

    if (!version) { return false; }

    u64 identity = gcs_hash_bytes(UINT64_C(14695981039346656037), version, strlen(version));
    identity = gcs_hash_bytes(identity, c->compiler, strlen(c->compiler));
    const char* archiver = getenv("AR");

    if (!archiver || !*archiver) {
        const char* ar_args[] = {c->compiler, "-print-prog-name=ar", NULL};

        if (!gcs_tool(c, ar_args, capture)) { return false; }

        char* resolved = gcs_read(c, capture);

        if (!resolved) { return false; }

        resolved[strcspn(resolved, "\r\n")] = 0;
        archiver = resolved;
    }

    c->archiver = archiver;
    const char* ar_version[] = {c->archiver, "--version", NULL};

    if (!gcs_tool(c, ar_version, capture)) { return false; }

    char* ar_text = gcs_read(c, capture);

    if (!ar_text) { return false; }

    identity = gcs_hash_bytes(identity, ar_text, strlen(ar_text));
    char fingerprint[32];
    snprintf(fingerprint, sizeof(fingerprint), "%016" PRIx64, identity);
    c->toolchain = gcs_copy(c, fingerprint, strlen(fingerprint));

    return c->toolchain && gcs_remove_file(c, capture);
}

static bool gcs_export_output(gcs_compilation* c, const char* destination) {
    if (!c->result.output_path || !gcs_make_directories(c, destination)) { return false; }
    if (!gcs_path_compare(c->result.output_path, destination)) { return true; }

    char* temporary = gcs_join(c, destination, ".tmp");
    FILE* input = fopen(c->result.output_path, "rb");
    FILE* output = temporary ? fopen(temporary, "wb") : NULL;
    bool success = input && output;
    u8 buffer[4096];
    size_t count;

    while (success && (count = fread(buffer, 1, sizeof(buffer), input)) != 0) {
        success = fwrite(buffer, 1, count, output) == count;
    }

    if (input) {
        success = success && !ferror(input);
        fclose(input);
    }

    if (output && fclose(output)) { success = false; }

    if (success) { success = gcs_replace_file(c, temporary, destination); }
    if (success) { c->result.output_path = gcs_copy(c, destination, strlen(destination)); }

    return success;
}

bool gcs_native_build(gcs_compilation* c, const greencsharp_request* request) {
    c->planning = true;
    c->configuration = request->configuration && *request->configuration ? request->configuration : "Release";

    if (strcmp(c->configuration, "Release") && strcmp(c->configuration, "Debug")) {
        gcs_error(c, request->input_path, 0, "supported native configurations are Debug and Release");

        return false;
    }

    const char* path = gcs_absolute(c, request->input_path);
    const char* directory = path ? gcs_directory(c, path) : NULL;

    if (!directory || !gcs_toolchain_probe(c, directory) || !gcs_load_input(c, path)) { return false; }

    gcs_project* selected = NULL;

    for (gcs_project* project = c->projects; project; project = project->next) {
        if (!gcs_project_layout(c, project)) { return false; }

        for (gcs_project* other = project->next; other; other = other->next) {
            if (!gcs_path_compare(project->assembly, other->assembly)) {
                gcs_error(c, project->path, 0, "duplicate AssemblyName '%s' in project graph", project->assembly);

                return false;
            }
        }

        if (!gcs_path_compare(project->path, path)) { selected = project; }
    }

    bool success = true;

    if (request->output_path && request->mode == GCS_CLEAN) {
        gcs_error(c, path, 0, "--clean does not accept an output path");

        return false;
    }

    if (request->output_path && !selected) {
        gcs_error(c, path, 0, "solution builds publish each project; select a project when using -o");

        return false;
    }

    if (request->output_path && request->mode == GCS_EMIT_C) {
        size_t units = 0;

        for (gcs_source* source = c->sources; source; source = source->next) {
            if (source->project == selected) { ++units; }
        }

        if (units != 1) {
            gcs_error(c, path, 0, "-o with --emit-c requires one source unit; omit -o to retain all generated units");

            return false;
        }
    }

    if (selected) {
        success = gcs_build_project(c, selected, request->mode);
    } else {
        for (gcs_project* project = c->projects; project && success; project = project->next) {
            success = gcs_build_project(c, project, request->mode == GCS_EMIT_C && !project->executable ? GCS_BUILD : request->mode);
        }
    }

    if (success && request->mode == GCS_CLEAN) { c->result.output_path = path; }
    if (success && request->output_path) { success = gcs_export_output(c, request->output_path); }

    return success && !c->diagnostic[0];
}
