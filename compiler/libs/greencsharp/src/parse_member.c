/* Static method declarations and imported signatures share one bounded parser. */
#include "internal.h"

static gcs_variable* gcs_member_parameters(gcs_parser* p) {
    gcs_variable* first = NULL;
    gcs_variable** insertion = &first;
    size_t id = 0;

    if (!gcs_expect(p, "(")) { return NULL; }
    if (gcs_take(p, ")")) { return NULL; }

    do {
        gcs_variable* parameter = gcs_allocate(p->compilation, sizeof(*parameter));

        if (!parameter) { return NULL; }

        parameter->type = gcs_parse_type(p, false);
        parameter->name = gcs_identifier(p);
        parameter->id = id++;
        *insertion = parameter;
        insertion = &parameter->next;
    } while (gcs_take(p, ",") && !p->compilation->diagnostic[0]);

    gcs_expect(p, ")");

    return first;
}

static bool gcs_member_body(gcs_parser* p, gcs_method* method) {
    if (p->source->imported && gcs_take(p, ";")) { return true; }

    if (gcs_take(p, "=")) {
        gcs_expect(p, ">");
        method->body = gcs_new_node(p, GCS_BLOCK);
        gcs_node* statement = gcs_new_node(p, method->type == GCS_VOID ? GCS_EXPRESSION : GCS_RETURN);

        if (!method->body || !statement) { return false; }

        statement->first = gcs_expression(p, 1);
        method->body->first = statement;

        return gcs_expect(p, ";");
    }

    if (!gcs_is(p, "{")) {
        gcs_error(p->compilation, p->source->path, method->line, "expected method/constructor body; properties are unsupported");

        return false;
    }

    method->body = gcs_statement(p);

    return !p->compilation->diagnostic[0];
}

bool gcs_parse_member(gcs_parser* p) {
    u32 access = 0;

    if (gcs_take(p, "public")) { access = 2; }
    else if (gcs_take(p, "internal")) { access = 1; }
    else { gcs_take(p, "private"); }

    if (!gcs_take(p, "static")) {
        gcs_error(p->compilation, p->source->path, p->token.line, "only static methods are implemented; instance members and constructors are unsupported");

        return false;
    }

    size_t line = p->token.line;
    gcs_type type = gcs_parse_type(p, false);
    const char* name = gcs_identifier(p);

    gcs_method* method = gcs_allocate(p->compilation, sizeof(*method));

    if (!method) { return false; }

    method->name = name;
    method->type = type;
    method->owner = p->owner;
    method->source = p->source;
    method->access = access;
    method->is_static = true;
    method->line = line;
    method->id = p->compilation->method_count++;
    method->parameters = gcs_member_parameters(p);

    if (!gcs_member_body(p, method)) { return false; }

    method->next = p->compilation->methods;
    p->compilation->methods = method;

    return !p->compilation->diagnostic[0];
}
