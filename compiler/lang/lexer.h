#pragma once
#include "types.h"
#include "lexer_helper.h"

// --------------------------------------------------
// keyword resolver (NO allocation)
// --------------------------------------------------

// ANCHOR: greenc_resolve_ident
static inline greenc_token greenc_resolve_ident(
    greenc_keywords* k,
    greenc_token tok
) {
    if (tok.kind != TOK_IDENT) return tok;

    void* v = greenc_dict_get_len(&k->map, tok.start, tok.len);

    if (v) {
        tok.kind = (greenc_tok_kind)(long long)v;
    }

    return tok;
}

// --------------------------------------------------
// main lexer entry
// --------------------------------------------------

// SEARCH: greenc_next_token(
static inline greenc_token greenc_next_token(
    greenc_lexer* l,
    greenc_keywords* k
) {
    skip_ws(l);

    int start = l->pos;
    char c = next(l);


    // 1. string
    if (c == '"') return lex_string(l, start);

    // 2. number
    if (c >= '0' && c <= '9') return lex_number(l, start);

    // 3. identifier or keyword
    if (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_'
    ) {
        greenc_token tok = lex_ident(l, start);
        return greenc_resolve_ident(k, tok);
    }

    // 4. # directives
    if (c == '#') {
        while (peek(l) == ' ' || peek(l) == '\t') next(l); /// skip spaces after #
        if (greenc_str_equals(l->src + l->pos, 7, "include")) {
            char after = l->src[l->pos + 7];
            if (after == ' ' || after == '\t' || after == '<' || after == '"') {
                l->pos += 7;
                while (peek(l) == ' ' || peek(l) == '\t') next(l); /// skip spaces before path
                int path_start = l->pos;
                char open = l->src[l->pos];
                if (open == '<' || open == '"') {
                    char close = (open == '<') ? '>' : '"';
                    next(l); /// consume opening delimiter
                    while (peek(l) != close && peek(l) != 0 && peek(l) != '\n') next(l);
                    if (peek(l) == close) next(l); /// consume closing delimiter
                }
                return (greenc_token){TOK_HASH_INCLUDE, l->src + path_start, l->pos - path_start};
            }
        }
        while (peek(l) != '\n' && peek(l) != 0) next(l); /// skip unknown # directive
        return greenc_next_token(l, k);
    }

    // 5. slashes
    if (c == '/') {
        // ///marker
        if (peek(l) == '/' && l->src[l->pos+1] == '/') {
            next(l); next(l);
            return lex_marker_slash(l, start);
        }

        // /* block comment */
        if (peek(l) == '*') {
            next(l); /// consume *
            while (peek(l) != 0) {
                if (peek(l) == '*' && l->src[l->pos+1] == '/') {
                    next(l); next(l); /// consume */
                    break;
                }
                next(l);
            }
            return (greenc_token){TOK_BLOCK_COMMENT, l->src + start, l->pos - start};
        }

        // // normal comment
        if (peek(l) == '/') {
            while (peek(l) != '\n' && peek(l) != 0) next(l);
            return (greenc_token){TOK_LINE_COMMENT, l->src + start, l->pos - start};
        }

        return (greenc_token){TOK_SLASH, "/", 1};
    }

    // block comment or marker
    if (c == '*') {
        // not used standalone
    }

    // 5. operators + symbols
    switch (c) {
        case 0:  return (greenc_token){TOK_EOF, "", 0};

        case '(': return (greenc_token){TOK_LPAREN, "(", 1};
        case ')': return (greenc_token){TOK_RPAREN, ")", 1};

        case '{': return (greenc_token){TOK_LBRACE, "{", 1};
        case '}': return (greenc_token){TOK_RBRACE, "}", 1};

        case ',': return (greenc_token){TOK_COMMA, ",", 1};

        case ';': return (greenc_token){TOK_SEMI, ";", 1};
        case '=': return (greenc_token){TOK_EQ, "=", 1};

        case '+': return (greenc_token){TOK_PLUS,  "+", 1};
        case '-': return (greenc_token){TOK_MINUS, "-", 1};
        case '*': return (greenc_token){TOK_STAR,  "*", 1};
        case '%': return (greenc_token){TOK_PERCENT, "%", 1};
    }

    return (greenc_token){TOK_UNKNOWN, l->src + start, 1};
    // fallback
    // return (greenc_token){TOK_EOF, "", 0};
}
