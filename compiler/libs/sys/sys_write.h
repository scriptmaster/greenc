#pragma once
#include "builtin/types.h"
#include "builtin/strings1.h"

static inline size_t greenc_strlen(const char* s);

// ----------------------------
// Windows
// ----------------------------
#ifdef _WIN32

#include <windows.h>

static inline void sys_write(const char* buf, usize len) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteFile(h, buf, (DWORD)len, &written, 0);
}

// ----------------------------
// Unix (Linux + macOS)
// ----------------------------
#else

#ifdef GREENC_HOSTED
#include <unistd.h>
#endif

// ----------------------------
// syscall_write (multi-platform)
// ----------------------------
static inline isize syscall_write(int fd, const void* buf, usize len) {

#if defined(GREENC_HOSTED)
    return (isize)write(fd, buf, (size_t)len);
#elif defined(__x86_64__)

    #if defined(__APPLE__)
        #define SYS_WRITE 0x2000004
    #else
        #define SYS_WRITE 1
    #endif

    isize ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_WRITE), "D"(fd), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
    return ret;

#elif defined(__aarch64__)

    #if defined(__APPLE__)
        #define SYS_WRITE 0x2000004
    #else
        #define SYS_WRITE 64
    #endif

    register isize x0 __asm__("x0") = fd;
    register const void* x1 __asm__("x1") = buf;
    register usize x2 __asm__("x2") = len;
    register isize x8 __asm__("x8") = SYS_WRITE;

    __asm__ volatile (
        "svc #0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x8)
        : "memory"
    );

    return x0;

#else
    #error Unsupported architecture
#endif
}

// ----------------------------
// sys_write wrapper
// ----------------------------
static inline void sys_write(const char* buf, usize len) {
    while (len > 0) {
        isize n = syscall_write(1, buf, len);
        if (n <= 0) break;
        buf += n;
        len -= (usize)n;
    }
}

#endif

// internal function only
static inline void sys_print(const char* msg) {
    sys_write(msg, greenc_strlen(msg));
}

// internal function only
static inline void sys_println(const char* msg) {
    sys_write(msg, greenc_strlen(msg));
    sys_write("\n", 1);
}
