#pragma once

#include "types.h"
#include "lexer.h"
#include "../libs/standard.h"

// --------------------------------------------------
// parser struct
// --------------------------------------------------

typedef struct {
    greenc_lexer* lexer;
    greenc_keywords* keywords;
    greenc_token tok;
} greenc_parser;

// --------------------------------------------------
// helpers
// --------------------------------------------------

// SEARCH: parser_next(
static inline void parser_next(greenc_parser* p) {
    p->tok = greenc_next_token(p->lexer, p->keywords);
}

// SEARCH: parser_expect(
static inline void parser_expect(greenc_parser* p, greenc_tok_kind kind) {
    if (p->tok.kind != kind) {
        print("parse error: unexpected token");
        return;
    }
    parser_next(p);
}

// --------------------------------------------------
// debug token print (useful now)
// --------------------------------------------------

// SEARCH: parser_debug_token(
static inline void parser_debug_token(greenc_token tok) {
    print(tok.start);

    sys_write("\tToken=", 6);
    sys_write(tok.start, tok.len);
    sys_write("\n", 1);

    char* len_str = greenc_itoa(tok.len);
    sys_write("\t\tlen=", 7);
    sys_write(len_str, greenc_strlen(len_str));

    greenc_free(len_str);
}

// --------------------------------------------------
// parse return / error
// --------------------------------------------------

// SEARCH: parse_return(
static void parse_return(greenc_parser* p) {

    // skip return / error
    parser_next(p);

    if (p->tok.kind != TOK_NUMBER) {
        print("parse error: expected number");
        return;
    }

    parser_debug_token(p->tok);

    parser_next(p);

    parser_expect(p, TOK_SEMI);
}

// --------------------------------------------------
// parse function
// --------------------------------------------------

// SEARCH: parse_function(
static void parse_function(greenc_parser* p) {

    // accept: int OR fn
    if (p->tok.kind == TOK_INT || p->tok.kind == TOK_FN) {
        parser_next(p);
    } else {
        print("parse error: expected function keyword");
        return;
    }

    // function name
    if (p->tok.kind != TOK_IDENT) {
        print("parse error: expected function name");
        return;
    }

    print("function: ");
    parser_debug_token(p->tok);

    parser_next(p);

    parser_expect(p, TOK_LPAREN);
    parser_expect(p, TOK_RPAREN);

    parser_expect(p, TOK_LBRACE);

    // body
    while (p->tok.kind != TOK_RBRACE &&
           p->tok.kind != TOK_EOF) {

        if (p->tok.kind == TOK_RETURN) {
            parse_return(p);
        } else {
            // skip unknown tokens for now
            parser_next(p);
        }
    }

    parser_expect(p, TOK_RBRACE);
}

// --------------------------------------------------
// parser entry
// --------------------------------------------------

// SEARCH: parse_file(
static void parse_file(
    const char* source,
    greenc_keywords* keywords
) {
    greenc_lexer lexer = { source, 0 };

    greenc_parser p;
    p.lexer = &lexer;
    p.keywords = keywords;

    parser_next(&p);

    while (p.tok.kind != TOK_EOF) {

        // only function parsing for now
        if (p.tok.kind == TOK_INT || p.tok.kind == TOK_FN) {
            parse_function(&p);
        } else {
            parser_next(&p);
        }
    }
}
