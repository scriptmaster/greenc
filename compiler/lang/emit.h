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

        case GREENC_EXPR_BINOP:
            greenc_emit_expr(b, e->inner, depth + 1);  /// left
            greenc_emit_buf_append_str(b, " ");
            greenc_emit_buf_append_span(b, e->tok.start, e->tok.len); /// operator
            greenc_emit_buf_append_str(b, " ");
            greenc_emit_expr(b, e->callee, depth + 1); /// right
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

    if (st->kind == GREENC_STMT_COMMENT) {
        if (st->tok.kind == TOK_MARKER_COMMENT) {
            /// reconstruct ///  prefix — lex_marker_slash strips it
            greenc_emit_buf_append_str(b, "/// ");
            greenc_emit_buf_append_span(b, st->tok.start, st->tok.len);
        } else {
            /// TOK_LINE_COMMENT  text already starts with "//"
            /// TOK_BLOCK_COMMENT text already starts with "/*" and ends with "*/"
            greenc_emit_buf_append_span(b, st->tok.start, st->tok.len);
        }
        greenc_emit_buf_append_char(b, '\n');
        return;
    }

    greenc_emit_expr(b, st->expr, 0);
    greenc_emit_buf_append_str(b, ";\n");
}

static void greenc_emit_doc_params(greenc_emit_buf* b, const char* src, int len) {
    /// trim whitespace
    int s = 0, e = len;
    while (s < e && (src[s] == ' ' || src[s] == '\t' || src[s] == '\n' || src[s] == '\r')) s++;
    while (e > s && (src[e-1] == ' ' || src[e-1] == '\t' || src[e-1] == '\n' || src[e-1] == '\r')) e--;

    /// treat empty or bare "void" as no params
    int is_void = (e - s == 4)
        && src[s]=='v' && src[s+1]=='o' && src[s+2]=='i' && src[s+3]=='d';

    if (s >= e || is_void) {
        greenc_emit_buf_append_str(b, " * parameters: none\n");
        return;
    }

    greenc_emit_buf_append_str(b, " * parameters:\n");

    int i = s;
    while (i <= e) {
        /// find next comma or end
        int j = i;
        while (j < e && src[j] != ',') j++;

        /// trim param token edges
        int ps = i, pe = j;
        while (ps < pe && (src[ps] == ' ' || src[ps] == '\t')) ps++;
        while (pe > ps && (src[pe-1] == ' ' || src[pe-1] == '\t')) pe--;

        if (ps < pe) {
            /// find last whitespace or * to split type from name
            int split = pe - 1;
            while (split > ps && src[split] != ' ' && src[split] != '\t' && src[split] != '*') split--;

            if (split > ps) {
                int name_s = split + 1;
                int type_e = split;
                while (type_e > ps && (src[type_e-1] == ' ' || src[type_e-1] == '\t')) type_e--;
                greenc_emit_buf_append_str(b, " *   ");
                greenc_emit_buf_append_span(b, src + name_s, pe - name_s); /// name
                greenc_emit_buf_append_str(b, ": ");
                greenc_emit_buf_append_span(b, src + ps, type_e - ps);     /// type
                greenc_emit_buf_append_str(b, "\n");
            } else {
                greenc_emit_buf_append_str(b, " *   ");
                greenc_emit_buf_append_span(b, src + ps, pe - ps);
                greenc_emit_buf_append_str(b, "\n");
            }
        }

        if (j >= e) break;
        i = j + 1;
    }
}

/// returns 1 if the name span is exactly "main"
static inline int greenc_name_is_main(const char* s, int n) {
    return n == 4 && s[0]=='m' && s[1]=='a' && s[2]=='i' && s[3]=='n';
}

/// emits function name, renaming "main" to "__main" when freestanding
static inline void greenc_emit_func_name(greenc_emit_buf* b, greenc_token name, int freestanding) {
    if (freestanding && greenc_name_is_main(name.start, name.len)) {
        greenc_emit_buf_append_str(b, "__main");
    } else {
        greenc_emit_buf_append_span(b, name.start, name.len);
    }
}

