/* Project-local .c/.h generation keeps dependency implementation bodies out of consumers. */
#include "internal.h"

bool gcs_emit_project_headers(gcs_compilation* c, gcs_project* project) {
    char* include = gcs_build_path(c, project->object_directory, "include");
    char* runtime = include ? gcs_build_path(c, include, "greenc_runtime.h") : NULL;
    char* header_name = gcs_join(c, project->assembly, ".h");
    char* public_path = header_name && include ? gcs_build_path(c, include, header_name) : NULL;
    char* private_name = gcs_join(c, project->assembly, ".internal.h");
    char* private_path = private_name && include ? gcs_build_path(c, include, private_name) : NULL;
    gcs_buffer public_text = {0}, private_text = {0};
    gcs_append(c, &public_text, "/* Public native contract for %s. */\n#pragma once\n#include \"greenc_runtime.h\"\n\n", project->assembly);
    gcs_append(c, &private_text, "/* Private project declarations; not a consumer access grant. */\n#pragma once\n#include \"%s\"\n", header_name);

    for (gcs_project* dependency = c->projects; dependency; dependency = dependency->next) {
        if (dependency == project) { continue; }

        gcs_append(c, &private_text, "#include \"%s.h\"\n", dependency->assembly);
    }

    for (gcs_method* method = c->methods; method; method = method->next) {
        if (method->source->project != project) { continue; }

        gcs_emit_signature(c, method, &private_text);
        gcs_append(c, &private_text, ";\n");

        if (method->owner && method->owner->exported && method->access == 2) {
            gcs_emit_signature(c, method, &public_text);
            gcs_append(c, &public_text, ";\n");
        }
    }

    gcs_emit_runtime(c);
    gcs_buffer runtime_text = {0};
    gcs_append(c, &runtime_text, "#pragma once\n#ifndef GREENC_CSHARP_RUNTIME_V2\n#define GREENC_CSHARP_RUNTIME_V2\n%s\n#endif\n", c->program.data);
    bool success = runtime && public_path && private_path && gcs_write_text(c, runtime, runtime_text.data) &&
                   gcs_write_text(c, public_path, public_text.data) && gcs_write_text(c, private_path, private_text.data);
    if (success) {
        gcs_record_artifact(c, runtime);
        gcs_record_artifact(c, public_path);
        gcs_record_artifact(c, private_path);
    }

    free(c->program.data);
    c->program = (gcs_buffer){0};
    free(runtime_text.data);
    free(public_text.data);
    free(private_text.data);

    return success;
}

static bool gcs_emit_source(gcs_compilation* c, gcs_project* project, gcs_source* source) {
    const char* relative = source->path;
    size_t directory_length = strlen(project->directory);

    if (!strncmp(relative, project->directory, directory_length) && relative[directory_length] == '/') { relative += directory_length + 1; }

    const char* identity = gcs_symbol(c, "unit", relative);
    const char* basename = strrchr(relative, '/');
    basename = basename ? basename + 1 : relative;
    char* file = gcs_join(c, identity, "_");
    file = file ? gcs_join(c, file, basename) : NULL;
    char* generated_directory = gcs_build_path(c, project->object_directory, "generated");
    char* object_directory = gcs_build_path(c, project->object_directory, "objects");
    if (relative != source->path && strrchr(relative, '/')) {
        const char* folder = gcs_copy(c, relative, (size_t)(strrchr(relative, '/') - relative));
        generated_directory = folder ? gcs_build_path(c, generated_directory, folder) : NULL;
        object_directory = folder ? gcs_build_path(c, object_directory, folder) : NULL;
    }

    char* generated_name = file ? gcs_join(c, file, ".c") : NULL;
    char* object_name = file ? gcs_join(c, file, ".o") : NULL;
    char* generated = generated_name && generated_directory ? gcs_build_path(c, generated_directory, generated_name) : NULL;
    char* object = object_name && object_directory ? gcs_build_path(c, object_directory, object_name) : NULL;
    char* include = gcs_build_path(c, project->object_directory, "include");

    if (!generated || !object || !include) { return false; }

    for (gcs_object* previous = c->objects; previous; previous = previous->next) {
        if (!gcs_path_compare(previous->path, object)) {
            gcs_error(c, source->path, 0, "native source-unit path collision");

            return false;
        }
    }

    gcs_append(c, &c->program, "/* One C# source unit; dependency implementations are compiled separately. */\n#include \"%s.internal.h\"\n\n", project->assembly);

    for (gcs_method* method = c->methods; method; method = method->next) {
        if (method->source == source) { gcs_emit_method(c, method); }
    }

    if (c->entry && c->entry->source == source) { gcs_emit_entry(c); }

    bool success = gcs_write_text(c, generated, c->program.data);

    if (success && c->build_mode != GCS_EMIT_C) { success = gcs_object_build(c, generated, object, include, c->build_mode == GCS_REBUILD); }

    if (success) {
        gcs_object* item = gcs_allocate(c, sizeof(*item));

        if (!item) { return false; }

        item->path = object;
        item->next = c->objects;
        c->objects = item;
        c->result.output_path = generated;
        gcs_record_artifact(c, generated);

        if (c->build_mode != GCS_EMIT_C) {
            gcs_record_artifact(c, object);
            gcs_record_artifact(c, gcs_join(c, object, ".dependencies.h"));
            gcs_record_artifact(c, gcs_join(c, object, ".cache.h"));
        }
    }

    free(c->program.data);
    c->program = (gcs_buffer){0};

    return success;
}

bool gcs_emit_project(gcs_compilation* c, gcs_project* project) {
    if (!gcs_emit_project_headers(c, project)) { return false; }

    for (gcs_source* source = c->sources; source; source = source->next) {
        if (source->project == project && !source->imported && !gcs_emit_source(c, project, source)) { return false; }
    }

    return !c->diagnostic[0];
}
