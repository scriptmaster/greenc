#pragma once
#include "../builtin/types.h"

#ifdef _WIN32

#include <windows.h>

typedef struct {
    char name[256];
    int is_dir;
} gc_dirent;

static inline int gc_listdir(const char* path, gc_dirent* out, int max) {
    WIN32_FIND_DATAA data;
    char search[512];

    // path + "\*"
    int i = 0;
    while (path[i]) { search[i] = path[i]; i++; }
    search[i++] = '\\';
    search[i++] = '*';
    search[i] = 0;

    HANDLE h = FindFirstFileA(search, &data);
    if (h == INVALID_HANDLE_VALUE) return 0;

    int count = 0;

    do {
        if (count >= max) break;

        char* name = data.cFileName;

        // skip . and ..
        if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))
            continue;

        // copy name
        int j = 0;
        while (name[j] && j < 255) {
            out[count].name[j] = name[j];
            j++;
        }
        out[count].name[j] = 0;

        out[count].is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        count++;

    } while (FindNextFileA(h, &data));

    FindClose(h);
    return count;
}

#else

// ----------------------------
// Linux getdents64 syscall
// ----------------------------

#define SYS_getdents64 217

typedef struct {
    char name[256];
    int is_dir;
} gc_dirent;

struct linux_dirent64 {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

static inline int sys_open_dir(const char* path) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(2), "D"(path), "S"(0), "d"(0)
        : "rcx", "r11", "memory"
    );
    return (int)ret;
}

static inline int gc_listdir(const char* path, gc_dirent* out, int max) {

    char buf[4096];
    int fd = sys_open_dir(path);
    if (fd < 0) return 0;

    int count = 0;

    while (1) {
        long nread;

        __asm__ volatile (
            "syscall"
            : "=a"(nread)
            : "a"(SYS_getdents64), "D"(fd), "S"(buf), "d"(sizeof(buf))
            : "rcx", "r11", "memory"
        );

        if (nread <= 0) break;

        int bpos = 0;
        while (bpos < nread) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + bpos);

            char* name = d->d_name;

            // skip . and ..
            if (!(name[0] == '.' &&
                (name[1] == 0 || (name[1] == '.' && name[2] == 0)))) {

                if (count >= max) break;

                int j = 0;
                while (name[j] && j < 255) {
                    out[count].name[j] = name[j];
                    j++;
                }
                out[count].name[j] = 0;

                out[count].is_dir = (d->d_type == 4); // DT_DIR
                count++;
            }

            bpos += d->d_reclen;
        }
    }

    return count;
}

#endif