static void greenc_emit_func(greenc_emit_buf* b, greenc_func* gfunc, int freestanding) {
    if (!gfunc || !b->str) {
        return;
    }

    /// resolve return type span
    const char* ret_src = (gfunc->return_type.start && gfunc->return_type.len > 0)
        ? gfunc->return_type.start : "int";
    int ret_len = (gfunc->return_type.start && gfunc->return_type.len > 0)
        ? gfunc->return_type.len : 3;

    /// emit doc comment
    greenc_emit_buf_append_str(b, "/**\n");
    greenc_emit_buf_append_str(b, " * function: ");
    greenc_emit_func_name(b, gfunc->name, freestanding);
    greenc_emit_buf_append_str(b, "\n");
    greenc_emit_buf_append_str(b, " * return_type: ");
    greenc_emit_buf_append_span(b, ret_src, ret_len);
    greenc_emit_buf_append_str(b, "\n");
    greenc_emit_doc_params(b, gfunc->params_src, gfunc->params_src_len);
    greenc_emit_buf_append_str(b, " */\n");

    /// emit signature — empty params use () not (void)
    greenc_emit_buf_append_span(b, ret_src, ret_len);
    greenc_emit_buf_append_str(b, " ");
    greenc_emit_func_name(b, gfunc->name, freestanding);
    greenc_emit_buf_append_str(b, "(");

    if (gfunc->params_src && gfunc->params_src_len > 0) {
        /// check if bare "void"
        const char* p = gfunc->params_src;
        int pl = gfunc->params_src_len;
        int ps = 0, pe = pl;
        while (ps < pe && (p[ps] == ' ' || p[ps] == '\t')) ps++;
        while (pe > ps && (p[pe-1] == ' ' || p[pe-1] == '\t')) pe--;
        int is_void = (pe - ps == 4)
            && p[ps]=='v' && p[ps+1]=='o' && p[ps+2]=='i' && p[ps+3]=='d';
        if (!is_void) {
            greenc_emit_buf_append_span(b, gfunc->params_src, gfunc->params_src_len);
        }
    }

    greenc_emit_buf_append_str(b, ") {\n");

    for (int i = 0; i < gfunc->stmt_count; i++) {
        greenc_emit_stmt(b, gfunc->stmts[i], 4);
    }

    greenc_emit_buf_append_str(b, "}\n");
}


static void greenc_emit_buf_append_int(greenc_emit_buf* b, int n) {
    if (n == 0) { greenc_emit_buf_append_str(b, "0"); return; }
    char tmp[32];
    int i = 0;
    if (n < 0) { greenc_emit_buf_append_str(b, "-"); n = -n; }
    while (n > 0) { tmp[i++] = (char)('0' + n % 10); n /= 10; }
    char rev[32];
    for (int j = 0; j < i; j++) rev[j] = tmp[i - 1 - j];
    greenc_emit_buf_append_span(b, rev, i);
}

static void greenc_emit_forward_decl(greenc_emit_buf* b, greenc_func* gfunc, int freestanding);

static int greenc_include_is_stdio(const char* s, int n) {
    return greenc_str_equals(s, n, "<stdio.h>");
}

static int greenc_include_is_greenc(const char* s, int n) {
    return greenc_str_equals(s, n, "<greenc>")
        || greenc_str_equals(s, n, "<greenc.h>")
        || greenc_str_equals(s, n, "\"greenc.h\"");
}

/// count how many parameters user's main() declares (0 if empty/void)
static int greenc_count_main_params(const char* src, int len) {
    if (!src || len == 0) return 0;
    int s = 0, e = len;
    while (s < e && (src[s]==' ' || src[s]=='\t' || src[s]=='\n')) s++;
    while (e > s && (src[e-1]==' ' || src[e-1]=='\t' || src[e-1]=='\n')) e--;
    if (e <= s) return 0;
    if ((e-s)==4 && src[s]=='v' && src[s+1]=='o' && src[s+2]=='i' && src[s+3]=='d') return 0;
    int commas = 0;
    for (int i = s; i < e; i++) { if (src[i]==',') commas++; }
    return commas + 1;
}

/// emit greenc_main wrapper that calls user __main() with the right number of args
static void greenc_emit_greenc_main_wrapper(greenc_emit_buf* b, greenc_func* main_func) {
    int np = (main_func && main_func->params_src)
        ? greenc_count_main_params(main_func->params_src, main_func->params_src_len)
        : 0;
    int is_void = main_func && main_func->return_type.kind == TOK_VOID;
    greenc_emit_buf_append_str(b, "\n");
    greenc_emit_buf_append_str(b, "/**\n");
    greenc_emit_buf_append_str(b, " * greenc freestanding entry — bridges greenc_start -> user __main\n");
    greenc_emit_buf_append_str(b, " */\n");
    greenc_emit_buf_append_str(b, "int greenc_main(int argc, char** argv, char** envp) {\n");
    if (np == 0) {
        greenc_emit_buf_append_str(b, "    (void)argc; (void)argv; (void)envp;\n");
        if (is_void) {
            greenc_emit_buf_append_str(b, "    __main();\n");
            greenc_emit_buf_append_str(b, "    return 0;\n");
        } else {
            greenc_emit_buf_append_str(b, "    return __main();\n");
        }
    } else if (np == 1) {
        greenc_emit_buf_append_str(b, "    (void)argv; (void)envp;\n");
        if (is_void) {
            greenc_emit_buf_append_str(b, "    __main(argc);\n");
            greenc_emit_buf_append_str(b, "    return 0;\n");
        } else {
            greenc_emit_buf_append_str(b, "    return __main(argc);\n");
        }
    } else if (np == 2) {
        greenc_emit_buf_append_str(b, "    (void)envp;\n");
        if (is_void) {
            greenc_emit_buf_append_str(b, "    __main(argc, argv);\n");
            greenc_emit_buf_append_str(b, "    return 0;\n");
        } else {
            greenc_emit_buf_append_str(b, "    return __main(argc, argv);\n");
        }
    } else {
        if (is_void) {
            greenc_emit_buf_append_str(b, "    __main(argc, argv, envp);\n");
            greenc_emit_buf_append_str(b, "    return 0;\n");
        } else {
            greenc_emit_buf_append_str(b, "    return __main(argc, argv, envp);\n");
        }
    }
    greenc_emit_buf_append_str(b, "}\n");
}

