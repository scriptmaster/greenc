#pragma once

#include "types.h"
#include "ast_nodes.h"
#include "lexer.h"
#include "../libs/standard.h"

typedef struct {
    greenc_lexer* lexer;
    greenc_keywords* keywords;
    greenc_token tok;
    int failed;
} greenc_parser;

static inline void parser_next(greenc_parser* p) {
    p->tok = greenc_next_token(p->lexer, p->keywords);
}

static inline void parser_expect(greenc_parser* p, greenc_tok_kind kind) {
    if (p->tok.kind != kind) {
        print("parse error: unexpected token");
        p->failed = 1;
        return;
    }
    parser_next(p);
}

static greenc_expr* greenc_expr_new(greenc_expr_kind k) {
    greenc_expr* e = (greenc_expr*)greenc_malloc(sizeof(greenc_expr));
    if (!e) return NULL;
    e->kind = k;
    e->tok = (greenc_token){TOK_EOF, "", 0};
    e->inner = NULL;
    e->callee = NULL;
    e->args = NULL;
    e->arg_count = 0;
    return e;
}

static greenc_stmt* greenc_stmt_new(greenc_stmt_kind k, greenc_expr* ex) {
    greenc_stmt* s = (greenc_stmt*)greenc_malloc(sizeof(greenc_stmt));
    if (!s) return NULL;
    s->kind = k;
    s->expr = ex;
    s->tok = (greenc_token){TOK_EOF, "", 0};
    return s;
}

static greenc_program* greenc_program_new(void) {
    greenc_program* pr = (greenc_program*)greenc_malloc(sizeof(greenc_program));
    if (!pr) return NULL;
    pr->funcs = NULL;
    pr->func_count = 0;
    pr->func_cap = 0;
    pr->includes = NULL;
    pr->include_count = 0;
    pr->include_cap = 0;
    return pr;
}

static void greenc_program_push_include(greenc_program* pr, greenc_token tok) {
    if (pr->include_count >= pr->include_cap) {
        int ncap = pr->include_cap ? pr->include_cap * 2 : 4;
        greenc_token* ni = (greenc_token*)greenc_malloc(sizeof(greenc_token) * (size_t)ncap);
        if (!ni) return;
        for (int i = 0; i < pr->include_count; i++) ni[i] = pr->includes[i];
        greenc_free(pr->includes);
        pr->includes = ni;
        pr->include_cap = ncap;
    }
    pr->includes[pr->include_count++] = tok;
}



static void greenc_func_push_stmt(greenc_func* gfunc, greenc_stmt* st) {
    if (gfunc->stmt_count >= gfunc->stmt_cap) {
        int ncap = gfunc->stmt_cap ? gfunc->stmt_cap * 2 : 8;
        greenc_stmt** ns =
            (greenc_stmt**)greenc_malloc(sizeof(greenc_stmt*) * (size_t)ncap);
        if (!ns) return;
        for (int i = 0; i < gfunc->stmt_count; i++) {
            ns[i] = gfunc->stmts[i];
        }
        gfunc->stmts = ns;
        gfunc->stmt_cap = ncap;
    }
    gfunc->stmts[gfunc->stmt_count++] = st;
}

static void greenc_program_push_func(greenc_program* pr, greenc_func* gfunc) {
    if (pr->func_count >= pr->func_cap) {
        int ncap = pr->func_cap ? pr->func_cap * 2 : 8;
        greenc_func** nf =
            (greenc_func**)greenc_malloc(sizeof(greenc_func*) * (size_t)ncap);
        if (!nf) return;
        for (int i = 0; i < pr->func_count; i++) {
            nf[i] = pr->funcs[i];
        }
        pr->funcs = nf;
        pr->func_cap = ncap;
    }
    pr->funcs[pr->func_count++] = gfunc;
}

static void greenc_expr_args_push(
    greenc_expr*** arr,
    int* count,
    int* cap,
    greenc_expr* e
) {
    if (*count >= *cap) {
        int ncap = *cap ? *cap * 2 : 4;
        greenc_expr** na =
            (greenc_expr**)greenc_malloc(sizeof(greenc_expr*) * (size_t)ncap);
        if (!na) return;
        for (int i = 0; i < *count; i++) {
            na[i] = (*arr)[i];
        }
        *arr = na;
        *cap = ncap;
    }
    (*arr)[(*count)++] = e;
}

static greenc_expr* parse_expr(greenc_parser* p);
static greenc_expr* parse_unary(greenc_parser* p);

static int parse_is_binop(greenc_tok_kind k) {
    return k == TOK_PLUS || k == TOK_MINUS || k == TOK_STAR || k == TOK_SLASH || k == TOK_PERCENT;
}

static int parse_binop_prec(greenc_tok_kind k) {
    if (k == TOK_STAR || k == TOK_SLASH || k == TOK_PERCENT) return 2;
    if (k == TOK_PLUS || k == TOK_MINUS) return 1;
    return 0;
}

