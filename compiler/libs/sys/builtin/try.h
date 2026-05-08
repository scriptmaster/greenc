#pragma once

#define Default(T) ((T){0})

#define Try(expr) ({ \
    __auto_type _tmp = (expr); \
    if (_tmp.error) \
        (__typeof__(_tmp.value)){0}; \
    else \
        _tmp.value; \
})

#define TryReturn(expr) ({ \
    __auto_type _tmp = (expr); \
    if (_tmp.error) \
        return _tmp; \
    _tmp.value; \
})

#define TryOr(expr, fallback) ({ \
    __auto_type _tmp = (expr); \
    if (_tmp.error) \
        (fallback); \
    else \
        _tmp.value; \
})
