#include "../builtin/strings.h"

// simple join path
static inline char* gc_path_join(const char* a, const char* b) {
    return greenc_concat(a, "/", b);
}
