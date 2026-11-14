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

    size_t used = 0, capacity = 4096;
    char* buffer = greenc_malloc(capacity + 1);
    while (buffer) {
        if (used == capacity) {
            char* larger = capacity < ((size_t)-1) / 2 ? greenc_malloc(capacity * 2 + 1) : NULL;
            if (larger) {
                greenc_memcpy(larger, buffer, used);
                capacity *= 2;
            }

            greenc_free(buffer);
            buffer = larger;
            if (!buffer) { break; }
        }

        size_t room = capacity - used;
        isize count = sys_read(fd, buffer + used, room < 65536 ? room : 65536);
        if (count < 0) {
            greenc_free(buffer);
            buffer = NULL;
            break;
        }

        if (!count) {
            buffer[used] = '\0';
            break;
        }

        used += (size_t)count;
    }

    sys_close(fd);
    return buffer;
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
