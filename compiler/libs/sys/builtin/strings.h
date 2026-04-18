#pragma once
#ifndef GREENC_STRINGS_H
#define GREENC_STRINGS_H
#include "types.h"
#include "stdarg.h"
#include "alloc.h"

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

static inline char* greenc_concat_impl(int count, ...) {
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

/* Macro to automatically count arguments (up to 10) */
#define _GREENC_COUNT(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10, N, ...) N
#define greenc_concat(...) \
    greenc_concat_impl(_GREENC_COUNT(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1), __VA_ARGS__)



static inline char* greenc_itoa(int value) {
    char buf[32];
    int i = 30;
    buf[31] = '\0';

    int neg = value < 0;
    if (neg) value = -value;

    do {
        buf[i--] = '0' + (value % 10);
        value /= 10;
    } while (value);

    if (neg) buf[i--] = '-';

    return greenc_concat(&buf[i + 1]);
}

#endif
