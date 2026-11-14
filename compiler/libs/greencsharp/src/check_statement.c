/* Statement scopes, local type inference, control-flow conditions and return validation. */
#include "internal.h"

static bool gcs_condition(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* expression) {
    if (gcs_check_expression(c, method, scope, expression) == GCS_BOOL) { return true; }

    gcs_error(c, method->source->path, expression ? expression->token.line : method->line, "condition must have bool type");

    return false;
}

static bool gcs_check_local(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node) {
    gcs_type initializer = gcs_check_expression(c, method, scope, node->first);

    if (node->type == GCS_INFER) { node->type = initializer; }

    if (node->type != initializer || initializer == GCS_VOID || initializer >= GCS_INFER) {
        gcs_error(c, method->source->path, node->token.line, "initializer type does not match local '%s'", node->name);

        return false;
    }

    gcs_variable* variable = gcs_allocate(c, sizeof(*variable));

    if (!variable) { return false; }

    variable->name = node->name;
    variable->type = node->type;
    variable->id = method->next_variable++;
    node->variable = variable;

    return gcs_declare_variable(c, method, scope, variable, node->token.line);
}

bool gcs_check_statement(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node, size_t loops) {
    if (!node || c->diagnostic[0]) { return false; }

    switch (node->kind) {
        case GCS_BLOCK: {
            gcs_scope nested = {.parent = scope};

            for (gcs_node* statement = node->first; statement; statement = statement->next) {
                if (!gcs_check_statement(c, method, &nested, statement, loops)) { return false; }
            }

            return true;
        }
        case GCS_DECLARE: return gcs_check_local(c, method, scope, node);
        case GCS_EXPRESSION:
            if (node->first && (node->first->kind == GCS_CALL || node->first->kind == GCS_ASSIGN || node->first->kind == GCS_UPDATE)) {
                return gcs_check_expression(c, method, scope, node->first) != GCS_INVALID;
            }

            gcs_error(c, method->source->path, node->token.line, "expression statement must be a call or assignment");

            return false;
        case GCS_RETURN: {
            gcs_type value = node->first ? gcs_check_expression(c, method, scope, node->first) : GCS_VOID;

            if ((!node->first && method->top_level) || (value == method->type && (!node->first || value != GCS_VOID))) { return true; }

            gcs_error(c, method->source->path, node->token.line, "return type mismatch in '%s'", method->name);

            return false;
        }
        case GCS_IF:
            return gcs_condition(c, method, scope, node->first) &&
                   gcs_check_statement(c, method, scope, node->second, loops) &&
                   (!node->third || gcs_check_statement(c, method, scope, node->third, loops));
        case GCS_WHILE:
            return gcs_condition(c, method, scope, node->first) &&
                   gcs_check_statement(c, method, scope, node->second, loops + 1);
        case GCS_FOR: {
            gcs_scope nested = {.parent = scope};

            if (node->first && !gcs_check_statement(c, method, &nested, node->first, loops)) { return false; }
            if (node->second && !gcs_condition(c, method, &nested, node->second)) { return false; }

            if (node->third && node->third->kind != GCS_CALL && node->third->kind != GCS_ASSIGN && node->third->kind != GCS_UPDATE) {
                gcs_error(c, method->source->path, node->token.line, "for step must be a call or assignment");

                return false;
            }

            return (!node->third || gcs_check_expression(c, method, &nested, node->third) != GCS_INVALID) &&
                   gcs_check_statement(c, method, &nested, node->fourth, loops + 1);
        }
        case GCS_BREAK: case GCS_CONTINUE:
            if (loops) { return true; }

            gcs_error(c, method->source->path, node->token.line, "break/continue outside a loop");

            return false;
        default: return false;
    }
}
