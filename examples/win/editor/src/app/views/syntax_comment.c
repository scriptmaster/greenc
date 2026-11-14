/*
 ** syntax_comment.c - 1 function.
 ** syntax_comment_after - Track C-style block comments while skipping quoted text and line comments.
 ** See also: row_index.c, syntax.c, syntax.h.
 */
#include "syntax.h"

bool syntax_comment_after(const char* line, size_t length, bool comment) {
    char quote = 0;

    for (size_t i = 0; i < length; ++i) {
        char c = line[i];
        char next = i + 1 < length ? line[i + 1] : 0;

        if (comment) {
            if (c == '*' && next == '/') {
                comment = false;
                ++i;
            }
        } else if (quote) {
            if (c == '\\' && i + 1 < length) { ++i; }
            else if (c == quote) { quote = 0; }
        } else if (c == '"' || c == '\'') {
            quote = c;
        } else if (c == '/' && next == '/') {
            break;
        } else if (c == '/' && next == '*') {
            comment = true;
            ++i;
        }
    }

    return comment;
}
