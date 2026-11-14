/* Type checking binds names and calls before any target text is emitted. */
#include "internal.h"

static bool gcs_console(gcs_compilation* c, gcs_method* method, gcs_scope* scope, const char* name) {
    if (!strcmp(name, "System.Console.WriteLine") || !strcmp(name, "System.Console.Write")) { return true; }
    if (strcmp(name, "Console.WriteLine") && strcmp(name, "Console.Write")) { return false; }
    if (gcs_find_variable(scope, "Console")) { return false; }

    for (gcs_class* owner = c->classes; owner; owner = owner->next) {
        const char* short_name = strrchr(owner->name, '.');

        if (!strcmp(short_name ? short_name + 1 : owner->name, "Console")) { return false; }
    }

    if (method->source->project->implicit_usings) { return true; }

    for (gcs_using* import = method->source->imports; import; import = import->next) {
        if (!strcmp(import->name, "System")) { return true; }
    }

    return false;
}

static gcs_type gcs_check_call(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node) {
    size_t count = 0;

    for (gcs_node* argument = node->first; argument; argument = argument->next) {
        gcs_type type = gcs_check_expression(c, method, scope, argument);

        if (type == GCS_VOID || type >= GCS_INFER) { return GCS_INVALID; }

        ++count;
    }

    const char* dot = strchr(node->name, '.');
    const char* root = dot ? gcs_copy(c, node->name, (size_t)(dot - node->name)) : node->name;

    if (!root || gcs_find_variable(scope, root)) {
        gcs_error(c, method->source->path, node->token.line, "local '%s' is not a static method or namespace", root ? root : "");

        return GCS_INVALID;
    }

    if (gcs_console(c, method, scope, node->name)) {
        node->builtin = true;

        if (count > 1 || (node->first && node->first->type > GCS_STRING) ||
            (!count && !gcs_suffix(node->name, "WriteLine"))) {
            gcs_error(c, method->source->path, node->token.line, "Console supports Write(value) and WriteLine([value])");

            return GCS_INVALID;
        }

        return GCS_VOID;
    }

    node->method = gcs_resolve_method(c, method, node->name, node->token.line);

    if (!node->method) { return GCS_INVALID; }

    gcs_node* argument = node->first;
    gcs_variable* parameter = node->method->parameters;

    while (argument && parameter && argument->type == parameter->type) {
        argument = argument->next;
        parameter = parameter->next;
    }

    if (argument || parameter) {
        gcs_error(c, method->source->path, node->token.line, "argument count or type mismatch for '%s'", node->name);

        return GCS_INVALID;
    }

    return node->method->type;
}

static gcs_type gcs_check_operator(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node) {
    if (node->kind == GCS_UNARY && !strcmp(node->name, "-") && node->first &&
        node->first->kind == GCS_INTEGER && strtoull(node->first->name, NULL, 10) == 2147483648ULL) {
        node->kind = GCS_INTEGER;
        node->name = "-2147483648";
        node->integer = INT32_MIN;
        node->constant = true;
        node->first = NULL;

        return GCS_INT;
    }

    gcs_type left = gcs_check_expression(c, method, scope, node->first);
    gcs_type right = node->second ? gcs_check_expression(c, method, scope, node->second) : GCS_INVALID;

    if (node->kind == GCS_UNARY) {
        if (!strcmp(node->name, "!") && left == GCS_BOOL) { return GCS_BOOL; }
        if (!strcmp(node->name, "-") && left == GCS_INT) { return GCS_INT; }
    } else if (node->kind == GCS_ASSIGN) {
        if (node->first->kind == GCS_NAME && node->first->variable && left == right && left < GCS_INFER &&
            (!strcmp(node->name, "=") || left == GCS_INT)) { return left; }
    } else if (!strcmp(node->name, "==") || !strcmp(node->name, "!=")) {
        if (left == right && left != GCS_VOID && left < GCS_ARGS) { return GCS_BOOL; }
    } else if (!strcmp(node->name, "&&") || !strcmp(node->name, "||")) {
        if (left == GCS_BOOL && right == GCS_BOOL) { return GCS_BOOL; }
    } else if (left == GCS_INT && right == GCS_INT) {
        return strchr("<>", node->name[0]) ? GCS_BOOL : GCS_INT;
    }

    gcs_error(c, method->source->path, node->token.line, "incompatible operands for '%s'", node->name);

    return GCS_INVALID;
}

gcs_type gcs_check_expression(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node) {
    if (!node || c->diagnostic[0]) { return GCS_INVALID; }

    switch (node->kind) {
        case GCS_INTEGER: {
            char* end;
            errno = 0;
            unsigned long long value = strtoull(node->name, &end, 10);

            if (errno || *end || value > INT32_MAX) {
                gcs_error(c, method->source->path, node->token.line, "integer literal exceeds supported int range");

                return GCS_INVALID;
            }

            node->type = GCS_INT;
            node->integer = (i32)value;
            node->constant = true;

            break;
        }
        case GCS_BOOLEAN:
            node->type = GCS_BOOL;
            node->constant = true;
            node->integer = !strcmp(node->name, "true");

            break;
        case GCS_TEXT: node->type = GCS_STRING; break;
        case GCS_NAME:
            node->variable = gcs_find_variable(scope, node->name);

            if (!node->variable && gcs_suffix(node->name, ".Length")) {
                const char* name = gcs_copy(c, node->name, strlen(node->name) - 7);
                node->variable = name ? gcs_find_variable(scope, name) : NULL;

                if (node->variable && node->variable->type == GCS_ARGS) {
                    node->kind = GCS_LENGTH;
                    node->type = GCS_INT;

                    break;
                }

                node->variable = NULL;
            }

            if (!node->variable) {
                gcs_error(c, method->source->path, node->token.line, "unknown local '%s'", node->name);

                return GCS_INVALID;
            }

            node->type = node->variable->type;

            break;
        case GCS_CALL: node->type = gcs_check_call(c, method, scope, node); break;
        case GCS_UNARY: case GCS_BINARY: case GCS_ASSIGN:
            node->type = gcs_check_operator(c, method, scope, node);

            break;
        case GCS_INDEX:
            if (gcs_check_expression(c, method, scope, node->first) != GCS_ARGS ||
                gcs_check_expression(c, method, scope, node->second) != GCS_INT) {
                gcs_error(c, method->source->path, node->token.line, "only string[] command-line arguments can be indexed");

                return GCS_INVALID;
            }

            node->type = GCS_STRING;

            break;
        case GCS_UPDATE:
            if (gcs_check_expression(c, method, scope, node->first) != GCS_INT || node->first->kind != GCS_NAME) {
                gcs_error(c, method->source->path, node->token.line, "increment/decrement requires an int local");

                return GCS_INVALID;
            }

            node->type = GCS_INT;

            break;
        default: return GCS_INVALID;
    }

    if (!gcs_check_constant(c, method, node)) { return GCS_INVALID; }

    return node->type;
}
