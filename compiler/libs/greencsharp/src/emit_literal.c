/* Literal lowering preserves decimal values and prevents native C trigraph conversion. */
#include "internal.h"

void gcs_emit_literal(gcs_compilation* c, const gcs_node* node, size_t id) {
    if (node->kind == GCS_INTEGER) {
        gcs_append(c, &c->program, "i32 gcs_temp_%zu = %" PRId32 ";\n", id, node->integer);

        return;
    }

    if (node->kind == GCS_BOOLEAN) {
        gcs_append(c, &c->program, "bool gcs_temp_%zu = %s;\n", id, node->name);

        return;
    }

    gcs_append(c, &c->program, "const char* gcs_temp_%zu = \"", id);
    const char* cursor = node->name + 1;
    const char* end = node->name + strlen(node->name) - 1;

    while (cursor < end && !c->diagnostic[0]) {
        const char* question = memchr(cursor, '?', (size_t)(end - cursor));
        size_t length = (size_t)((question ? question : end) - cursor);

        if (length > INT_MAX) {
            gcs_error(c, NULL, 0, "string emission span exceeds native formatting limit");

            return;
        }

        gcs_append(c, &c->program, "%.*s", (int)length, cursor);

        if (!question) { break; }

        gcs_append(c, &c->program, "\\?");
        cursor = question + 1;
    }

    gcs_append(c, &c->program, "\";\n");
}
