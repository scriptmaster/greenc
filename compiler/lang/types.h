#pragma once
#include "../libs/libgreenc.h"

typedef enum {
    GREENC_FILE_C,
    GREENC_FILE_H,
    GREENC_FILE_GC,
    GREENC_FILE_OTHER
} greenc_file_kind;

typedef struct {
    string_list c_files;
    string_list h_files;
    string_list gc_files;
} greenc_build_graph;


//
// LEXER
//
typedef enum {
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_SEMI, TOK_EQ,
    TOK_FN, TOK_LET, TOK_RETURN,
    TOK_ERROR,   // maps to return
    // C types
    TOK_INT, TOK_FLOAT, TOK_BOOL,
    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    // Markers: ///move
    TOK_MARKER_COMMENT,
    // Custom tokens Start:

    // Custom tokens End.
    // last set of tokens
    TOK_UNKNOWN, TOK_EOF
} greenc_tok_kind;

typedef struct {
    greenc_tok_kind kind;
    const char* start;
    int len;
} greenc_token;

typedef struct {
    const char* src;
    int pos;
} greenc_lexer;

typedef struct {
    greenc_dict map;  // string → token kind
} greenc_keywords;


// checker.h markers
typedef enum {
    GREENC_MARK_NONE,
    GREENC_MARK_MOVE,
    GREENC_MARK_READ,
    GREENC_MARK_STACK
} greenc_mark_kind;

typedef struct {
    greenc_mark_kind kind;
    const char* free_fn;
    int free_fn_len;
} greenc_marker;

typedef struct {
    const char* name;
    int len;

    greenc_mark_kind mark;
    int freed;
} greenc_var;
