#pragma once
#include "types_intrinsic.h"
#include "stdarg.h"
#include "alloc.h"

/** Declarations */
static inline size_t greenc_strlen(const char* s);

static inline void* greenc_memcpy(void* dest, const void* src, size_t n); /// defer free
// static inline void* greenc_copy_mem(const void* src, size_t srclen); /// defer free
static inline char* greenc_copy_chars(const char* src); /// defer free
static inline char* greenc_concat_counted(int count, ...); /// defer free

static inline int   greenc_string_startswith(const char* s, const char* prefix);
static inline int   greenc_string_endswith(const char* s, const char* suffix);

/** Definitions */
#define greenc_copy greenc_copy_chars

/* Macro to automatically count arguments (up to 10) */
#define _GREENC_COUNT(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10, N, ...) N
#define greenc_concat(...) greenc_concat_counted(_GREENC_COUNT(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1), __VA_ARGS__)

static inline size_t greenc_strlen(const char* s) {
    if (!s) return 0;
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static inline void* greenc_memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

// static inline void* greenc_copy_mem(const void* src, size_t srclen) {
//     if (!src) return NULL;

//     char* copy = (char*)greenc_malloc(srclen);
//     if (!copy) return NULL; // Could not allocate memory

//     for (size_t i = 0; i < srclen; i++) copy[i] = (char*)src[i];

//     return copy;
// }

static inline char* greenc_copy_chars(const char* src) {
    if (!src) return NULL;

    const char* s = (const char*)src;
    if (!s) return NULL; // Could not cast

    int srclen = greenc_strlen(s);

    char* copy = (char*)greenc_malloc(srclen + 1);
    if (!copy) return NULL; // Could not allocate memory

    for (size_t i = 0; i < srclen; i++) copy[i] = s[i];
    copy[srclen] = '\0'; // Make it a string

    return copy;
}

static inline char* greenc_concat_counted(int count, ...) {
    va_list args;
    size_t total_len = 0;

    /* Pass 1: Measure */
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char* s = va_arg(args, const char*);
        total_len += greenc_strlen(s);
    }
    va_end(args);

    /* Allocate exact size + null terminator */
    char* result = (char*)greenc_malloc(total_len + 1);
    if (!result) return NULL;

    /* Pass 2: Copy */
    char* ptr = result;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char* s = va_arg(args, const char*);
        size_t len = greenc_strlen(s);
        if (s && len > 0) {
            greenc_memcpy(ptr, s, len);
            ptr += len;
        }
    }
    va_end(args);

    *ptr = '\0';
    return result;
}

static inline int greenc_string_startswith(const char* s, const char* prefix) {
    if (!s || !prefix) return 0;

    int slen = greenc_strlen(s);
    int tlen = greenc_strlen(prefix);

    if (tlen > slen) return 0;

    // int end = slen - tlen;

    for (int i = 0; i < tlen; i++) {
        if (s[i] != prefix[i])
            return 0;
    }

    return 1;
}

static inline int greenc_string_endswith(const char* s, const char* suffix) {
    if (!s || !suffix) return 0;

    int slen = greenc_strlen(s);
    int tlen = greenc_strlen(suffix);

    if (tlen > slen) return 0;

    int start = slen - tlen;

    for (int i = 0; i < tlen; i++) {
        if (s[start + i] != suffix[i])
            return 0;
    }

    return 1;
}
