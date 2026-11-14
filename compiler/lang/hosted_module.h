#pragma once
#include <stdbool.h>
#include "compile_pipeline.h"

/* Hosted modules keep C declarators, expressions and preprocessing intact for
 * the native backend. Parse scopes and statements before emitting any file.
 * A typed local "let T value" lowers to "T value". */
typedef struct {
    char* cursor;
    const char* token;
    size_t length;
    unsigned depth;
    bool failed;
    struct ownership_flow* ownership;
} hosted_parser;

static bool hosted_word(hosted_parser* p, const char* text) {
    return greenc_strlen(text) == p->length && greenc_str_equals(p->token, p->length, text);
}

static bool hosted_identifier(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static void hosted_next(hosted_parser* p) {
    char* s = p->cursor;
    for (;;) {
        while (*s && (unsigned char)*s <= ' ') { ++s; }

        if (*s == '#' || (s[0] == '/' && s[1] == '/')) {
            while (*s && (*s != '\n' || (s[-1] == '\\' || (s[-1] == '\r' && s[-2] == '\\')))) { ++s; }

            continue;
        }

        if (s[0] == '/' && s[1] == '*') {
            s += 2;
            while (*s && !(s[0] == '*' && s[1] == '/')) { ++s; }
            if (!*s) { p->failed = true; break; }
            s += 2;

            continue;
        }

        break;
    }

    p->token = s;
    if (*s == '"' || *s == '\'') {
        char quote = *s++;
        while (*s && *s != quote && *s != '\n') {
            if (*s == '\\' && s[1]) { ++s; }
            ++s;
        }

        if (*s == quote) { ++s; } else { p->failed = true; }
    } else if (hosted_identifier(*s)) {
        do { ++s; } while (hosted_identifier(*s));
    } else if (*s) {
        ++s;
    }

    p->cursor = s;
    p->length = (size_t)(s - p->token);
}

static bool hosted_take(hosted_parser* p, const char* token) {
    if (!hosted_word(p, token)) { return false; }
    hosted_next(p);

    return true;
}

static bool hosted_group(hosted_parser* p) {
    const char* end = hosted_word(p, "(") ? ")" : hosted_word(p, "[") ? "]" : "}";
    if (++p->depth > 256) { return false; }
    hosted_next(p);
    while (p->length && !p->failed && !hosted_word(p, end)) {
        if (hosted_word(p, "(") || hosted_word(p, "[") || hosted_word(p, "{")) {
            if (!hosted_group(p)) { return false; }
        } else if (hosted_word(p, ")") || hosted_word(p, "]") || hosted_word(p, "}")) {
            return false;
        } else {
            hosted_next(p);
        }
    }

    --p->depth;
    return hosted_take(p, end);
}

static bool hosted_until(hosted_parser* p, const char* end) {
    while (p->length && !p->failed && !hosted_word(p, end)) {
        if (hosted_word(p, "(") || hosted_word(p, "[") || hosted_word(p, "{")) {
            if (!hosted_group(p)) { return false; }
        } else if (hosted_word(p, "}") || hosted_word(p, ")") || hosted_word(p, "]")) {
            return false;
        } else {
            hosted_next(p);
        }
    }

    return hosted_take(p, end);
}

#include "ownership.h"

static bool hosted_statement(hosted_parser* p);

static bool hosted_body(hosted_parser* p) {
    if (hosted_take(p, "{")) {
        size_t scope = p->ownership->count;
        while (p->length && !hosted_word(p, "}")) {
            if (!hosted_statement(p)) { return false; }
        }

        return ownership_leave(p, scope) && hosted_take(p, "}");
    }

    bool conditional = hosted_word(p, "if");
    if (conditional || hosted_word(p, "while") || hosted_word(p, "for") || hosted_word(p, "switch")) {
        bool loop = hosted_word(p, "for") || hosted_word(p, "while");
        hosted_next(p);
        const char* condition = p->token;
        if (!hosted_word(p, "(") || !hosted_group(p)) { return false; }
        if (!ownership_expression(p, condition, p->token, false)) { return false; }

        return ownership_branch(p, condition, conditional, loop);
    }

    if (hosted_take(p, "do")) {
        return hosted_statement(p) && hosted_take(p, "while") && hosted_word(p, "(") &&
               hosted_group(p) && hosted_take(p, ";");
    }

    if (hosted_word(p, "case") || hosted_word(p, "default")) {
        return hosted_until(p, ":") && hosted_statement(p);
    }

    if (hosted_word(p, "else") || hosted_word(p, "}")) { return false; }
    if (hosted_word(p, "let")) {
        greenc_memcpy(p->cursor - 3, "   ", 3);
        hosted_next(p);
        if (!p->length || !hosted_identifier(*p->token)) { return false; }
    }

    const char* start = p->token;
    if (!hosted_until(p, ";")) { return false; }

    return ownership_statement(p, start, p->token);
}

static bool hosted_statement(hosted_parser* p) {
    if (++p->depth > 256) { return false; }
    bool parsed = hosted_body(p);
    --p->depth;

    return parsed;
}

static bool hosted_declaration(hosted_parser* p) {
    hosted_parser previous = *p, function = {0}, parameters_start = {0};
    bool initializer = false;
    bool parameters = false;
    while (p->length && !p->failed) {
        if (hosted_take(p, ";")) { return true; }
        if (hosted_word(p, "{") && parameters && !initializer) {
            p->ownership->reachable = true;
            if (!contract_enter(p, function, parameters_start)) { return false; }
            if (!hosted_statement(p) || !ownership_leave(p, 0)) { return false; }
            p->ownership->return_contract = -1;
            return true;
        }
        if (hosted_word(p, "(") || hosted_word(p, "[") || hosted_word(p, "{")) {
            parameters = hosted_word(p, "(");
            if (parameters) { function = previous; parameters_start = *p; }
            if (!hosted_group(p)) { return false; }
        } else {
            if (hosted_word(p, "}") || hosted_word(p, ")") || hosted_word(p, "]")) { return false; }
            previous = *p;
            if (hosted_word(p, "=")) { initializer = true; }
            hosted_next(p);
        }
    }

    return false;
}

static bool greenc_check_hosted_source(char* source, const char* path, const char* contracts) {
    ownership_flow flow = {.reachable = true, .contracts = contracts, .return_contract = -1};
    hosted_parser parser = {.cursor = source, .ownership = &flow};
    hosted_next(&parser);
    while (parser.length && !parser.failed) {
        if (!hosted_declaration(&parser)) { parser.failed = true; }
    }

    if (parser.failed) {
        print("greenc: invalid hosted module");
        print(path);
        if (parser.length) { sys_write(parser.token, parser.length); sys_write("\n", 1); }
    }

    greenc_free(flow.bindings);
    return !parser.failed;
}

static bool greenc_check_annotated_source(char* source, const char* path) {
    hosted_parser token = ownership_tokens(source);
    while (token.length) {
        if (hosted_word(&token, ";") && ownership_marker(token.cursor) >= 0) {
            return greenc_check_hosted_source(source, path, NULL);
        }
        hosted_next(&token);
    }

    return true;
}

static int greenc_emit_hosted_module(const greenc_args* cli) {
    if (cli->input_files.count != 1 || !cli->output_file) {
        print("greenc: --hosted-module requires one input and -o generated.c");
        return 1;
    }

    char* source = greenc_read_file(cli->input_files.items[0]);
    if (!source) { return 1; }

    char* contracts = cli->contracts_file ? greenc_read_file(cli->contracts_file) : NULL;
    bool valid = (!cli->contracts_file || contracts) && contract_validate(contracts) &&
                 greenc_check_hosted_source(source, cli->input_files.items[0], contracts);
    greenc_free(contracts);
    char* path = valid ? greenc_output_exe_abs_path_alloc(cli->input_files.items[0]) : NULL;
    /* Preserve hosted source locations for diagnostics and native debuggers. */
    for (char* c = path; c && *c; ++c) {
        if (*c == '"' || *c == '\n' || *c == '\r') { valid = false; }
    }

    char* mapped = valid && path ? greenc_concat("#line 1 \"", path, "\"\n", source) : NULL;
    int result = !mapped || greenc_write_file(cli->output_file, mapped) != 0;
    greenc_free(mapped);
    greenc_free(path);
    release(source);
    return result;
}
