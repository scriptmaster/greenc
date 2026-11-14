/* Project identity and collision-safe target/configuration paths. */
#include "internal.h"
#include <ctype.h>

u64 gcs_hash_bytes(u64 hash, const void* data, size_t size) {
    const u8* bytes = data;

    for (size_t i = 0; i < size; ++i) { hash = (hash ^ bytes[i]) * UINT64_C(1099511628211); }

    return hash;
}

const char* gcs_symbol(gcs_compilation* c, const char* domain, const char* name) {
    u64 hash = gcs_hash_bytes(UINT64_C(14695981039346656037), name, strlen(name));
    char text[64];
    snprintf(text, sizeof(text), "gcs_%s_%016" PRIx64, domain, hash);

    return gcs_copy(c, text, strlen(text));
}

const char* gcs_assembly_name(gcs_compilation* c, const char* path) {
    const char* name = strrchr(path, '/');
    name = name ? name + 1 : path;
    const char* dot = strrchr(name, '.');

    return gcs_copy(c, name, dot ? (size_t)(dot - name) : strlen(name));
}

char* gcs_build_path(gcs_compilation* c, const char* directory, const char* name) {
    char* prefix = gcs_join(c, directory, "/");

    return prefix ? gcs_join(c, prefix, name) : NULL;
}

bool gcs_project_layout(gcs_compilation* c, gcs_project* project) {
    if (!project->assembly) { project->assembly = gcs_assembly_name(c, project->path); }

    const char* identity = project->assembly;

    if (!identity || !*identity || !strcmp(identity, ".") || !strcmp(identity, "..")) { return false; }

    for (const unsigned char* p = (const unsigned char*)identity; *p; ++p) {
        if (!isalnum(*p) && *p != '.' && *p != '_' && *p != '-') {
            gcs_error(c, project->path, 0, "assembly names must contain only letters, digits, '.', '_' or '-'");

            return false;
        }
    }

    char* target = gcs_build_path(c, "obj", c->target);
    char* configuration = target ? gcs_build_path(c, target, c->configuration) : NULL;
    char* relative = configuration ? gcs_build_path(c, configuration, identity) : NULL;
    project->object_directory = relative ? gcs_build_path(c, project->directory, relative) : NULL;
    target = gcs_build_path(c, "bin", c->target);
    relative = target ? gcs_build_path(c, target, c->configuration) : NULL;
    project->output_directory = relative ? gcs_build_path(c, project->directory, relative) : NULL;

    if (!project->object_directory || !project->output_directory) { return false; }

    char* filename = project->executable ? gcs_copy(c, identity, strlen(identity)) : gcs_join(c, "lib", identity);
    if (filename && !project->executable) { filename = gcs_join(c, filename, ".a"); }
    if (filename && project->executable && (strstr(c->target, "mingw") || strstr(c->target, "windows"))) { filename = gcs_join(c, filename, ".exe"); }
    project->output = filename ? gcs_build_path(c, project->output_directory, filename) : NULL;
    filename = gcs_join(c, identity, ".semantic.h");
    project->contract = filename ? gcs_build_path(c, project->output_directory, filename) : NULL;
    filename = gcs_join(c, identity, ".h");
    project->public_header = filename ? gcs_build_path(c, project->output_directory, filename) : NULL;

    return project->output && project->contract && project->public_header;
}
