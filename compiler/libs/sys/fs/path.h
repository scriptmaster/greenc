#pragma once

#include "../builtin/strings.h"

#ifdef _WIN32
    #define GREENC_PATH_SEP '\\'
    #define GREENC_PATH_SEP_STR "\\"
#else
    #define GREENC_PATH_SEP '/'
    #define GREENC_PATH_SEP_STR "/"
#endif

static inline char* greenc_path_join(const char* a, const char* b) {
    return greenc_concat(a, GREENC_PATH_SEP_STR, b);
}
