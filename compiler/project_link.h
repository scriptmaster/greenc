/*
 ** project_link.h - 2 functions.
 ** greenc_project_output - Derive the executable path from the current project directory.
 ** greenc_project_link - Write native sources and library options after application objects, then link.
 ** See also: project_build.h, project_sources.h, project_libraries.h.
 */
#pragma once

static char* greenc_project_output(void) {
    char* root = greenc_project_path(".");
    const char* base = root;

    for (const char* c = root; c && *c; ++c) {
        if (*c == '/') { base = c + 1; }
    }

    char* output = root && *base ? greenc_concat("bin/", base, ".exe") : NULL;
    greenc_free(root);

    return output;
}

static bool greenc_project_link(greenc_project* project, const char* output) {
    return greenc_project_append(&project->arguments, project->libraries->native_arguments ? project->libraries->native_arguments : "") &&
           greenc_project_append(&project->arguments, project->libraries->link_arguments ? project->libraries->link_arguments : "") &&
           greenc_project_append(&project->arguments, project->link_arguments ? project->link_arguments : "") &&
           greenc_project_append(&project->arguments, "-o\n") &&
           greenc_project_quote(&project->arguments, output) &&
           greenc_write_file("build/greenc/link.rsp", project->arguments) == 0 &&
           greenc_project_run("gcc @build/greenc/link.rsp") == 0;
}
