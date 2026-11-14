//#pragma file "compiler/libs/sys/start.h"
#pragma once

#if defined(__linux__) && defined(GREENC_MAIN) && !defined(GREENC_HOSTED)

#include "exit.h"

// ----------------------------
// forward declaration
// ----------------------------
int greenc_main(int argc, char** argv, char** envp);

// ----------------------------
// _start
// ----------------------------
__attribute__((noreturn))
void _start() {

#if defined(__x86_64__)

    long argc;
    char** argv;

    __asm__ volatile (
        "mov (%%rsp), %0\n"
        "lea 8(%%rsp), %1\n"
        : "=r"(argc), "=r"(argv)
        :
        : "memory"
    );

    char** envp = argv + argc + 1;

    int ret = greenc_main((int)argc, argv, envp);
    sys_exit(ret);

#elif defined(__aarch64__)

    long argc;
    char** argv;

    __asm__ volatile (
        "ldr %0, [sp]\n"
        "add %1, sp, #8\n"
        : "=r"(argc), "=r"(argv)
        :
        : "memory"
    );

    char** envp = argv + argc + 1;

    int ret = greenc_main((int)argc, argv, envp);
    sys_exit(ret);

#else
    #error Unsupported architecture
#endif
}

#endif


#if !defined(_WIN32) && defined(GREENC_MAIN) && !defined(GREENC_HOSTED)
int main(int argc, char** argv, char** envp) {
    return greenc_main(argc, argv, envp);
}
#endif

#if defined(GREENC_HOSTED) && defined(GREENC_MAIN)
int greenc_main(int argc, char** argv, char** envp);

int main(int argc, char** argv) {
    return greenc_main(argc, argv, NULL);
}
#endif

#if defined(_WIN32) && defined(GREENC_MAIN) && !defined(GREENC_HOSTED)

#include "winstart.h"

#endif
