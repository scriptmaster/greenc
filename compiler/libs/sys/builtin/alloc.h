#ifndef GREENC_ALLOC_H
#define GREENC_ALLOC_H

#if defined(GREENC_HOSTED)
    #include <stdlib.h>
#elif defined(_WIN32)
    #include <windows.h>
#else
    /* Linux x86_64 Syscall Definitions */
    #define GREENC_SYS_MMAP   9
    #define GREENC_SYS_MUNMAP 11
    #define GREENC_MAP_PRIVATE 0x02
    #define GREENC_MAP_ANONYMOUS 0x20
    #define GREENC_PROT_READ 0x1
    #define GREENC_PROT_WRITE 0x2

    typedef struct { size_t size; } greenc_mem_header_t;
#endif

static inline void* greenc_malloc(size_t size) {
    if (size == 0) return NULL;

#if defined(GREENC_HOSTED)
    return malloc(size);
#elif defined(_WIN32)
    /* Windows Heap Allocation */
    return HeapAlloc(GetProcessHeap(), 0, size);
#else
    /* Linux mmap Allocation */
    size_t total_size = sizeof(greenc_mem_header_t) + size;
    void* addr;

    __asm__ volatile (
        "movq %5, %%r10\n\t"
        "movq %6, %%r8\n\t"
        "movq %7, %%r9\n\t"
        "syscall"
        : "=a" (addr)
        : "a" ((long)GREENC_SYS_MMAP),
          "D" (NULL),
          "S" (total_size),
          "d" (GREENC_PROT_READ | GREENC_PROT_WRITE),
          "r" ((long)(GREENC_MAP_PRIVATE | GREENC_MAP_ANONYMOUS)),
          "r" ((long)-1),
          "r" ((long)0)
        : "rcx", "r11", "r10", "r8", "r9", "memory"
    );

    if ((long)addr < 0) return NULL;

    greenc_mem_header_t* h = (greenc_mem_header_t*)addr;
    h->size = total_size;
    return (void*)(h + 1);
#endif
}

static inline void greenc_free(void* ptr) {
    if (!ptr) return;

#if defined(GREENC_HOSTED)
    free(ptr);
#elif defined(_WIN32)
    HeapFree(GetProcessHeap(), 0, ptr);
#else
    greenc_mem_header_t* h = (greenc_mem_header_t*)ptr - 1;
    __asm__ volatile (
        "syscall"
        :
        : "a" ((long)GREENC_SYS_MUNMAP), "D" (h), "S" (h->size)
        : "rcx", "r11", "memory"
    );
#endif
    // ptr = NULL; // no effect, it must set to null at source
}

static inline void greenc_mfree(void** ptr) {
    if (!ptr || !*ptr) { return; }

    greenc_free(*ptr);
    *ptr = NULL;
}

#endif

#define greenc_release(ptr) greenc_mfree((void**)&(ptr))
