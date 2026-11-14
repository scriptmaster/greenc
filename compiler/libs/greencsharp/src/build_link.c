/* Fresh archives and executable links receive direct argv and publish atomically. */
#include "internal.h"

static void gcs_link_order(gcs_project* project, bool* visited, gcs_project** ordered, size_t* count) {
    if (visited[project->id]) { return; }

    visited[project->id] = true;

    for (gcs_reference* reference = project->references; reference; reference = reference->next) {
        gcs_link_order(reference->project, visited, ordered, count);
    }

    ordered[(*count)++] = project;
}

bool gcs_link_project(gcs_compilation* c, gcs_project* project, bool force) {
    char* temporary = gcs_join(c, project->output, project->executable ? ".tmp.exe" : ".tmp.a");
    char* stamp = gcs_join(c, project->output, ".cache.h");
    /* At most 1,024 own sources, 127 dependency archives and tool options. */
    const char* arguments[1200];
    size_t count = 0;
    arguments[count++] = project->executable ? c->compiler : c->archiver;

    if (!project->executable) {
        arguments[count++] = "rcs";
        arguments[count++] = temporary;
    }

    u64 fingerprint = gcs_hash_bytes(UINT64_C(14695981039346656037), c->toolchain, strlen(c->toolchain));
    bool success = temporary && stamp;

    for (gcs_object* object = c->objects; object && success; object = object->next) {
        arguments[count++] = object->path;
        success = gcs_hash_file(c, object->path, &fingerprint);
    }

    if (project->executable) {
        bool visited[128] = {0};
        gcs_project* ordered[128];
        size_t project_count = 0;
        gcs_link_order(project, visited, ordered, &project_count);

        while (project_count && success) {
            gcs_project* dependency = ordered[--project_count];

            if (dependency == project) { continue; }

            success = gcs_project_layout(c, dependency) && gcs_hash_file(c, dependency->output, &fingerprint);
            arguments[count++] = dependency->output;
        }

        arguments[count++] = "-o";
        arguments[count++] = temporary;
    }

    arguments[count] = NULL;

    for (size_t i = 0; i < count; ++i) { fingerprint = gcs_hash_bytes(fingerprint, arguments[i], strlen(arguments[i]) + 1); }

    bool cached = !force && success && gcs_file_exists(project->output) && gcs_stamp_matches(c, stamp, fingerprint);

    if (success && (force || !cached)) {
        success = gcs_make_directories(c, project->output) && gcs_remove_file(c, temporary) &&
                  gcs_tool(c, arguments, NULL) && gcs_replace_file(c, temporary, project->output) && gcs_stamp_write(c, stamp, fingerprint);
    }

    if (success) {
        gcs_record_artifact(c, project->output);
        gcs_record_artifact(c, stamp);
    }

    return success && !c->diagnostic[0];
}
