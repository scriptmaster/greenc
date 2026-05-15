#pragma once

#include "ast_nodes.h"
#include "../libs/standard.h"

typedef struct {
    char* str;
    size_t len;
    size_t cap;
} greenc_emit_buf;

static inline void greenc_emit_buf_init(greenc_emit_buf* b) {
    b->str = NULL;
    b->len = 0;
    b->cap = 0;
}

static inline void greenc_emit_buf_reserve(greenc_emit_buf* b, size_t extra) {
    size_t need = b->len + extra + 1;
    if (need <= b->cap) {
        return;
    }

    size_t nc = b->cap ? b->cap : 128;
    while (nc < need) {
        nc *= 2;
    }

    char* nd = (char*)greenc_malloc(nc);
    if (!nd) {
        return;
    }

    if (b->str && b->len > 0) {
        greenc_memcpy(nd, b->str, b->len);
    }

    if (b->str) {
        greenc_free(b->str);
    }

    b->str = nd;
    b->cap = nc;
}

static inline void greenc_emit_buf_append_str(greenc_emit_buf* b, const char* s) {
    size_t sl = greenc_strlen(s);
    greenc_emit_buf_reserve(b, sl);
    if (!b->str) {
        return;
    }

    greenc_memcpy(b->str + b->len, s, sl);
    b->len += sl;
    b->str[b->len] = '\0';
}

static inline void greenc_emit_buf_append_span(
    greenc_emit_buf* b,
    const char* s,
    int len
) {
    greenc_emit_buf_reserve(b, (size_t)len);
    if (!b->str) {
        return;
    }

    greenc_memcpy(b->str + b->len, s, (size_t)len);
    b->len += (size_t)len;
    b->str[b->len] = '\0';
}

static inline void greenc_emit_buf_append_char(greenc_emit_buf* b, char c) {
    greenc_emit_buf_reserve(b, 1);
    if (!b->str) {
        return;
    }

    b->str[b->len++] = c;
    b->str[b->len] = '\0';
}

static inline void greenc_emit_string_lit(greenc_emit_buf* b, greenc_token t) {
    greenc_emit_buf_append_str(b, "\"");

    for (int i = 0; i < t.len; i++) {
        char c = t.start[i];

        if (c == '\\') {
            greenc_emit_buf_append_str(b, "\\\\");
        } else if (c == '"') {
            greenc_emit_buf_append_str(b, "\\\"");
        } else if (c == '\n') {
            greenc_emit_buf_append_str(b, "\\n");
        } else if (c == '\r') {
            greenc_emit_buf_append_str(b, "\\r");
        } else if (c == '\t') {
            greenc_emit_buf_append_str(b, "\\t");
        } else {
            greenc_emit_buf_append_char(b, c);
        }
    }

    greenc_emit_buf_append_str(b, "\"");
}

static int greenc_emit_expr_has_print_call(greenc_expr* e);

static int greenc_emit_call_has_print(greenc_expr* e) {
    if (!e || e->kind != GREENC_EXPR_CALL) {
        return 0;
    }

    if (
        e->callee &&
        e->callee->kind == GREENC_EXPR_IDENT &&
        greenc_str_equals(
            e->callee->tok.start,
            e->callee->tok.len,
            "print"
        )
    ) {
        return 1;
    }

    if (greenc_emit_expr_has_print_call(e->callee)) {
        return 1;
    }

    for (int i = 0; i < e->arg_count; i++) {
        if (greenc_emit_expr_has_print_call(e->args[i])) {
            return 1;
        }
    }

    return 0;
}

static int greenc_emit_expr_has_print_call(greenc_expr* e) {
    if (!e) {
        return 0;
    }

    switch (e->kind) {
        case GREENC_EXPR_CALL:
            return greenc_emit_call_has_print(e);
        case GREENC_EXPR_NEG:
            return greenc_emit_expr_has_print_call(e->inner);
        default:
            return 0;
    }
}

static int greenc_emit_program_uses_print(greenc_program* pr) {
    if (!pr) {
        return 0;
    }

    for (int fi = 0; fi < pr->func_count; fi++) {
        greenc_func* gfunc = pr->funcs[fi];

        for (int si = 0; si < gfunc->stmt_count; si++) {
            greenc_stmt* st = gfunc->stmts[si];

            if (
                st &&
                st->expr &&
                greenc_emit_expr_has_print_call(st->expr)
            ) {
                return 1;
            }
        }
    }

    return 0;
}