static char* greenc_emit_program(
    greenc_program* pr,
    const char* gcc_source_path,
    const char* output_exe_path,
    const char* base_path,
    const char* gcc_flags,  /// full flags string e.g. "-std=c17 -s" or "-std=c17 -s -static-libgcc ..."
    int freestanding         /// 1 = rename main→greenc_main and emit GREENC_MAIN define
) {
    if (!pr || !gcc_source_path || !output_exe_path) {
        return NULL;
    }

    greenc_emit_buf buf;
    greenc_emit_buf_init(&buf);

    int needs_print = greenc_emit_program_uses_print(pr);

    /// check if source already has a greenc include
    int has_greenc = 0;
    for (int i = 0; i < pr->include_count; i++) {
        if (greenc_include_is_greenc(pr->includes[i].start, pr->includes[i].len)) {
            has_greenc = 1;
            break;
        }
    }
    int has_stdio = 0;
    for (int i = 0; i < pr->include_count; i++) {
        if (greenc_include_is_stdio(pr->includes[i].start, pr->includes[i].len)) {
            has_stdio = 1;
            break;
        }
    }

    /// freestanding (default): auto-include greenc.h when print used and no explicit greenc include
    /// libc mode (-nogc): fall back to stdio.h
    int auto_greenc  = (freestanding  && needs_print && !has_greenc) ? 1 : 0;
    int auto_stdio   = (!freestanding && needs_print && !has_greenc && !has_stdio) ? 1 : 0;
    int include_count = pr->include_count + auto_greenc + auto_stdio;
    int define_count  = 0;
    int func_count    = pr->func_count;



    /// find entry point
    greenc_token entry = {TOK_EOF, NULL, 0};
    for (int i = 0; i < pr->func_count; i++) {
        if (greenc_str_equals(pr->funcs[i]->name.start, pr->funcs[i]->name.len, "main")) {
            entry = pr->funcs[i]->name;
            break;
        }
    }

    /// gcc hint comment — multi-line block
    greenc_emit_buf_append_str(&buf, "/*\n");
    greenc_emit_buf_append_str(&buf, "gcc -o ");
    greenc_emit_buf_append_str(&buf, output_exe_path);
    greenc_emit_buf_append_str(&buf, " ");
    greenc_emit_buf_append_str(&buf, gcc_source_path);
    greenc_emit_buf_append_str(&buf, " ");
    greenc_emit_buf_append_str(&buf, gcc_flags ? gcc_flags : "-std=c17 -s");
    greenc_emit_buf_append_str(&buf, "\n*/\n");
    greenc_emit_buf_append_str(&buf, "\n");

    /// table of contents
    greenc_emit_buf_append_str(&buf, "/**\n");
    greenc_emit_buf_append_str(&buf, " * Table of Contents:\n");
    greenc_emit_buf_append_str(&buf, " *  Includes: ");    greenc_emit_buf_append_int(&buf, include_count); greenc_emit_buf_append_str(&buf, "\n");
    greenc_emit_buf_append_str(&buf, " *  Definitions: ");  greenc_emit_buf_append_int(&buf, define_count);  greenc_emit_buf_append_str(&buf, "\n");
    greenc_emit_buf_append_str(&buf, " *  Declarations: 0\n");
    greenc_emit_buf_append_str(&buf, " *  Functions: ");    greenc_emit_buf_append_int(&buf, func_count);    greenc_emit_buf_append_str(&buf, "\n");
    greenc_emit_buf_append_str(&buf, " *  Start/Entry: ");
    if (entry.start) {
        greenc_emit_buf_append_span(&buf, entry.start, entry.len);
    } else {
        greenc_emit_buf_append_str(&buf, "none");
    }
    greenc_emit_buf_append_str(&buf, "\n");
    greenc_emit_buf_append_str(&buf, " *  Warnings: 0\n");
    greenc_emit_buf_append_str(&buf, " */\n");
    greenc_emit_buf_append_str(&buf, "\n");

    /// includes section
    greenc_emit_buf_append_str(&buf, "/**\n * All Includes first\n */\n");
    for (int i = 0; i < pr->include_count; i++) {
        const char* s = pr->includes[i].start;
        int n = pr->includes[i].len;
        if (greenc_include_is_greenc(s, n)) {
            /// resolve greenc include to full path
            greenc_emit_buf_append_str(&buf, "#include \"");
            if (base_path) {
                greenc_emit_buf_append_str(&buf, base_path);
                greenc_emit_buf_append_str(&buf, "/compiler/libs/greenc.h");
            } else {
                greenc_emit_buf_append_str(&buf, "greenc.h"); /// fallback
            }
            greenc_emit_buf_append_str(&buf, "\"\n");
        } else {
            greenc_emit_buf_append_str(&buf, "#include ");
            greenc_emit_buf_append_span(&buf, s, n);
            greenc_emit_buf_append_str(&buf, "\n");
        }
    }
    if (auto_greenc) {
        greenc_emit_buf_append_str(&buf, "#include \"");
        if (base_path) {
            greenc_emit_buf_append_str(&buf, base_path);
            greenc_emit_buf_append_str(&buf, "/compiler/libs/greenc.h");
        } else {
            greenc_emit_buf_append_str(&buf, "greenc.h"); /// fallback
        }
        greenc_emit_buf_append_str(&buf, "\"\n");
    }
    if (auto_stdio) {
        greenc_emit_buf_append_str(&buf, "#include <stdio.h>\n");
    }
    greenc_emit_buf_append_str(&buf, "\n");

    /// definitions section
    greenc_emit_buf_append_str(&buf, "/**\n * Definitions\n */\n");
    greenc_emit_buf_append_str(&buf, "\n");


    /// declarations section: type declarations only (no forward funcs counted)
    /// === Type Declarations ===
    /// (none — omitted when no custom types)

    /// === Forward Declarations === (auto-generated, not counted in ToC)
    greenc_emit_buf_append_str(&buf, "/**\n * Declarations\n */\n");
    greenc_emit_buf_append_str(&buf, "\n");
    greenc_emit_buf_append_str(&buf, "/* === Forward Declarations === */\n");
    for (int i = 0; i < pr->func_count; i++) {
        greenc_emit_forward_decl(&buf, pr->funcs[i], freestanding);
    }
    greenc_emit_buf_append_str(&buf, "\n");


    /// functions
    for (int i = 0; i < pr->func_count; i++) {
        if (i > 0) {
            greenc_emit_buf_append_str(&buf, "\n");
        }
        greenc_emit_func(&buf, pr->funcs[i], freestanding);
    }

    /// freestanding: emit greenc_main wrapper at the end calling user's main
    if (freestanding) {
        greenc_func* main_func = NULL;
        for (int i = 0; i < pr->func_count; i++) {
            if (greenc_name_is_main(pr->funcs[i]->name.start, pr->funcs[i]->name.len)) {
                main_func = pr->funcs[i];
                break;
            }
        }
        greenc_emit_greenc_main_wrapper(&buf, main_func);
    }

    if (!buf.str) {
        return NULL;
    }

    return buf.str;
}