static greenc_expr* parse_binop_rhs(greenc_parser* p, greenc_expr* lhs, int min_prec) {
    while (parse_is_binop(p->tok.kind) && parse_binop_prec(p->tok.kind) >= min_prec) {
        greenc_token op = p->tok;
        int prec = parse_binop_prec(op.kind);
        parser_next(p);
        greenc_expr* rhs = parse_unary(p);
        if (!rhs || p->failed) return lhs;
        while (parse_is_binop(p->tok.kind) && parse_binop_prec(p->tok.kind) > prec) {
            rhs = parse_binop_rhs(p, rhs, prec + 1);
            if (!rhs || p->failed) return lhs;
        }
        greenc_expr* bin = greenc_expr_new(GREENC_EXPR_BINOP);
        if (!bin) return lhs;
        bin->tok = op;    /// operator token
        bin->inner = lhs; /// left side
        bin->callee = rhs; /// right side
        lhs = bin;
    }
    return lhs;
}

static greenc_expr* parse_expr(greenc_parser* p) {
    greenc_expr* lhs = parse_unary(p);
    if (!lhs || p->failed) return lhs;
    return parse_binop_rhs(p, lhs, 1);
}

static greenc_expr* parse_primary(greenc_parser* p) {
    if (p->failed) return NULL;

    if (p->tok.kind == TOK_NUMBER) {
        greenc_expr* e = greenc_expr_new(GREENC_EXPR_INT);
        if (!e) return NULL;
        e->tok = p->tok;
        parser_next(p);
        return e;
    }

    if (p->tok.kind == TOK_STRING) {
        greenc_expr* e = greenc_expr_new(GREENC_EXPR_STRING);
        if (!e) return NULL;
        e->tok = p->tok;
        parser_next(p);
        return e;
    }

    if (p->tok.kind == TOK_IDENT) {
        greenc_expr* e = greenc_expr_new(GREENC_EXPR_IDENT);
        if (!e) return NULL;
        e->tok = p->tok;
        parser_next(p);
        return e;
    }

    if (p->tok.kind == TOK_LPAREN) {
        parser_next(p);
        greenc_expr* inner = parse_expr(p);
        parser_expect(p, TOK_RPAREN);
        return inner;
    }

    print("parse error: expected expression");
    p->failed = 1;
    return NULL;
}

static greenc_expr* parse_postfix(greenc_parser* p) {
    greenc_expr* e = parse_primary(p);
    if (!e || p->failed) return e;

    while (p->tok.kind == TOK_LPAREN) {
        parser_next(p);
        greenc_expr** args = NULL;
        int n = 0;
        int cap = 0;

        if (p->tok.kind != TOK_RPAREN) {
            while (1) {
                greenc_expr* a = parse_expr(p);
                if (!a) {
                    p->failed = 1;
                    return e;
                }
                greenc_expr_args_push(&args, &n, &cap, a);
                if (p->tok.kind == TOK_COMMA) {
                    parser_next(p);
                    continue;
                }
                break;
            }
        }

        parser_expect(p, TOK_RPAREN);

        greenc_expr* call = greenc_expr_new(GREENC_EXPR_CALL);
        if (!call) {
            p->failed = 1;
            return e;
        }
        call->callee = e;
        call->args = args;
        call->arg_count = n;
        e = call;
    }

    return e;
}

static greenc_expr* parse_unary(greenc_parser* p) {
    if (p->tok.kind == TOK_MINUS) {
        parser_next(p);
        greenc_expr* inner = parse_unary(p);
        if (!inner) return NULL;
        greenc_expr* e = greenc_expr_new(GREENC_EXPR_NEG);
        if (!e) return NULL;
        e->inner = inner;
        return e;
    }
    return parse_postfix(p);
}

