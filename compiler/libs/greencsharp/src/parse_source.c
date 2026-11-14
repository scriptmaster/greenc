/* Namespace/import declarations and top-level or class-based entry points. */
#include "internal.h"

static const char* gcs_qualified(gcs_parser* p) {
    const char* name = gcs_identifier(p);

    while (gcs_take(p, ".")) {
        const char* member = gcs_identifier(p);
        const char* prefix = gcs_join(p->compilation, name, ".");
        name = prefix ? gcs_join(p->compilation, prefix, member) : NULL;

        if (!name) { return ""; }
    }

    return name;
}

bool gcs_parse_classes(gcs_parser* p, const char* space, bool nested) {
    if (++p->depth > 128) {
        gcs_error(p->compilation, p->source->path, p->token.line, "namespace nesting exceeds 128");

        return false;
    }

    while (p->token.kind != GCS_EOF && !gcs_is(p, "}") && !p->compilation->diagnostic[0]) {
        if (gcs_take(p, "using")) {
            if (nested || *space) {
                gcs_error(p->compilation, p->source->path, p->token.line, "using directives must precede namespaces in this implementation");

                return false;
            }

            gcs_using* import = gcs_allocate(p->compilation, sizeof(*import));

            if (!import) { return false; }

            import->name = gcs_qualified(p);
            import->next = p->source->imports;
            p->source->imports = import;
            gcs_expect(p, ";");

            continue;
        }

        if (gcs_take(p, "namespace")) {
            const char* name = gcs_qualified(p);
            const char* prefix = *space ? gcs_join(p->compilation, space, ".") : "";
            const char* full = prefix ? gcs_join(p->compilation, prefix, name) : NULL;

            if (!full) { return false; }

            if (gcs_take(p, ";")) {
                space = full;
            } else {
                gcs_expect(p, "{");
                gcs_parse_classes(p, full, true);
                gcs_expect(p, "}");
            }

            continue;
        }

        bool exported = gcs_take(p, "public");

        if (!exported) { gcs_take(p, "internal"); }

        bool is_static = gcs_take(p, "static");

        if (gcs_is(p, "interface")) {
            gcs_error(p->compilation, p->source->path, p->token.line, "interfaces and instance dispatch are not implemented");

            return false;
        }

        if (gcs_is(p, "class")) {
            if (!gcs_parse_class(p, space, exported)) { return false; }
        } else if (!nested && !*space && !exported && !is_static) {
            gcs_method* method = gcs_allocate(p->compilation, sizeof(*method));

            if (!method) { return false; }

            method->name = "<top-level>";
            method->type = GCS_INT;
            method->source = p->source;
            method->id = p->compilation->method_count++;
            method->top_level = true;
            method->is_static = true;
            method->parameters = gcs_allocate(p->compilation, sizeof(*method->parameters));

            if (!method->parameters) { return false; }

            method->parameters->name = "args";
            method->parameters->type = GCS_ARGS;
            method->parameters->id = p->compilation->variable_count++;
            method->body = gcs_new_node(p, GCS_BLOCK);

            if (!method->body) { return false; }

            gcs_node** insertion = &method->body->first;

            while (p->token.kind != GCS_EOF && !p->compilation->diagnostic[0]) {
                *insertion = gcs_statement(p);

                if (!*insertion) { return false; }

                insertion = &(*insertion)->next;
            }

            method->next = p->compilation->methods;
            p->compilation->methods = method;
        } else {
            gcs_error(p->compilation, p->source->path, p->token.line, "expected namespace or class declaration");
        }
    }

    --p->depth;

    return !p->compilation->diagnostic[0];
}

bool gcs_parse_source(gcs_compilation* c, gcs_source* source) {
    if (!source->text) { source->text = gcs_read(c, source->path); }
    if (!source->text) { return false; }

    const char* text = source->text;

    if (!strncmp(text, "\xef\xbb\xbf", 3)) { text += 3; }

    gcs_parser parser = {.compilation = c, .source = source, .cursor = text, .line = 1};
    gcs_next(&parser);
    bool success = gcs_parse_classes(&parser, "", false);

    if (success && parser.token.kind != GCS_EOF) {
        gcs_error(c, source->path, parser.token.line, "unexpected closing brace");

        return false;
    }

    return success;
}
