#pragma once
#include "types.h"


#define tostring(x) _Generic((x), \
    int: int_tostring \
)(x)

#define str(x) tostring(x).data

// -------------------------------------
// int -> string
// -------------------------------------
static inline buf32 int_tostring(int value) {

    buf32 s = {0};

    int i = 30;
    int negative = 0;

    long long n = value;

    if (n < 0) {
        negative = 1;
        n = -n;
    }

    do {
        s.data[i--] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);

    if (negative) {
        s.data[i--] = '-';
    }

    int start = i + 1;
    int out = 0;

    while (s.data[start]) {
        s.data[out++] = s.data[start++];
    }

    s.data[out] = 0;

    return s;
}
