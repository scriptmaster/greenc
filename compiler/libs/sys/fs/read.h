#include "../builtin/alloc.h"
#include "../builtin/strings1.h"
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

static inline int greenc_write_file(const char* path, const char* data) {
    usize len = greenc_strlen(data);

#ifdef _WIN32

    greenc_fd fd = sys_open_write_create(path);

    if (fd == INVALID_HANDLE_VALUE) {
        return -1;
    }

    DWORD written = 0;

    if (
        !WriteFile(
            fd,
            data,
            (DWORD)len,
            &written,
            NULL
        )
    ) {
        sys_close(fd);
        return -1;
    }

    sys_close(fd);

    return written == (DWORD)len ? 0 : -1;

#else

    greenc_fd fd = sys_open_write_create(path);

    if ((isize)fd < 0) {
        return -1;
    }

    isize n = sys_write_fd(fd, data, len);

    sys_close(fd);

    return n == (isize)len ? 0 : -1;

#endif
}
