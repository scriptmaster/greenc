#include "../builtin/alloc.h"
#include "file.h"
#include "pwd.h"

// ----------------------------
// Read full file (simple)
// ----------------------------
static inline char* greenc_read_file(const char* path) {
    greenc_fd fd = sys_open_read(path);
    if ((isize)fd < 0) return NULL;

    usize cap = 4096;
    char* buf = (char*)greenc_malloc(cap + 1);
    if (!buf) return NULL;

    isize n = sys_read(fd, buf, cap);
    if (n <= 0) {
        sys_close(fd);
        return NULL;
    }

    buf[n] = '\0';
    sys_close(fd);
    return buf;
}

// ----------------------------
// Get current directory
// ----------------------------
static inline char* greenc_getcwd_alloc() {
    usize size = 512;
    char* buf = (char*)greenc_malloc(size);
    if (!buf) return NULL;

    isize n = sys_getcwd(buf, size);
    if (n <= 0) return NULL;

    return buf;
}
