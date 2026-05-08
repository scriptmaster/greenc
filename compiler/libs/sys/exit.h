//#pragma file "compiler/libs/sys/exit.h"
#pragma once

#ifdef __linux__

#include "builtin/types.h"

// ----------------------------
// sys_exit (multi-arch)
// ----------------------------
static inline __attribute__((noreturn)) void sys_exit(int code) {

#if defined(__x86_64__)

    __asm__ volatile (
        "syscall"
        :
        : "a"(60), "D"(code)
        : "rcx", "r11", "memory"
    );

#elif defined(__aarch64__)

    register long x0 __asm__("x0") = code;
    register long x8 __asm__("x8") = 93; // SYS_exit for ARM64

    __asm__ volatile (
        "svc #0"
        :
        : "r"(x0), "r"(x8)
        : "memory"
    );

#else
    #error Unsupported architecture
#endif

    // Safety loop in emulator environments
    while (1) { }
}

#endif
