/* Expressions lower to sequential temporaries, preserving C# evaluation order. */
#include "internal.h"

static void gcs_emit_call(gcs_compilation* c, gcs_node* node, size_t id) {
    for (gcs_node* argument = node->first; argument; argument = argument->next) { gcs_emit_expression(c, argument); }

    if (node->builtin) {
        bool newline = gcs_suffix(node->name, "WriteLine");

        if (!node->first) {
            gcs_append(c, &c->program, "fputc('\\n', stdout);\n");

            return;
        }

        const char* suffix = node->first->type == GCS_INT ? "int" : node->first->type == GCS_BOOL ? "bool" : "string";
        gcs_append(c, &c->program, "gcs_write_%s(gcs_temp_%zu, %s);\n", suffix, node->first->emitted_id, newline ? "true" : "false");

        return;
    }

    if (node->type != GCS_VOID) { gcs_append(c, &c->program, "%s gcs_temp_%zu = ", gcs_type_name(node->type), id); }

    gcs_append(c, &c->program, "%s(", node->method->symbol);

    for (gcs_node* argument = node->first; argument; argument = argument->next) {
        gcs_append(c, &c->program, "%sgcs_temp_%zu", argument == node->first ? "" : ", ", argument->emitted_id);
    }

    gcs_append(c, &c->program, ");\n");
}

static void gcs_emit_binary(gcs_compilation* c, gcs_node* node, size_t id) {
    size_t left = gcs_emit_expression(c, node->first);

    if (!strcmp(node->name, "&&") || !strcmp(node->name, "||")) {
        gcs_append(c, &c->program, "bool gcs_temp_%zu = gcs_temp_%zu;\nif (%sgcs_temp_%zu) {\n", id, left,
                   !strcmp(node->name, "||") ? "!" : "", id);
        size_t right = gcs_emit_expression(c, node->second);
        gcs_append(c, &c->program, "gcs_temp_%zu = gcs_temp_%zu;\n}\n", id, right);

        return;
    }

    size_t right = gcs_emit_expression(c, node->second);
    gcs_append(c, &c->program, "%s gcs_temp_%zu = ", gcs_type_name(node->type), id);

    if (node->first->type == GCS_STRING) {
        gcs_append(c, &c->program, "strcmp(gcs_temp_%zu, gcs_temp_%zu) %s 0", left, right, node->name);
    } else if (strchr("+-*/%", node->name[0])) {
        const char* helper = node->name[0] == '+' ? "add" : node->name[0] == '-' ? "subtract" :
                             node->name[0] == '*' ? "multiply" : node->name[0] == '/' ? "divide" : "remainder";
        gcs_append(c, &c->program, "gcs_%s(gcs_temp_%zu, gcs_temp_%zu)", helper, left, right);
    } else {
        gcs_append(c, &c->program, "gcs_temp_%zu %s gcs_temp_%zu", left, node->name, right);
    }

    gcs_append(c, &c->program, ";\n");
}

static size_t gcs_emit_assignment(gcs_compilation* c, gcs_node* node, size_t id) {
    bool compound = strcmp(node->name, "=") != 0;
    size_t previous = compound ? gcs_emit_expression(c, node->first) : SIZE_MAX;
    size_t value = gcs_emit_expression(c, node->second);

    if (compound) {
        const char* helper = node->name[0] == '+' ? "add" : node->name[0] == '-' ? "subtract" :
                             node->name[0] == '*' ? "multiply" : node->name[0] == '/' ? "divide" : "remainder";
        gcs_append(c, &c->program, "i32 gcs_temp_%zu = gcs_%s(gcs_temp_%zu, gcs_temp_%zu);\n", id, helper, previous, value);
        value = id;
    }

    gcs_append(c, &c->program, "gcs_local_%zu = gcs_temp_%zu;\n", node->first->variable->id, value);
    node->emitted_id = value;

    return value;
}

static size_t gcs_emit_update(gcs_compilation* c, gcs_node* node, size_t id) {
    size_t previous = gcs_emit_expression(c, node->first);
    gcs_append(c, &c->program, "i32 gcs_temp_%zu = gcs_%s(gcs_temp_%zu, 1);\n", id,
               node->name[0] == '+' ? "add" : "subtract", previous);
    gcs_append(c, &c->program, "gcs_local_%zu = gcs_temp_%zu;\n", node->first->variable->id, id);
    node->emitted_id = node->postfix ? previous : id;

    return node->emitted_id;
}

size_t gcs_emit_expression(gcs_compilation* c, gcs_node* node) {
    if (!node || c->diagnostic[0]) { return SIZE_MAX; }

    size_t id = c->temporary_count++;
    node->emitted_id = id;

    switch (node->kind) {
        case GCS_INTEGER: case GCS_BOOLEAN: case GCS_TEXT: gcs_emit_literal(c, node, id); break;
        case GCS_NAME:
            gcs_append(c, &c->program, "%s gcs_temp_%zu = gcs_local_%zu;\n", gcs_type_name(node->type), id, node->variable->id);

            break;
        case GCS_UNARY: {
            size_t operand = gcs_emit_expression(c, node->first);

            if (!strcmp(node->name, "-")) {
                gcs_append(c, &c->program, "i32 gcs_temp_%zu = gcs_subtract(0, gcs_temp_%zu);\n", id, operand);
            } else {
                gcs_append(c, &c->program, "bool gcs_temp_%zu = !gcs_temp_%zu;\n", id, operand);
            }

            break;
        }
        case GCS_BINARY: gcs_emit_binary(c, node, id); break;
        case GCS_ASSIGN: return gcs_emit_assignment(c, node, id);
        case GCS_UPDATE: return gcs_emit_update(c, node, id);
        case GCS_LENGTH:
            gcs_append(c, &c->program, "i32 gcs_temp_%zu = gcs_local_%zu.count;\n", id, node->variable->id);

            break;
        case GCS_INDEX: {
            size_t array = gcs_emit_expression(c, node->first);
            size_t index = gcs_emit_expression(c, node->second);
            gcs_append(c, &c->program, "const char* gcs_temp_%zu = gcs_argument(gcs_temp_%zu, gcs_temp_%zu);\n", id, array, index);

            break;
        }
        case GCS_CALL: gcs_emit_call(c, node, id); break;
        default: break;
    }

    return node->type == GCS_VOID ? SIZE_MAX : id;
}
