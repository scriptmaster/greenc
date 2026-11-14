/* Source tokens preserve locations; unsupported literal forms fail before lowering. */
#include "internal.h"
#include <ctype.h>

static void gcs_skip(gcs_parser* p) {
    for (;;) {
        while (*p->cursor && isspace((unsigned char)*p->cursor)) {
            if (*p->cursor++ == '\n') { ++p->line; }
        }

        if (!strncmp(p->cursor, "//", 2)) {
            while (*p->cursor && *p->cursor != '\n') { ++p->cursor; }
        } else if (!strncmp(p->cursor, "/*", 2)) {
            p->cursor += 2;

            while (*p->cursor && strncmp(p->cursor, "*/", 2)) {
                if (*p->cursor++ == '\n') { ++p->line; }
            }

            if (!*p->cursor) {
                gcs_error(p->compilation, p->source->path, p->line, "unterminated comment");

                return;
            }

            p->cursor += 2;
        } else {
            return;
        }
    }
}

void gcs_next(gcs_parser* p) {
    gcs_skip(p);
    const char* start = p->cursor;
    p->token = (gcs_token){.kind = GCS_EOF, .text = start, .line = p->line};

    if (!*start || p->compilation->diagnostic[0]) { return; }

    if (isalpha((unsigned char)*start) || *start == '_') {
        p->token.kind = GCS_IDENTIFIER;

        do { ++p->cursor; } while (isalnum((unsigned char)*p->cursor) || *p->cursor == '_');
    } else if (isdigit((unsigned char)*start)) {
        p->token.kind = GCS_NUMBER;

        do { ++p->cursor; } while (isdigit((unsigned char)*p->cursor));
    } else if (*start == '"') {
        p->token.kind = GCS_LITERAL;
        ++p->cursor;

        while (*p->cursor && *p->cursor != '"') {
            if ((unsigned char)*p->cursor < 32 || (unsigned char)*p->cursor > 126) {
                gcs_error(p->compilation, p->source->path, p->line, "only single-line ASCII string literals are supported");

                break;
            }

            if (*p->cursor++ == '\\') {
                if (!*p->cursor || !strchr("nrt\\\"", *p->cursor)) {
                    gcs_error(p->compilation, p->source->path, p->line, "unsupported string escape");

                    break;
                }

                ++p->cursor;
            }
        }

        if (*p->cursor != '"') {
            gcs_error(p->compilation, p->source->path, p->line, "unterminated string literal");
        } else {
            ++p->cursor;
        }
    } else {
        p->token.kind = GCS_PUNCTUATION;
        ++p->cursor;

        if (start[1] && ((strchr("=!<>+-*/%", start[0]) && start[1] == '=') ||
            ((start[0] == '+' || start[0] == '-') && start[1] == start[0]) ||
            (start[0] == '&' && start[1] == '&') || (start[0] == '|' && start[1] == '|'))) { ++p->cursor; }
    }

    p->token.length = (size_t)(p->cursor - start);
}

bool gcs_is(const gcs_parser* p, const char* text) {
    return strlen(text) == p->token.length && !memcmp(p->token.text, text, p->token.length);
}

bool gcs_take(gcs_parser* p, const char* text) {
    if (!gcs_is(p, text)) { return false; }

    gcs_next(p);

    return true;
}

bool gcs_expect(gcs_parser* p, const char* text) {
    if (gcs_take(p, text)) { return true; }

    gcs_error(p->compilation, p->source->path, p->token.line, "expected '%s', found '%.*s'", text,
              (int)p->token.length, p->token.text);

    return false;
}
