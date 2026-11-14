/* Stable method symbols allow independently compiled project translation units. */
#include "internal.h"

const char* gcs_type_name(gcs_type type) {
    const char* names[] = {"void", "i32", "bool", "const char*", "gcs_arguments"};

    return type < GCS_INFER ? names[type] : "void";
}

bool gcs_emit_signature(gcs_compilation* c, gcs_method* method, gcs_buffer* output) {
    gcs_append(c, output, "/* C#: %s.%s */\n", method->owner ? method->owner->name : "<global>", method->name);
    gcs_append(c, output, "%s %s(", gcs_type_name(method->type), method->symbol);

    if (!method->parameters) { gcs_append(c, output, "void"); }

    for (gcs_variable* parameter = method->parameters; parameter; parameter = parameter->next) {
        gcs_append(c, output, "%s%s gcs_local_%zu /* %s */", parameter == method->parameters ? "" : ", ",
                   gcs_type_name(parameter->type), parameter->id, parameter->name);
    }

    return gcs_append(c, output, ")");
}

void gcs_emit_method(gcs_compilation* c, gcs_method* method) {
    c->emitting_method = method;
    c->temporary_count = 0;
    gcs_emit_signature(c, method, &c->program);
    gcs_append(c, &c->program, " {\n");

    for (gcs_variable* parameter = method->parameters; parameter; parameter = parameter->next) {
        gcs_append(c, &c->program, "(void)gcs_local_%zu;\n", parameter->id);
    }

    gcs_emit_statement(c, method->body);

    if (method->top_level) { gcs_append(c, &c->program, "return 0;\n"); }

    gcs_append(c, &c->program, "}\n\n");
}

void gcs_emit_entry(gcs_compilation* c) {
    if (!c->entry) { return; }

    bool arguments = c->entry->parameters != NULL;
    gcs_append(c, &c->program, arguments ? "int main(int argc, char** argv) {\ngcs_arguments args = {(i32)(argc - 1), argv + 1};\n" : "int main(void) {\n");

    if (c->entry->type == GCS_INT) {
        gcs_append(c, &c->program, "return (int)%s(%s);\n", c->entry->symbol, arguments ? "args" : "");
    } else {
        gcs_append(c, &c->program, "%s(%s);\nreturn 0;\n", c->entry->symbol, arguments ? "args" : "");
    }

    gcs_append(c, &c->program, "}\n");
}

bool gcs_emit(gcs_compilation* c) {
    gcs_emit_runtime(c);

    for (gcs_method* method = c->methods; method; method = method->next) {
        gcs_emit_signature(c, method, &c->program);
        gcs_append(c, &c->program, ";\n");
    }

    for (gcs_method* method = c->methods; method; method = method->next) {
        if (!method->source->imported) { gcs_emit_method(c, method); }
    }

    gcs_emit_entry(c);

    return !c->diagnostic[0];
}