static void greenc_emit_forward_decl(greenc_emit_buf* b, greenc_func* gfunc, int freestanding) {
    /// return type
    if (gfunc->return_type.start && gfunc->return_type.len > 0) {
        greenc_emit_buf_append_span(b, gfunc->return_type.start, gfunc->return_type.len);
    } else {
        greenc_emit_buf_append_str(b, "int");
    }
    greenc_emit_buf_append_str(b, " ");
    greenc_emit_func_name(b, gfunc->name, freestanding);
    greenc_emit_buf_append_str(b, "(");
    if (gfunc->params_src && gfunc->params_src_len > 0) {
        const char* p = gfunc->params_src;
        int pl = gfunc->params_src_len;
        int ps = 0, pe = pl;
        while (ps < pe && (p[ps] == ' ' || p[ps] == '\t')) ps++;
        while (pe > ps && (p[pe-1] == ' ' || p[pe-1] == '\t')) pe--;
        int is_void = (pe - ps == 4)
            && p[ps]=='v' && p[ps+1]=='o' && p[ps+2]=='i' && p[ps+3]=='d';
        if (!is_void) {
            greenc_emit_buf_append_span(b, gfunc->params_src, gfunc->params_src_len);
        }
    }
    greenc_emit_buf_append_str(b, ");\n");
}
