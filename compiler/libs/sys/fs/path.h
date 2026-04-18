#pragma once

#include "../builtin/strings.h"

#ifdef _WIN32
    #define GC_PATH_SEP '\\'
    #define GC_PATH_SEP_STR "\\"
#else
    #define GC_PATH_SEP '/'
    #define GC_PATH_SEP_STR "/"
#endif

static inline char* gc_path_join(const char* a, const char* b) {
    return greenc_concat(a, GC_PATH_SEP_STR, b);
}
