/* Precedence-based expression AST; no C text is substituted for source syntax. */
#include "internal.h"

static size_t gcs_precedence(const gcs_parser* p) {
    if (gcs_is(p, "=") || gcs_is(p, "+=") || gcs_is(p, "-=") || gcs_is(p, "*=") || gcs_is(p, "/=") || gcs_is(p, "%=")) { return 1; }
    if (gcs_is(p, "||")) { return 2; }
    if (gcs_is(p, "&&")) { return 3; }
    if (gcs_is(p, "==") || gcs_is(p, "!=")) { return 4; }
    if (gcs_is(p, "<") || gcs_is(p, ">") || gcs_is(p, "<=") || gcs_is(p, ">=")) { return 5; }
    if (gcs_is(p, "+") || gcs_is(p, "-")) { return 6; }
    if (gcs_is(p, "*") || gcs_is(p, "/") || gcs_is(p, "%")) { return 7; }

    return 0;
}

static void gcs_parse_arguments(gcs_parser* p, gcs_node* node) {
    if (!gcs_expect(p, "(")) { return; }

    gcs_node** argument = &node->first;

    if (!gcs_is(p, ")")) {
        do {
            *argument = gcs_expression(p, 1);

            if (!*argument) { return; }

            argument = &(*argument)->next;
        } while (gcs_take(p, ","));
    }

    gcs_expect(p, ")");
}

static gcs_node* gcs_primary(gcs_parser* p) {
    gcs_node* node = NULL;

    if (gcs_take(p, "(")) {
        node = gcs_expression(p, 1);
        gcs_expect(p, ")");

        return node;
    }

    if (gcs_is(p, "-") || gcs_is(p, "!") || gcs_is(p, "++") || gcs_is(p, "--")) {
        node = gcs_new_node(p, gcs_is(p, "++") || gcs_is(p, "--") ? GCS_UPDATE : GCS_UNARY);

        if (!node) { return NULL; }

        node->name = gcs_copy(p->compilation, p->token.text, p->token.length);
        gcs_next(p);
        node->first = gcs_expression(p, 8);

        return node;
    }

    gcs_node_kind kind = p->token.kind == GCS_NUMBER ? GCS_INTEGER :
                         p->token.kind == GCS_LITERAL ? GCS_TEXT :
                         gcs_is(p, "true") || gcs_is(p, "false") ? GCS_BOOLEAN : GCS_NAME;
    node = gcs_new_node(p, kind);

    if (!node) { return NULL; }

    if (kind != GCS_NAME) {
        node->name = gcs_copy(p->compilation, p->token.text, p->token.length);
        gcs_next(p);

        return node;
    }

    node->name = gcs_identifier(p);

    while (gcs_take(p, ".") && !p->compilation->diagnostic[0]) {
        const char* member = gcs_identifier(p);
        const char* prefix = gcs_join(p->compilation, node->name, ".");
        node->name = prefix ? gcs_join(p->compilation, prefix, member) : NULL;

        if (!node->name) { return NULL; }
    }

    if (gcs_is(p, "(")) {
        node->kind = GCS_CALL;
        gcs_parse_arguments(p, node);
    }

    if (gcs_take(p, "[")) {
        gcs_node* index = gcs_new_node(p, GCS_INDEX);

        if (!index) { return NULL; }

        index->first = node;
        index->second = gcs_expression(p, 1);
        gcs_expect(p, "]");
        node = index;
    }

    if (gcs_is(p, "++") || gcs_is(p, "--")) {
        gcs_node* update = gcs_new_node(p, GCS_UPDATE);

        if (!update) { return NULL; }

        update->first = node;
        update->postfix = true;
        update->name = gcs_copy(p->compilation, p->token.text, p->token.length);
        gcs_next(p);
        node = update;
    }

    return node;
}

static bool gcs_expression_height(gcs_parser* p, gcs_node* node) {
    if (!node) { return false; }

    size_t height = 0;
    gcs_node* children[] = {node->first, node->second};

    for (size_t i = 0; i < 2; ++i) {
        for (gcs_node* child = children[i]; child; child = child->next) {
            if (child->height > height) { height = child->height; }
        }
    }

    node->height = height + 1;

    if (node->height > 128) {
        gcs_error(p->compilation, p->source->path, node->token.line, "expression tree depth exceeds 128");

        return false;
    }

    return true;
}

gcs_node* gcs_expression(gcs_parser* p, size_t minimum) {
    if (++p->depth > 128 || p->compilation->diagnostic[0]) {
        gcs_error(p->compilation, p->source->path, p->token.line, "expression nesting exceeds 128");
        --p->depth;

        return NULL;
    }

    gcs_node* left = gcs_primary(p);

    if (!gcs_expression_height(p, left)) {
        --p->depth;

        return NULL;
    }

    size_t precedence;

    while (left && (precedence = gcs_precedence(p)) >= minimum && precedence && !p->compilation->diagnostic[0]) {
        gcs_node* binary = gcs_new_node(p, precedence == 1 ? GCS_ASSIGN : GCS_BINARY);

        if (!binary) {
            left = NULL;

            break;
        }

        binary->name = gcs_copy(p->compilation, p->token.text, p->token.length);
        gcs_next(p);
        binary->first = left;
        binary->second = gcs_expression(p, precedence + (precedence != 1));
        left = binary;

        if (!gcs_expression_height(p, left)) { break; }
    }

    --p->depth;

    return left;
}
