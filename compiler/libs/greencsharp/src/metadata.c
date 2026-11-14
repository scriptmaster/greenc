/* Reviewable dependency graph and typed AST artifacts accompany generated native C. */
#include "internal.h"

static void gcs_ast_node(gcs_compilation* c, const gcs_node* node) {
    if (!node) {
        gcs_append(c, &c->ast, "null");

        return;
    }

    const char* kinds[] = {"integer", "boolean", "string", "name", "call", "unary", "binary", "assignment",
        "block", "declaration", "expression", "return", "if", "while", "for", "break", "continue", "length", "index", "update"};
    const char* types[] = {"void", "int", "bool", "string", "string[]", "inferred", "statement"};
    gcs_append(c, &c->ast, "{\"kind\":\"%s\",\"type\":\"%s\",\"line\":%zu,\"name\":", kinds[node->kind], types[node->type], node->token.line);
    gcs_json(c, &c->ast, node->name ? node->name : "");
    if (node->method) { gcs_append(c, &c->ast, ",\"method\":%zu", node->method->id); }
    if (node->variable) { gcs_append(c, &c->ast, ",\"variable\":%zu", node->variable->id); }
    gcs_append(c, &c->ast, ",\"children\":[");
    const gcs_node* children[] = {node->first, node->second, node->third, node->fourth};

    for (size_t i = 0; i < 4; ++i) {
        if (i) { gcs_append(c, &c->ast, ","); }

        gcs_append(c, &c->ast, "[");

        for (const gcs_node* child = children[i]; child; child = child->next) {
            if (child != children[i]) { gcs_append(c, &c->ast, ","); }

            gcs_ast_node(c, child);
        }

        gcs_append(c, &c->ast, "]");
    }

    gcs_append(c, &c->ast, "]}");
}

static void gcs_graph(gcs_compilation* c) {
    gcs_append(c, &c->dependencies, "{\"projects\":[");

    for (gcs_project* project = c->projects; project; project = project->next) {
        if (project != c->projects) { gcs_append(c, &c->dependencies, ","); }

        gcs_append(c, &c->dependencies, "{\"executable\":%s,\"path\":", project->executable ? "true" : "false");
        gcs_json(c, &c->dependencies, project->path);
        gcs_append(c, &c->dependencies, ",\"references\":[");

        for (gcs_reference* reference = project->references; reference; reference = reference->next) {
            if (reference != project->references) { gcs_append(c, &c->dependencies, ","); }

            gcs_json(c, &c->dependencies, reference->project->path);
        }

        gcs_append(c, &c->dependencies, "]}");
    }

    gcs_append(c, &c->dependencies, "],\"sources\":[");

    for (gcs_source* source = c->sources; source; source = source->next) {
        if (source != c->sources) { gcs_append(c, &c->dependencies, ","); }

        gcs_append(c, &c->dependencies, "{\"path\":");
        gcs_json(c, &c->dependencies, source->path);
        gcs_append(c, &c->dependencies, ",\"project\":");
        gcs_json(c, &c->dependencies, source->project->path);
        gcs_append(c, &c->dependencies, "}");
    }

    gcs_append(c, &c->dependencies, "]}\n");
}

bool gcs_emit_metadata(gcs_compilation* c) {
    gcs_append(c, &c->ast, "{\"methods\":[");

    for (gcs_method* method = c->methods; method; method = method->next) {
        if (method != c->methods) { gcs_append(c, &c->ast, ","); }

        gcs_append(c, &c->ast, "{\"id\":%zu,\"class\":", method->id);
        gcs_json(c, &c->ast, method->owner ? method->owner->name : "");
        gcs_append(c, &c->ast, ",\"name\":");
        gcs_json(c, &c->ast, method->name);
        gcs_append(c, &c->ast, ",\"symbol\":");
        gcs_json(c, &c->ast, method->symbol ? method->symbol : "");
        gcs_append(c, &c->ast, ",\"source\":");
        gcs_json(c, &c->ast, method->source->path);
        gcs_append(c, &c->ast, ",\"returnType\":%u,\"parameters\":[", (u32)method->type);

        for (gcs_variable* parameter = method->parameters; parameter; parameter = parameter->next) {
            if (parameter != method->parameters) { gcs_append(c, &c->ast, ","); }

            gcs_append(c, &c->ast, "{\"id\":%zu,\"name\":", parameter->id);
            gcs_json(c, &c->ast, parameter->name);
            gcs_append(c, &c->ast, ",\"type\":%u}", (u32)parameter->type);
        }

        gcs_append(c, &c->ast, "],\"body\":");
        gcs_ast_node(c, method->body);
        gcs_append(c, &c->ast, "}");
    }

    gcs_append(c, &c->ast, "]}\n");
    gcs_graph(c);

    return !c->diagnostic[0];
}
