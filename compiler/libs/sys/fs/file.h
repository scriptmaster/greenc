#pragma once
#include "../builtin/types.h"

#ifdef _WIN32

#include <windows.h>

// ----------------------------
// Windows File API
// ----------------------------
typedef HANDLE greenc_fd;

static inline greenc_fd sys_open_read(const char* path) {
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

static inline isize sys_read(greenc_fd fd, void* buf, usize len) {
    DWORD read = 0;
    if (!ReadFile(fd, buf, (DWORD)len, &read, 0)) return -1;
    return (isize)read;
}

static inline void sys_close(greenc_fd fd) {
    CloseHandle(fd);
}

static inline greenc_fd sys_open_write_create(const char* path) {
    return CreateFileA(
        path,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
}

static inline int greenc_mkdir_one(const char* path) {
    if (CreateDirectoryA(path, NULL)) {
        return 0;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    return -1;
}

#else

// ----------------------------
// Linux Syscalls (x86_64 + ARM64)
// ----------------------------

#if defined(__x86_64__)
    #define SYS_OPEN 2
    #define SYS_READ 0
    #define SYS_WRITE 1
    #define SYS_CLOSE 3
    #define SYS_MKDIR 83
#elif defined(__aarch64__)
    #define SYS_OPEN 56
    #define SYS_READ 63
    #define SYS_WRITE 64
    #define SYS_CLOSE 57
#else
    #error Unsupported architecture
#endif

#define GREENC_O_WRONLY 00000001
#define GREENC_O_CREAT 00000100
#define GREENC_O_TRUNC 00001000

typedef int greenc_fd;

static inline greenc_fd sys_open_read(const char* path) {
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

static inline isize sys_read(greenc_fd fd, void* buf, usize len) {
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

static inline void sys_close(greenc_fd fd) {

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

static inline greenc_fd sys_open_write_create(const char* path) {
    long flags = (long)(GREENC_O_WRONLY | GREENC_O_CREAT | GREENC_O_TRUNC);
    long mode = 0644;
    long ret;

#if defined(__x86_64__)
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_OPEN), "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
#elif defined(__aarch64__)
    register long x0 __asm__("x0") = (long)path;
    register long x1 __asm__("x1") = flags;
    register long x2 __asm__("x2") = mode;
    register long x8 __asm__("x8") = SYS_OPEN;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    ret = x0;
#endif

    return (int)ret;
}

static inline isize sys_write_fd(greenc_fd fd, const void* buf, usize len) {
#if defined(__x86_64__)
    isize ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_WRITE), "D"(fd), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
    return ret;
#elif defined(__aarch64__)
    register isize x0 __asm__("x0") = fd;
    register const void* x1 __asm__("x1") = buf;
    register usize x2 __asm__("x2") = len;
    register isize x8 __asm__("x8") = SYS_WRITE;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
#endif
}

static inline int greenc_mkdir_one(const char* path) {
#if defined(__x86_64__)
    long ret;
    unsigned mode = 0755;

    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"((long)SYS_MKDIR), "D"(path), "S"(mode)
        : "rcx", "r11", "memory"
    );

    if (ret == 0) {
        return 0;
    }

    if (ret == -17) {
        return 0;
    }

    return -1;
#else
    (void)path;
    return -1;
#endif
}

#endif
