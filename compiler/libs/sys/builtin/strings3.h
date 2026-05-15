#pragma once
#include "strings1.h"

#define tostring(x) _Generic((x), \
    int: int_tostring \
)(x)

#define str(x) tostring(x).str
#define tocstring(x) x.str

#define itoa int_tostring
#define itoa_alloc greenc_itoa_alloc

// -------------------------------------
// int -> string
// -------------------------------------

static inline buf32 int_tostring(int value) {
    buf32 s = {0};

    int i = sizeof(s.str) - 2;
    int negative = 0;

    long long n = value;

    if (n < 0) {
        negative = 1;
        n = -(long long)n;
    }

    do {
        s.str[i--] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);

    if (negative) {
        s.str[i--] = '-';
    }

    int start = i + 1;
    int out = 0;

    while (s.str[start]) {
        s.str[out++] = s.str[start++];
    }

    s.str[out] = 0;

    // In C: returning a struct (non-intrinsic type) returns a copy of the struct, not the original stack address.
    return s;
    /** Here: `return s;` is Equivalent to:
      * buf32 temp;
      * memcpy(&return_slot, &temp, sizeof(buf32));
    */
}

static inline char* greenc_itoa_alloc(int value) {
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

    return greenc_copy(&buf[i + 1]);
}
