/* C# constant integer expressions are checked even when ordinary int arithmetic wraps. */
#include "internal.h"

bool gcs_check_constant(gcs_compilation* c, gcs_method* method, gcs_node* node) {
    if (node->kind != GCS_UNARY && node->kind != GCS_BINARY) { return true; }
    if (!node->first || !node->first->constant || (node->second && !node->second->constant)) { return true; }

    i64 left = node->first->integer;
    i64 right = node->second ? node->second->integer : 0;
    i64 value = 0;
    const char* op = node->name;

    if (node->kind == GCS_UNARY) {
        value = !strcmp(op, "!") ? !left : -left;
    } else if (!strcmp(op, "+")) { value = left + right; }
    else if (!strcmp(op, "-")) { value = left - right; }
    else if (!strcmp(op, "*")) { value = left * right; }
    else if (!strcmp(op, "/") || !strcmp(op, "%")) {
        if (!right) {
            gcs_error(c, method->source->path, node->token.line, "division by zero in a constant expression");

            return false;
        }

        value = !strcmp(op, "/") ? left / right : left % right;
    } else if (!strcmp(op, "==")) { value = left == right; }
    else if (!strcmp(op, "!=")) { value = left != right; }
    else if (!strcmp(op, "<")) { value = left < right; }
    else if (!strcmp(op, ">")) { value = left > right; }
    else if (!strcmp(op, "<=")) { value = left <= right; }
    else if (!strcmp(op, ">=")) { value = left >= right; }
    else if (!strcmp(op, "&&")) { value = left && right; }
    else if (!strcmp(op, "||")) { value = left || right; }

    if (value < INT32_MIN || value > INT32_MAX) {
        gcs_error(c, method->source->path, node->token.line, "integer overflow in a constant expression");

        return false;
    }

    node->constant = true;
    node->integer = (i32)value;

    return true;
}
