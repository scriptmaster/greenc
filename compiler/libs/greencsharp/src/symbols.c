/* Lexical local scopes and assembly-aware static method lookup. */
#include "internal.h"

gcs_variable* gcs_find_variable(gcs_scope* scope, const char* name) {
    for (; scope; scope = scope->parent) {
        for (gcs_variable* variable = scope->variables; variable; variable = variable->next) {
            if (!strcmp(variable->name, name)) { return variable; }
        }
    }

    return NULL;
}

bool gcs_declare_variable(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_variable* variable, size_t line) {
    if (variable->type == GCS_VOID || variable->type >= GCS_INFER || gcs_find_variable(scope, variable->name)) {
        gcs_error(c, method->source->path, line, "invalid or duplicate local '%s'", variable->name);

        return false;
    }

    variable->next = scope->variables;
    scope->variables = variable;

    return true;
}

static bool gcs_project_reaches(const gcs_project* caller, const gcs_project* target, bool* visited) {
    if (caller == target) { return true; }
    if (visited[caller->id]) { return false; }

    visited[caller->id] = true;

    for (gcs_reference* reference = caller->references; reference; reference = reference->next) {
        if (gcs_project_reaches(reference->project, target, visited)) { return true; }
    }

    return false;
}

static bool gcs_method_matches(gcs_compilation* c, const gcs_method* caller, const gcs_method* target, const char* name) {
    if (!target->owner) { return false; }

    const char* dot = strrchr(name, '.');

    if (!dot) { return caller->owner == target->owner && !strcmp(name, target->name); }
    if (strcmp(dot + 1, target->name)) { return false; }

    const char* type = gcs_copy(c, name, (size_t)(dot - name));

    if (!type) { return false; }
    if (!strcmp(type, target->owner->name)) { return true; }

    const char* space = caller->owner ? caller->owner->space : "";
    const char* prefix = *space ? gcs_join(c, space, ".") : "";
    const char* qualified = prefix ? gcs_join(c, prefix, type) : NULL;

    if (qualified && !strcmp(qualified, target->owner->name)) { return true; }

    for (gcs_using* import = caller->source->imports; import; import = import->next) {
        prefix = gcs_join(c, import->name, ".");
        qualified = prefix ? gcs_join(c, prefix, type) : NULL;

        if (qualified && !strcmp(qualified, target->owner->name)) { return true; }
    }

    return false;
}

gcs_method* gcs_resolve_method(gcs_compilation* c, gcs_method* caller, const char* name, size_t line) {
    gcs_method* match = NULL;

    for (gcs_method* target = c->methods; target; target = target->next) {
        if (!gcs_method_matches(c, caller, target, name)) { continue; }

        bool same_project = caller->source->project == target->source->project;
        bool visited[128] = {0};
        bool accessible = caller->owner == target->owner ||
                          (same_project && target->access >= 1) ||
                          (!same_project && target->access == 2 && target->owner->exported &&
                           gcs_project_reaches(caller->source->project, target->source->project, visited));

        if (!accessible) { continue; }

        if (match) {
            gcs_error(c, caller->source->path, line, "ambiguous static method '%s'", name);

            return NULL;
        }

        match = target;
    }

    if (!match) { gcs_error(c, caller->source->path, line, "unknown or inaccessible static method '%s'", name); }

    return match;
}
