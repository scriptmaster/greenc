#pragma once
#include "types.h"

// @public
static void greenc_keywords_init(greenc_keywords* k) {
    greenc_dict_init(&k->map);

    greenc_dict_put(&k->map, "fn",     (void*)TOK_FN);
    greenc_dict_put(&k->map, "let",    (void*)TOK_LET);
    greenc_dict_put(&k->map, "return", (void*)TOK_RETURN);
    greenc_dict_put(&k->map, "int",    (void*)TOK_INT);
}
