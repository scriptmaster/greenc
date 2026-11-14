/* Versioned source-independent C# declaration contracts retain semantic access information. */
#include "internal.h"

static const char* gcs_contract_type(gcs_type type) {
    const char* names[] = {"void", "int", "bool", "string", "string[]"};

    return type < GCS_INFER ? names[type] : "void";
}

bool gcs_emit_contract(gcs_compilation* c, gcs_project* project, gcs_buffer* output) {
    gcs_append(c, output, "greencsharp-contract-v2\nassembly %s\ntarget %s\nconfiguration %s\ntoolchain %s\n---\n",
               project->assembly, c->target, c->configuration, c->toolchain);

    for (gcs_class* owner = c->classes; owner; owner = owner->next) {
        if (owner->source->project != project) { continue; }

        const char* name = strrchr(owner->name, '.');

        if (*owner->space) { gcs_append(c, output, "namespace %s {\n", owner->space); }

        gcs_append(c, output, "%s class %s {\n", owner->exported ? "public" : "internal", name ? name + 1 : owner->name);

        for (gcs_method* method = c->methods; method; method = method->next) {
            if (method->owner != owner || !method->access) { continue; }

            gcs_append(c, output, "%s static %s %s(", method->access == 2 ? "public" : "internal", gcs_contract_type(method->type), method->name);

            for (gcs_variable* parameter = method->parameters; parameter; parameter = parameter->next) {
                gcs_append(c, output, "%s%s %s", parameter == method->parameters ? "" : ", ", gcs_contract_type(parameter->type), parameter->name);
            }

            gcs_append(c, output, ");\n");
        }

        gcs_append(c, output, "}\n");

        if (*owner->space) { gcs_append(c, output, "}\n"); }
    }

    return !c->diagnostic[0];
}

bool gcs_import_project(gcs_compilation* c, gcs_project* project) {
    if (!gcs_project_layout(c, project)) { return false; }

    char* text = gcs_read_data(c, project->contract);

    if (!text) { return false; }

    gcs_buffer expected = {0};
    gcs_append(c, &expected, "greencsharp-contract-v2\nassembly %s\ntarget %s\nconfiguration %s\ntoolchain %s\n---\n",
               project->assembly, c->target, c->configuration, c->toolchain);
    bool compatible = expected.data && !strncmp(text, expected.data, expected.length);
    size_t prefix = expected.length;
    free(expected.data);

    if (!compatible) {
        gcs_error(c, project->contract, 0, "incompatible native project contract; rebuild for this target/configuration/toolchain");

        return false;
    }

    if (!gcs_add_source(c, project, project->contract)) { return false; }

    for (gcs_source* source = c->sources; source; source = source->next) {
        if (!gcs_path_compare(source->path, project->contract)) {
            source->imported = true;
            source->text = text + prefix;

            return true;
        }
    }

    return false;
}

bool gcs_prepare_symbols(gcs_compilation* c) {
    for (gcs_method* method = c->methods; method; method = method->next) {
        const char* owner = method->owner ? method->owner->name : "<global>";
        char* prefix = gcs_join(c, method->source->project->assembly, "|");
        char* full = prefix ? gcs_join(c, prefix, owner) : NULL;
        prefix = full ? gcs_join(c, full, "|") : NULL;
        full = prefix ? gcs_join(c, prefix, method->name) : NULL;
        method->symbol = full ? gcs_symbol(c, "method", full) : NULL;

        if (!method->symbol) { return false; }

        for (gcs_method* other = c->methods; other != method; other = other->next) {
            if (other->symbol && !strcmp(method->symbol, other->symbol)) {
                gcs_error(c, method->source->path, method->line, "native method symbol collision");

                return false;
            }
        }
    }

    return true;
}