static void greenc_emit_expr(greenc_emit_buf* b, greenc_expr* e, int depth);

static void greenc_emit_expr(greenc_emit_buf* b, greenc_expr* e, int depth) {
    if (!e || !b->str) {
        return;
    }

    (void)depth;

    switch (e->kind) {
        case GREENC_EXPR_INT:
            greenc_emit_buf_append_span(b, e->tok.start, e->tok.len);
            break;

        case GREENC_EXPR_STRING:
            greenc_emit_string_lit(b, e->tok);
            break;

        case GREENC_EXPR_IDENT:
            greenc_emit_buf_append_span(b, e->tok.start, e->tok.len);
            break;

        case GREENC_EXPR_NEG:
            greenc_emit_buf_append_str(b, "-");
            if (
                e->inner &&
                e->inner->kind == GREENC_EXPR_NEG
            ) {
                greenc_emit_buf_append_str(b, "(");
                greenc_emit_expr(b, e->inner, depth + 1);
                greenc_emit_buf_append_str(b, ")");
            } else {
                greenc_emit_expr(b, e->inner, depth + 1);
            }
            break;

        case GREENC_EXPR_CALL:
            greenc_emit_expr(b, e->callee, depth + 1);
            greenc_emit_buf_append_str(b, "(");

            for (int i = 0; i < e->arg_count; i++) {
                if (i > 0) {
                    greenc_emit_buf_append_str(b, ", ");
                }

                greenc_emit_expr(b, e->args[i], depth + 1);
            }

            greenc_emit_buf_append_str(b, ")");
            break;

        default:
            break;
    }
}

static void greenc_emit_stmt(greenc_emit_buf* b, greenc_stmt* st, int indent) {
    if (!st || !b->str) {
        return;
    }

    for (int i = 0; i < indent; i++) {
        greenc_emit_buf_append_str(b, " ");
    }

    if (st->kind == GREENC_STMT_RETURN) {
        greenc_emit_buf_append_str(b, "return ");
        greenc_emit_expr(b, st->expr, 0);
        greenc_emit_buf_append_str(b, ";\n");
        return;
    }

    greenc_emit_expr(b, st->expr, 0);
    greenc_emit_buf_append_str(b, ";\n");
}

static void greenc_emit_func(greenc_emit_buf* b, greenc_func* gfunc) {
    if (!gfunc || !b->str) {
        return;
    }

    greenc_emit_buf_append_str(b, "int ");
    greenc_emit_buf_append_span(b, gfunc->name.start, gfunc->name.len);
    greenc_emit_buf_append_str(b, "(void) {\n");

    for (int i = 0; i < gfunc->stmt_count; i++) {
        greenc_emit_stmt(b, gfunc->stmts[i], 4);
    }

    greenc_emit_buf_append_str(b, "}\n");
}

static char* greenc_emit_program(
    greenc_program* pr,
    const char* gcc_source_path,
    const char* output_exe_path
) {
    if (!pr || !gcc_source_path || !output_exe_path) {
        return NULL;
    }

    greenc_emit_buf buf;
    greenc_emit_buf_init(&buf);

    int needs_print = greenc_emit_program_uses_print(pr);

    greenc_emit_buf_append_str(&buf, "/* gcc -o ");
    greenc_emit_buf_append_str(&buf, output_exe_path);
    greenc_emit_buf_append_str(&buf, " ");
    greenc_emit_buf_append_str(&buf, gcc_source_path);
    greenc_emit_buf_append_str(&buf, " -std=c17 */\n");

    greenc_emit_buf_append_str(&buf, "\n");

    if (needs_print) {
        greenc_emit_buf_append_str(&buf, "#define print puts\n");
    }

    greenc_emit_buf_append_str(&buf, "\n");

    if (needs_print) {
        greenc_emit_buf_append_str(&buf, "#include <stdio.h>\n");
    }

    greenc_emit_buf_append_str(&buf, "\n");

    for (int i = 0; i < pr->func_count; i++) {
        if (i > 0) {
            greenc_emit_buf_append_str(&buf, "\n");
        }

        greenc_emit_func(&buf, pr->funcs[i]);
    }

    if (!buf.str) {
        return NULL;
    }

    return buf.str;
}
