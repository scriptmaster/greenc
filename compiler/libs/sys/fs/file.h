#pragma once
#include "../builtin/types.h"

#ifdef _WIN32

#include <windows.h>

// ----------------------------
// Windows File API
// ----------------------------
typedef HANDLE gc_fd;

static inline gc_fd sys_open_read(const char* path) {
    return CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );
}

static inline isize sys_read(gc_fd fd, void* buf, usize len) {
    DWORD read = 0;
    if (!ReadFile(fd, buf, (DWORD)len, &read, 0)) return -1;
    return (isize)read;
}

static inline void sys_close(gc_fd fd) {
    CloseHandle(fd);
}

#else

// ----------------------------
// Linux Syscalls (x86_64 + ARM64)
// ----------------------------

#if defined(__x86_64__)
    #define SYS_OPEN 2
    #define SYS_READ 0
    #define SYS_CLOSE 3
#elif defined(__aarch64__)
    #define SYS_OPEN 56
    #define SYS_READ 63
    #define SYS_CLOSE 57
#else
    #error Unsupported architecture
#endif

typedef int gc_fd;

static inline gc_fd sys_open_read(const char* path) {
    long ret;

#if defined(__x86_64__)
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_OPEN), "D"(path), "S"(0), "d"(0)
        : "rcx", "r11", "memory"
    );
#elif defined(__aarch64__)
    register long x0 __asm__("x0") = (long)path;
    register long x1 __asm__("x1") = 0;
    register long x2 __asm__("x2") = 0;
    register long x8 __asm__("x8") = SYS_OPEN;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
#endif

    return (int)ret;
}

static inline isize sys_read(gc_fd fd, void* buf, usize len) {
    isize ret;

#if defined(__x86_64__)
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_READ), "D"(fd), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
#elif defined(__aarch64__)
    register isize x0 __asm__("x0") = fd;
    register void* x1 __asm__("x1") = buf;
    register usize x2 __asm__("x2") = len;
    register isize x8 __asm__("x8") = SYS_READ;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
#endif

    return ret;
}

static inline void sys_close(gc_fd fd) {

#if defined(__x86_64__)
    __asm__ volatile (
        "syscall"
        :
        : "a"(SYS_CLOSE), "D"(fd)
        : "rcx", "r11", "memory"
    );
#elif defined(__aarch64__)
    register long x0 __asm__("x0") = fd;
    register long x8 __asm__("x8") = SYS_CLOSE;
    __asm__ volatile ("svc #0" : : "r"(x0), "r"(x8) : "memory");
#endif
}

#endif
