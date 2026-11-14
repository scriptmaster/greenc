/* Structured statements and initialized locals form a bounded statement AST. */
#include "internal.h"

static gcs_node* gcs_simple_statement(gcs_parser* p, bool semicolon) {
    bool declaration = gcs_is_declaration(p);
    gcs_node* node = gcs_new_node(p, declaration ? GCS_DECLARE : GCS_EXPRESSION);

    if (!node) { return NULL; }

    if (declaration) {
        node->type = gcs_parse_type(p, true);
        node->name = gcs_identifier(p);
        gcs_expect(p, "=");
    }

    node->first = gcs_expression(p, 1);

    if (semicolon) { gcs_expect(p, ";"); }

    return node;
}

static gcs_node* gcs_control_statement(gcs_parser* p, gcs_node_kind kind) {
    gcs_node* node = gcs_new_node(p, kind);

    if (!node) { return NULL; }

    gcs_next(p);
    gcs_expect(p, "(");
    node->first = gcs_expression(p, 1);
    gcs_expect(p, ")");
    node->second = gcs_statement(p);

    if (kind == GCS_IF && gcs_take(p, "else")) { node->third = gcs_statement(p); }

    return node;
}

static gcs_node* gcs_for_statement(gcs_parser* p) {
    gcs_node* node = gcs_new_node(p, GCS_FOR);

    if (!node) { return NULL; }

    gcs_next(p);
    gcs_expect(p, "(");

    if (!gcs_is(p, ";")) { node->first = gcs_simple_statement(p, false); }

    gcs_expect(p, ";");

    if (!gcs_is(p, ";")) { node->second = gcs_expression(p, 1); }

    gcs_expect(p, ";");

    if (!gcs_is(p, ")")) { node->third = gcs_expression(p, 1); }

    gcs_expect(p, ")");
    node->fourth = gcs_statement(p);

    return node;
}

static gcs_node* gcs_statement_body(gcs_parser* p) {
    if (gcs_is(p, "if")) { return gcs_control_statement(p, GCS_IF); }
    if (gcs_is(p, "while")) { return gcs_control_statement(p, GCS_WHILE); }
    if (gcs_is(p, "for")) { return gcs_for_statement(p); }

    if (gcs_is(p, "{")) {
        gcs_node* block = gcs_new_node(p, GCS_BLOCK);

        if (!block) { return NULL; }

        gcs_next(p);
        gcs_node** insertion = &block->first;

        while (!gcs_is(p, "}") && p->token.kind != GCS_EOF && !p->compilation->diagnostic[0]) {
            *insertion = gcs_statement(p);

            if (!*insertion) { return NULL; }

            insertion = &(*insertion)->next;
        }

        gcs_expect(p, "}");

        return block;
    }

    if (gcs_is(p, "return") || gcs_is(p, "break") || gcs_is(p, "continue")) {
        gcs_node_kind kind = gcs_is(p, "return") ? GCS_RETURN : gcs_is(p, "break") ? GCS_BREAK : GCS_CONTINUE;
        gcs_node* node = gcs_new_node(p, kind);

        if (!node) { return NULL; }

        gcs_next(p);

        if (kind == GCS_RETURN && !gcs_is(p, ";")) { node->first = gcs_expression(p, 1); }

        gcs_expect(p, ";");

        return node;
    }

    return gcs_simple_statement(p, true);
}

gcs_node* gcs_statement(gcs_parser* p) {
    if (++p->depth > 128 || p->compilation->diagnostic[0]) {
        gcs_error(p->compilation, p->source->path, p->token.line, "statement nesting exceeds 128");
        --p->depth;

        return NULL;
    }

    gcs_node* node = gcs_statement_body(p);
    --p->depth;

    return node;
}
