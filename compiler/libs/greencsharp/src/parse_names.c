/* Identifier paths, supported scalar types and bounded AST allocation. */
#include "internal.h"

const char* gcs_identifier(gcs_parser* p) {
    if (p->token.kind != GCS_IDENTIFIER) {
        gcs_error(p->compilation, p->source->path, p->token.line, "expected identifier");

        return "";
    }

    const char* reserved[] = {"abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked",
        "class", "const", "continue", "decimal", "default", "delegate", "do", "double", "else", "enum", "event",
        "explicit", "extern", "false", "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit", "in",
        "int", "interface", "internal", "is", "lock", "long", "namespace", "new", "null", "object", "operator", "out",
        "override", "params", "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed", "short",
        "sizeof", "stackalloc", "static", "string", "struct", "switch", "this", "throw", "true", "try", "typeof",
        "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "virtual", "void", "volatile", "while"};

    for (size_t i = 0; i < sizeof(reserved) / sizeof(*reserved); ++i) {
        if (gcs_is(p, reserved[i])) {
            gcs_error(p->compilation, p->source->path, p->token.line, "reserved keyword cannot be an identifier");

            return "";
        }
    }

    const char* name = gcs_copy(p->compilation, p->token.text, p->token.length);
    gcs_next(p);

    return name ? name : "";
}

gcs_type gcs_parse_type(gcs_parser* p, bool infer) {
    const char* names[] = {"void", "int", "bool", "string", "var"};

    for (size_t i = 0; i < (infer ? 5u : 4u); ++i) {
        if (gcs_take(p, names[i])) {
            if (i == GCS_STRING && gcs_take(p, "[")) {
                gcs_expect(p, "]");

                return GCS_ARGS;
            }

            return i == 4 ? GCS_INFER : (gcs_type)i;
        }
    }

    gcs_error(p->compilation, p->source->path, p->token.line, "unsupported or missing type");

    return GCS_INVALID;
}

bool gcs_is_declaration(gcs_parser* p) {
    return gcs_is(p, "int") || gcs_is(p, "bool") || gcs_is(p, "string") || gcs_is(p, "var");
}

gcs_node* gcs_new_node(gcs_parser* p, gcs_node_kind kind) {
    if (++p->compilation->node_count > 100000) {
        gcs_error(p->compilation, p->source->path, p->token.line, "AST node limit exceeded (100000)");

        return NULL;
    }

    gcs_node* node = gcs_allocate(p->compilation, sizeof(*node));

    if (node) {
        node->kind = kind;
        node->type = GCS_INVALID;
        node->token = p->token;
        node->height = 1;
    }

    return node;
}
