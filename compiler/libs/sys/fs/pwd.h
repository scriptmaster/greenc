#pragma once
#include "../builtin/types.h"

#ifdef _WIN32

#include <windows.h>

static inline usize sys_getcwd(char* buf, usize size) {
    return GetCurrentDirectoryA((DWORD)size, buf);
}

#else

#if defined(__x86_64__)
    #define SYS_GETCWD 79
#elif defined(__aarch64__)
    #define SYS_GETCWD 17
#else
    #error Unsupported architecture
#endif

static inline isize sys_getcwd(char* buf, usize size) {
    isize ret;

#if defined(__x86_64__)
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_GETCWD), "D"(buf), "S"(size)
        : "rcx", "r11", "memory"
    );
#elif defined(__aarch64__)
    register isize x0 __asm__("x0") = (isize)buf;
    register usize x1 __asm__("x1") = size;
    register isize x8 __asm__("x8") = SYS_GETCWD;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    ret = x0;
#endif

    return ret;
}

#endif
