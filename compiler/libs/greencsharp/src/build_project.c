/* Each project binds against published dependency contracts, then publishes its own artifact. */
#include "internal.h"

static bool gcs_publish_project(gcs_compilation* c, gcs_project* project) {
    gcs_buffer contract = {0};
    bool success = gcs_emit_contract(c, project, &contract);
    char* include = gcs_build_path(c, project->object_directory, "include");
    char* name = gcs_join(c, project->assembly, ".h");
    char* header = include && name ? gcs_build_path(c, include, name) : NULL;
    char* runtime = include ? gcs_build_path(c, include, "greenc_runtime.h") : NULL;
    char* output_runtime = gcs_build_path(c, project->output_directory, "greenc_runtime.h");
    char* text = header ? gcs_read(c, header) : NULL;
    char* runtime_text = runtime ? gcs_read(c, runtime) : NULL;
    success = success && text && runtime_text && output_runtime &&
              gcs_write_text(c, project->public_header, text) && gcs_write_text(c, output_runtime, runtime_text) &&
              gcs_write_data(c, project->contract, contract.data);
    free(contract.data);

    if (success) {
        gcs_record_artifact(c, project->public_header);
        gcs_record_artifact(c, project->contract);
    }

    return success;
}

static bool gcs_compile_project(gcs_compilation* plan, gcs_project* planned, u32 mode) {
    gcs_compilation* c = calloc(1, sizeof(*c));

    if (!c) { return false; }

    c->root_path = planned->path;
    c->importing = true;
    c->target = plan->target;
    c->configuration = plan->configuration;
    c->toolchain = plan->toolchain;
    c->compiler = plan->compiler;
    c->archiver = plan->archiver;
    c->build_mode = mode;
    bool success = gcs_load_input(c, planned->path);
    gcs_project* project = NULL;

    for (gcs_project* candidate = c->projects; candidate; candidate = candidate->next) {
        if (!gcs_path_compare(candidate->path, planned->path)) { project = candidate; }
    }

    for (gcs_source* source = c->sources; source && success; source = source->next) { success = gcs_parse_source(c, source); }

    success = success && project && gcs_project_layout(c, project) && gcs_check(c) && gcs_prepare_symbols(c);
    success = success && gcs_emit_project(c, project);

    if (success && mode != GCS_EMIT_C) {
        success = gcs_link_project(c, project, mode == GCS_REBUILD) && gcs_publish_project(c, project);
        c->result.output_path = project->output;
    }

    if (success && !c->result.output_path) {
        c->result.output_path = gcs_build_path(c, project->object_directory, "generated");
    }

    if (success) {
        const char* manifest = gcs_build_path(c, project->object_directory, "artifacts.h");
        success = manifest && gcs_merge_artifacts(c, manifest) && gcs_write_data(c, manifest, c->manifest.data ? c->manifest.data : "");
        plan->result.output_path = gcs_copy(plan, c->result.output_path, strlen(c->result.output_path));
        plan->result.artifact_kind = project->executable ? 1 : 2;
        printf("%s\n", c->result.output_path);
        fflush(stdout);
    }

    if (!success) { gcs_error(plan, planned->path, 0, "%s", c->diagnostic[0] ? c->diagnostic : "native project compilation failed"); }

    gcs_release(c);

    return success;
}

bool gcs_build_project(gcs_compilation* c, gcs_project* project, u32 mode) {
    if (project->build_state == 2) { return true; }
    if (project->build_state == 1) {
        gcs_error(c, project->path, 0, "cyclic native project build dependency");

        return false;
    }

    project->build_state = 1;

    for (gcs_reference* reference = project->references; reference; reference = reference->next) {
        if (reference->project->executable) {
            gcs_error(c, reference->project->path, 0, "ProjectReference requires a Library project");

            return false;
        }

        if (!gcs_build_project(c, reference->project, mode == GCS_REBUILD || mode == GCS_CLEAN ? mode : GCS_BUILD)) { return false; }
    }

    bool success = gcs_project_layout(c, project) &&
                   (mode == GCS_CLEAN ? gcs_clean_project(c, project) : gcs_compile_project(c, project, mode));
    project->build_state = success ? 2 : 0;

    return success;
}
