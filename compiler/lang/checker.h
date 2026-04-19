#pragma once

#include "types.h"
#include "lexer.h"
#include "../libs/standard.h"

// SEARCH: marker_kind_from_text(
static greenc_mark_kind marker_kind_from_text(const char* s, int len) {

    if (greenc_str_equals(s, len, "move"))  return GREENC_MARK_MOVE;
    if (greenc_str_equals(s, len, "read"))  return GREENC_MARK_READ;
    if (greenc_str_equals(s, len, "stack")) return GREENC_MARK_STACK;

    return GREENC_MARK_NONE;
}
