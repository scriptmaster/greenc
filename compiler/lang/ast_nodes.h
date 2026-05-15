#pragma once

#include "types.h"

typedef enum {
    GREENC_EXPR_INT,
    GREENC_EXPR_STRING,
    GREENC_EXPR_IDENT,
    GREENC_EXPR_CALL,
    GREENC_EXPR_NEG
} greenc_expr_kind;

typedef struct greenc_expr greenc_expr;

struct greenc_expr {
    greenc_expr_kind kind;
    greenc_token tok;
    greenc_expr* inner;
    greenc_expr* callee;
    greenc_expr** args;
    int arg_count;
};

typedef enum {
    GREENC_STMT_EXPR,
    GREENC_STMT_RETURN
} greenc_stmt_kind;

typedef struct greenc_stmt greenc_stmt;

struct greenc_stmt {
    greenc_stmt_kind kind;
    greenc_expr* expr;
};

typedef struct {
    greenc_token name;
    greenc_stmt** stmts;
    int stmt_count;
    int stmt_cap;
} greenc_func;

typedef struct {
    greenc_func** funcs;
    int func_count;
    int func_cap;
} greenc_program;
