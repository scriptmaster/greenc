/* Structured native control flow consumes the checked statement AST. */
#include "internal.h"

static void gcs_emit_for(gcs_compilation* c, gcs_node* node) {
    size_t first = c->temporary_count++;
    gcs_append(c, &c->program, "{\n");

    if (node->first) { gcs_emit_statement(c, node->first); }

    gcs_append(c, &c->program, "bool gcs_first_%zu = true;\nfor (;;) {\nif (!gcs_first_%zu) {\n", first, first);

    if (node->third) {
        size_t step = gcs_emit_expression(c, node->third);

        if (step != SIZE_MAX) { gcs_append(c, &c->program, "(void)gcs_temp_%zu;\n", step); }
    }

    gcs_append(c, &c->program, "}\ngcs_first_%zu = false;\n", first);

    if (node->second) {
        size_t condition = gcs_emit_expression(c, node->second);
        gcs_append(c, &c->program, "if (!gcs_temp_%zu) { break; }\n", condition);
    }

    gcs_emit_statement(c, node->fourth);
    gcs_append(c, &c->program, "}\n}\n");
}

void gcs_emit_statement(gcs_compilation* c, gcs_node* node) {
    if (!node || c->diagnostic[0]) { return; }

    switch (node->kind) {
        case GCS_BLOCK:
            gcs_append(c, &c->program, "{\n");

            for (gcs_node* statement = node->first; statement; statement = statement->next) { gcs_emit_statement(c, statement); }

            gcs_append(c, &c->program, "}\n");

            break;
        case GCS_DECLARE: {
            size_t value = gcs_emit_expression(c, node->first);
            gcs_append(c, &c->program, "%s gcs_local_%zu = gcs_temp_%zu;\n(void)gcs_local_%zu;\n",
                       gcs_type_name(node->type), node->variable->id, value, node->variable->id);

            break;
        }
        case GCS_EXPRESSION: {
            size_t value = gcs_emit_expression(c, node->first);

            if (value != SIZE_MAX) { gcs_append(c, &c->program, "(void)gcs_temp_%zu;\n", value); }

            break;
        }
        case GCS_RETURN:
            if (node->first) {
                size_t value = gcs_emit_expression(c, node->first);
                gcs_append(c, &c->program, "return gcs_temp_%zu;\n", value);
            } else {
                gcs_append(c, &c->program, c->emitting_method->top_level ? "return 0;\n" : "return;\n");
            }

            break;
        case GCS_IF: {
            size_t condition = gcs_emit_expression(c, node->first);
            gcs_append(c, &c->program, "if (gcs_temp_%zu) {\n", condition);
            gcs_emit_statement(c, node->second);
            gcs_append(c, &c->program, "}\n");

            if (node->third) {
                gcs_append(c, &c->program, "else {\n");
                gcs_emit_statement(c, node->third);
                gcs_append(c, &c->program, "}\n");
            }

            break;
        }
        case GCS_WHILE: {
            gcs_append(c, &c->program, "while (true) {\n");
            size_t condition = gcs_emit_expression(c, node->first);
            gcs_append(c, &c->program, "if (!gcs_temp_%zu) { break; }\n", condition);
            gcs_emit_statement(c, node->second);
            gcs_append(c, &c->program, "}\n");

            break;
        }
        case GCS_FOR: gcs_emit_for(c, node); break;
        case GCS_BREAK: gcs_append(c, &c->program, "break;\n"); break;
        case GCS_CONTINUE: gcs_append(c, &c->program, "continue;\n"); break;
        default: break;
    }
}
