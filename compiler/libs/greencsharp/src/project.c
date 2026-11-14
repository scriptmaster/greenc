/* Declarative project properties, explicit compile items and project references. */
#include "internal.h"

static bool gcs_project_attributes(gcs_compilation* c, gcs_project* project, const gcs_xml_node* node, const char* allowed) {
    for (gcs_xml_attribute* attribute = node->attributes; attribute; attribute = attribute->next) {
        if (!allowed || strcmp(attribute->name, allowed)) {
            gcs_error(c, project->path, 0, "unsupported attribute '%s' on %s (MSBuild evaluation is not implemented)", attribute->name, node->name);

            return false;
        }
    }

    return true;
}

static bool gcs_project_properties(gcs_compilation* c, gcs_project* project, gcs_xml_node* group) {
    for (gcs_xml_node* property = group->children; property; property = property->next) {
        if (!gcs_project_attributes(c, project, property, NULL) || property->children) { return false; }

        const char* value = property->text;

        if (!strcmp(property->name, "OutputType")) {
            if (strcmp(value, "Exe") && strcmp(value, "Library")) {
                gcs_error(c, project->path, 0, "only Exe and Library OutputType are supported");

                return false;
            }

            project->executable = !strcmp(value, "Exe");
        } else if (!strcmp(property->name, "AssemblyName")) {
            project->assembly = value;
        } else if (!strcmp(property->name, "EnableDefaultCompileItems")) {
            if (strcmp(value, "true") && strcmp(value, "false")) { return false; }

            project->default_sources = !strcmp(value, "true");
        } else if (!strcmp(property->name, "TargetFramework")) {
            if (strncmp(value, "net", 3) || strpbrk(value, "$;")) { return false; }
        } else if (!strcmp(property->name, "ImplicitUsings")) {
            if (strcmp(value, "enable") && strcmp(value, "disable")) { return false; }

            project->implicit_usings = !strcmp(value, "enable");
        } else if (!strcmp(property->name, "IsTestProject")) {
            if (strcmp(value, "true") && strcmp(value, "false")) { return false; }
        } else if (!strcmp(property->name, "Nullable")) {
            if (strcmp(value, "disable") && strcmp(value, "enable")) {
                gcs_error(c, project->path, 0, "unsupported Nullable project setting");

                return false;
            }
        } else {
            gcs_error(c, project->path, 0, "unsupported project property '%s'", property->name);

            return false;
        }
    }

    return true;
}

static bool gcs_project_items(gcs_compilation* c, gcs_project* project, gcs_xml_node* group, size_t depth) {
    for (gcs_xml_node* item = group->children; item; item = item->next) {
        const char* include = gcs_xml_value(item, "Include");

        if (!gcs_project_attributes(c, project, item, "Include") || !include || !*include ||
            strpbrk(include, "$%*?;") || item->children || item->text[0]) {
            gcs_error(c, project->path, 0, "only literal Include paths without item metadata are supported");

            return false;
        }

        if (strcmp(item->name, "Compile") && strcmp(item->name, "ProjectReference")) {
            gcs_error(c, project->path, 0, "unsupported project item '%s'; NuGet and assembly references are unavailable", item->name);

            return false;
        }

        char* path = gcs_resolve_path(c, project->directory, include);

        if (!path) { return false; }

        if (!strcmp(item->name, "Compile") && gcs_path_suffix(path, ".cs")) {
            if (c->importing && gcs_path_compare(project->path, c->root_path)) { continue; }

            if (!gcs_add_source(c, project, path)) { return false; }
        } else if (!strcmp(item->name, "ProjectReference") && gcs_path_suffix(path, ".csproj")) {
            gcs_project* dependency = gcs_load_project(c, path, depth + 1);
            bool present = false;

            for (gcs_reference* existing = project->references; existing; existing = existing->next) {
                if (existing->project == dependency) { present = true; }
            }

            if (present) { continue; }

            gcs_reference* reference = dependency ? gcs_allocate(c, sizeof(*reference)) : NULL;

            if (!reference) { return false; }

            reference->project = dependency;
            reference->next = project->references;
            project->references = reference;
        } else {
            gcs_error(c, project->path, 0, "unsupported project item '%s' (NuGet and assembly references are unavailable)", item->name);

            return false;
        }
    }

    return true;
}

bool gcs_read_project(gcs_compilation* c, gcs_project* project, size_t depth) {
    gcs_xml_node* root = gcs_xml_document(c, project->path);

    if (!root || strcmp(root->name, "Project") || root->text[0] || !gcs_project_attributes(c, project, root, "Sdk")) { return false; }

    const char* sdk = gcs_xml_value(root, "Sdk");

    if (sdk && strcmp(sdk, "Microsoft.NET.Sdk")) {
        gcs_error(c, project->path, 0, "only the declarative Microsoft.NET.Sdk project shape is supported");

        return false;
    }

    project->default_sources = sdk != NULL;

    for (gcs_xml_node* group = root->children; group; group = group->next) {
        if (!gcs_project_attributes(c, project, group, NULL) || group->text[0]) { return false; }

        if (!strcmp(group->name, "PropertyGroup")) {
            if (!gcs_project_properties(c, project, group)) { return false; }
        } else if (strcmp(group->name, "ItemGroup")) {
            gcs_error(c, project->path, 0, "unsupported project element '%s' (imports and targets cannot execute)", group->name);

            return false;
        }
    }

    bool imported = c->importing && gcs_path_compare(project->path, c->root_path);

    if (!imported && project->default_sources && !gcs_scan_sources(c, project, project->directory, 0)) { return false; }

    for (gcs_xml_node* group = root->children; group; group = group->next) {
        if (!strcmp(group->name, "ItemGroup") && !gcs_project_items(c, project, group, depth)) { return false; }
    }

    if (imported && !gcs_import_project(c, project)) { return false; }

    return !c->diagnostic[0];
}
