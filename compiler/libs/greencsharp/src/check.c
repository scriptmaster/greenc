/* Whole-program declarations and entry-point invariants precede lowering. */
#include "internal.h"

static bool gcs_returns(const gcs_node* node) {
    if (!node) { return false; }
    if (node->kind == GCS_RETURN) { return true; }
    if (node->kind == GCS_IF) { return gcs_returns(node->second) && gcs_returns(node->third); }

    if (node->kind == GCS_BLOCK) {
        for (const gcs_node* statement = node->first; statement; statement = statement->next) {
            if (gcs_returns(statement)) { return true; }
        }
    }

    return false;
}

static bool gcs_imports(gcs_compilation* c) {
    for (gcs_source* source = c->sources; source; source = source->next) {
        for (gcs_using* import = source->imports; import; import = import->next) {
            bool found = !strcmp(import->name, "System");
            size_t length = strlen(import->name);

            for (gcs_class* owner = c->classes; owner && !found; owner = owner->next) {
                found = !strcmp(owner->space, import->name) ||
                        (strlen(owner->space) > length && !strncmp(owner->space, import->name, length) && owner->space[length] == '.');
            }

            if (!found) {
                gcs_error(c, source->path, 0, "unknown or unsupported namespace '%s'", import->name);

                return false;
            }
        }
    }

    return true;
}

static bool gcs_declarations(gcs_compilation* c) {
    for (gcs_class* owner = c->classes; owner; owner = owner->next) {
        if (!strcmp(owner->name, "System.Console")) {
            gcs_error(c, owner->source->path, 0, "System.Console is provided by the native runtime");

            return false;
        }

        for (gcs_class* other = owner->next; other; other = other->next) {
            if (!strcmp(owner->name, other->name)) {
                gcs_error(c, owner->source->path, 0, "duplicate class '%s'; partial classes are unsupported", owner->name);

                return false;
            }
        }
    }

    for (gcs_method* method = c->methods; method; method = method->next) {
        for (gcs_method* other = method->next; other; other = other->next) {
            if (method->owner && method->owner == other->owner && !strcmp(method->name, other->name)) {
                gcs_error(c, method->source->path, method->line, "duplicate method '%s'; overloads are unsupported", method->name);

                return false;
            }
        }
    }

    return true;
}

static bool gcs_entry_point(gcs_compilation* c) {
    gcs_project* executable = NULL;

    for (gcs_project* project = c->projects; project; project = project->next) {
        if (c->importing && gcs_path_compare(project->path, c->root_path)) { continue; }

        if (!project->executable) { continue; }

        if (executable) {
            gcs_error(c, project->path, 0, "one executable project per compilation is currently supported");

            return false;
        }

        executable = project;
    }

    c->result.artifact_kind = executable ? 1 : 2;

    for (gcs_method* method = c->methods; method; method = method->next) {
        if (method->source->project != executable || (!method->top_level && strcmp(method->name, "Main"))) { continue; }

        bool arguments = !method->parameters || (method->parameters->type == GCS_ARGS && !method->parameters->next);

        if (c->entry || !arguments || (method->type != GCS_INT && method->type != GCS_VOID)) {
            gcs_error(c, method->source->path, method->line, "entry point must be static void/int Main([string[] args]) or one top-level file");

            return false;
        }

        c->entry = method;
    }

    if (executable && !c->entry) {
        gcs_error(c, executable->path, 0, "executable has no supported Main entry point");

        return false;
    }

    return true;
}

bool gcs_check(gcs_compilation* c) {
    if (!gcs_imports(c) || !gcs_declarations(c) || !gcs_entry_point(c)) { return false; }

    for (gcs_method* method = c->methods; method; method = method->next) {
        if (method->source->imported) { continue; }

        gcs_scope parameters = {0};

        for (gcs_variable* parameter = method->parameters; parameter; parameter = parameter->next) {
            if (parameter->id >= method->next_variable) { method->next_variable = parameter->id + 1; }

            if (parameter->type == GCS_VOID || parameter->type >= GCS_INFER) {
                gcs_error(c, method->source->path, method->line, "invalid parameter type");

                return false;
            }

            for (gcs_variable* other = parameter->next; other; other = other->next) {
                if (!strcmp(parameter->name, other->name)) {
                    gcs_error(c, method->source->path, method->line, "duplicate parameter '%s'", parameter->name);

                    return false;
                }
            }
        }

        parameters.variables = method->parameters;

        if (method->top_level && !method->source->project->executable) {
            gcs_error(c, method->source->path, method->line, "top-level statements require an Exe project");

            return false;
        }

        if (!gcs_check_statement(c, method, &parameters, method->body, 0)) { return false; }

        if (!method->top_level && method->type != GCS_VOID && !gcs_returns(method->body)) {
            gcs_error(c, method->source->path, method->line, "non-void method must return on every supported path");

            return false;
        }
    }

    return !c->diagnostic[0];
}
