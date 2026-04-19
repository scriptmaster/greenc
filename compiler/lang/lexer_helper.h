#pragma once
#include "types.h"

// --------------------------------------------------
// helpers
// --------------------------------------------------
///peek:
static inline char peek(greenc_lexer* l) {
    return l->src[l->pos];
}

///peek_i:
static inline char peek_i(greenc_lexer* l /* read */, int i)
/* read l: typeof(l)=='greenc_lexer' && l != NULL */
{
    return l->src[l->pos + i];
}

///peek_compare:
static inline int peek_compare(
    greenc_lexer* l,
    int len,
    const char* s
) {
    return greenc_str_equals(l->src + l->pos, len, s);
}

///next:
static inline char next(greenc_lexer* l) {
    return l->src[l->pos++];
}

///skip_ws:
static inline void skip_ws(greenc_lexer* l) {
    while (
        peek(l) == ' '  ||
        peek(l) == '\n' ||
        peek(l) == '\t' ||
        peek(l) == '\r'
    ) {
        next(l);
    }
}

// --------------------------------------------------
// identifier lexing
// --------------------------------------------------

static inline greenc_token lex_ident(greenc_lexer* l, int start) {

    while (
        (peek(l) >= 'a' && peek(l) <= 'z') ||
        (peek(l) >= 'A' && peek(l) <= 'Z') ||
        (peek(l) >= '0' && peek(l) <= '9') ||
        (peek(l) == '_')
    ) {
        next(l);
    }

    return (greenc_token){
        TOK_IDENT,
        l->src + start,
        l->pos - start
    };
}


///lex_number=
static inline greenc_token lex_number(greenc_lexer* l, int start) {

    while (peek(l) >= '0' && peek(l) <= '9') {
        next(l);
    }

    // optional: float
    if (peek(l) == '.') {
        next(l);
        while (peek(l) >= '0' && peek(l) <= '9') {
            next(l);
        }
    }

    return (greenc_token){
        TOK_NUMBER,
        l->src + start,
        l->pos - start
    };
}


// SEARCH: lex_string(
static inline greenc_token lex_string(greenc_lexer* l, int start) {

    // skip opening quote
    // already consumed '"'

    while (peek(l) != '"' && peek(l) != 0) {
        next(l);
    }

    // now at closing quote OR EOF
    int end = l->pos;

    if (peek(l) == '"') {
        next(l); // consume closing quote
    }

    // exclude quotes in token (important)
    return (greenc_token){
        TOK_STRING,
        l->src + start + 1,
        end - (start + 1)
    };
}



// --------------------------------------------------
// MARKER: ///move
// --------------------------------------------------

// SEARCH: lex_marker_slash
static inline greenc_token lex_marker_slash(greenc_lexer* l, int start) {

    // we already saw ///
    while (peek(l) == ' ') next(l);

    int mstart = l->pos;

    while (
        peek(l) != '\n' &&
        peek(l) != 0
    ) {
        next(l);
    }

    return (greenc_token){
        TOK_MARKER_COMMENT,
        l->src + mstart,
        l->pos - mstart
    };
}

// --------------------------------------------------
// MARKER: /*///move*/
// --------------------------------------------------

// SEARCH: lex_marker_block
static inline greenc_token lex_marker_block(greenc_lexer* l, int start) {

    // already consumed /*///
    int mstart = l->pos;

    while (peek(l) != 0) {

        if (peek(l) == '*' && l->src[l->pos+1] == '/') {
            int mend = l->pos;
            next(l); next(l); // consume */
            return (greenc_token){
                TOK_MARKER_COMMENT,
                l->src + mstart,
                mend - mstart
            };
        }

        next(l);
    }

    return (greenc_token){TOK_EOF, "", 0};
}
