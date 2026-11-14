/* Project graph loading, cycle detection and deterministic source ownership. */
#include "internal.h"

bool gcs_add_source(gcs_compilation* c, gcs_project* project, const char* path) {
    char* absolute = gcs_absolute(c, path);

    if (!absolute) { return false; }

    gcs_source** insertion = &c->sources;

    while (*insertion && gcs_path_compare((*insertion)->path, absolute) < 0) { insertion = &(*insertion)->next; }

    if (*insertion && !gcs_path_compare((*insertion)->path, absolute)) {
        if ((*insertion)->project != project) { gcs_error(c, path, 0, "source shared between projects is unsupported"); }

        return !c->diagnostic[0];
    }

    if (++c->source_count > 1024) {
        gcs_error(c, path, 0, "source limit exceeded (1024)");

        return false;
    }

    gcs_source* source = gcs_allocate(c, sizeof(*source));

    if (!source) { return false; }

    source->path = absolute;
    source->project = project;
    source->next = *insertion;
    *insertion = source;

    return true;
}

gcs_project* gcs_load_project(gcs_compilation* c, const char* path, size_t depth) {
    if (depth > 128) {
        gcs_error(c, path, 0, "project dependency depth exceeds 128");

        return NULL;
    }

    char* absolute = gcs_absolute(c, path);

    if (!absolute) { return NULL; }

    for (gcs_project* existing = c->projects; existing; existing = existing->next) {
        if (!gcs_path_compare(existing->path, absolute)) {
            if (existing->visiting) { gcs_error(c, path, 0, "cyclic ProjectReference dependency"); }

            return c->diagnostic[0] ? NULL : existing;
        }
    }

    if (++c->project_count > 128) {
        gcs_error(c, path, 0, "project limit exceeded (128)");

        return NULL;
    }

    gcs_project* project = gcs_allocate(c, sizeof(*project));

    if (!project) { return NULL; }

    project->path = absolute;
    project->assembly = gcs_assembly_name(c, absolute);
    project->id = c->project_count - 1;
    project->directory = gcs_directory(c, absolute);
    project->default_sources = true;
    project->visiting = true;
    project->next = c->projects;
    c->projects = project;
    bool success = project->directory && gcs_read_project(c, project, depth);
    project->visiting = false;
    project->loaded = success;

    return success ? project : NULL;
}

bool gcs_load_input(gcs_compilation* c, const char* path) {
    if (gcs_path_suffix(path, ".csproj")) { return gcs_load_project(c, path, 0) != NULL; }
    if (gcs_path_suffix(path, ".sln")) { return gcs_read_solution(c, path); }

    if (!gcs_path_suffix(path, ".cs")) {
        gcs_error(c, path, 0, "expected .cs, .csproj or .sln input");

        return false;
    }

    gcs_project* project = gcs_allocate(c, sizeof(*project));

    if (!project) { return false; }

    project->path = gcs_absolute(c, path);
    project->assembly = project->path ? gcs_assembly_name(c, project->path) : NULL;
    project->directory = project->path ? gcs_directory(c, project->path) : NULL;
    project->executable = true;
    project->loaded = true;
    c->projects = project;

    return project->directory && gcs_add_source(c, project, project->path);
}