static void parse_stmt(greenc_parser* p, greenc_func* gfunc) {
    if (p->failed) return;

    /// handle comment tokens — emit as-is, no parse error
    if (
        p->tok.kind == TOK_LINE_COMMENT ||
        p->tok.kind == TOK_BLOCK_COMMENT ||
        p->tok.kind == TOK_MARKER_COMMENT
    ) {
        greenc_stmt* st = greenc_stmt_new(GREENC_STMT_COMMENT, NULL);
        if (st) {
            st->tok = p->tok;
            greenc_func_push_stmt(gfunc, st);
        }
        parser_next(p);
        return;
    }

    if (p->tok.kind == TOK_RETURN) {
        parser_next(p);
        greenc_expr* ex = parse_expr(p);
        if (!ex) {
            p->failed = 1;
            return;
        }
        parser_expect(p, TOK_SEMI);
        greenc_stmt* st = greenc_stmt_new(GREENC_STMT_RETURN, ex);
        if (st) greenc_func_push_stmt(gfunc, st);
        return;
    }

    if (p->tok.kind == TOK_IDENT) {
        greenc_token ident = p->tok;
        parser_next(p);

        if (p->tok.kind == TOK_LPAREN) {
            parser_next(p);
            greenc_expr** args = NULL;
            int n = 0;
            int cap = 0;

            if (p->tok.kind != TOK_RPAREN) {
                while (1) {
                    greenc_expr* a = parse_expr(p);
                    if (!a) {
                        p->failed = 1;
                        return;
                    }
                    greenc_expr_args_push(&args, &n, &cap, a);
                    if (p->tok.kind == TOK_COMMA) {
                        parser_next(p);
                        continue;
                    }
                    break;
                }
            }

            parser_expect(p, TOK_RPAREN);
            greenc_expr* callee = greenc_expr_new(GREENC_EXPR_IDENT);
            if (!callee) {
                p->failed = 1;
                return;
            }
            callee->tok = ident;
            greenc_expr* call = greenc_expr_new(GREENC_EXPR_CALL);
            if (!call) {
                p->failed = 1;
                return;
            }
            call->callee = callee;
            call->args = args;
            call->arg_count = n;
            parser_expect(p, TOK_SEMI);
            greenc_stmt* st = greenc_stmt_new(GREENC_STMT_EXPR, call);
            if (st) greenc_func_push_stmt(gfunc, st);
            return;
        }

        p->tok = ident;
    }

    greenc_expr* ex = parse_expr(p);
    if (!ex) {
        p->failed = 1;
        return;
    }
    parser_expect(p, TOK_SEMI);
    greenc_stmt* st = greenc_stmt_new(GREENC_STMT_EXPR, ex);
    if (st) greenc_func_push_stmt(gfunc, st);
}

static greenc_func* parse_function(greenc_parser* p) {
    if (p->tok.kind != TOK_VOID && p->tok.kind != TOK_INT && p->tok.kind != TOK_FN) {
        print("parse error: expected return type or fn");
        p->failed = 1;
        return NULL;
    }

    greenc_token return_type = p->tok; /// capture return type before advancing

    parser_next(p); /// now pointing at function name

    if (p->tok.kind != TOK_IDENT) {
        print("parse error: expected function name");
        p->failed = 1;
        return NULL;
    }

    greenc_token name = p->tok; /// capture function name


    greenc_func* gfunc = (greenc_func*)greenc_malloc(sizeof(greenc_func));
    if (!gfunc) return NULL;
    gfunc->name = name;
    gfunc->return_type = return_type;
    gfunc->params_src = NULL;
    gfunc->params_src_len = 0;
    gfunc->stmts = NULL;
    gfunc->stmt_count = 0;
    gfunc->stmt_cap = 0;

    parser_next(p);

    parser_expect(p, TOK_LPAREN);
    gfunc->params_src = p->tok.start; /// start of first token inside (
    const char* params_end = p->tok.start;
    while (p->tok.kind != TOK_RPAREN && p->tok.kind != TOK_EOF) {
        params_end = p->tok.start + p->tok.len;
        parser_next(p);
    }
    gfunc->params_src_len = (int)(params_end - gfunc->params_src);
    parser_expect(p, TOK_RPAREN);

    parser_expect(p, TOK_LBRACE);

    while (p->tok.kind != TOK_RBRACE && p->tok.kind != TOK_EOF) {
        parse_stmt(p, gfunc);
        if (p->failed) break;
    }

    parser_expect(p, TOK_RBRACE);
    return gfunc;
}

static greenc_program* greenc_parse_file(
    const char* source,
    greenc_keywords* keywords
) {

    greenc_lexer lexer = {source, 0};

    greenc_parser p;
    p.lexer = &lexer;
    p.keywords = keywords;
    p.failed = 0;

    parser_next(&p);

    greenc_program* program = greenc_program_new();
    if (!program) return NULL;

    while (p.tok.kind != TOK_EOF) {
        if (p.tok.kind == TOK_HASH_INCLUDE) {
            greenc_program_push_include(program, p.tok);
            parser_next(&p);
            continue;
        }

        /// skip top-level comments gracefully
        if (
            p.tok.kind == TOK_LINE_COMMENT ||
            p.tok.kind == TOK_BLOCK_COMMENT ||
            p.tok.kind == TOK_MARKER_COMMENT
        ) {
            parser_next(&p);
            continue;
        }

        if (p.tok.kind == TOK_VOID || p.tok.kind == TOK_INT || p.tok.kind == TOK_FN) {
            greenc_func* gfunc = parse_function(&p);
            if (gfunc && !p.failed) {
                greenc_program_push_func(program, gfunc);
                continue;
            }

            break;
        }

        parser_next(&p);
    }

    if (p.failed) {
        return NULL;
    }

    return program;
}

static void greenc_program_merge(greenc_program* dst, greenc_program* src) {
    if (!src) {
        return;
    }

    for (int i = 0; i < src->include_count; i++) {
        greenc_program_push_include(dst, src->includes[i]);
    }

    for (int i = 0; i < src->func_count; i++) {
        greenc_program_push_func(dst, src->funcs[i]);
    }

    greenc_free(src->includes);
    greenc_free(src->funcs);
    greenc_free(src);
}
