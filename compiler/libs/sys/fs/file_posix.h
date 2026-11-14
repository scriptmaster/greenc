/* POSIX hosted file descriptors. Included by file.h with builtin types loaded. */
#pragma once
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int greenc_fd;

static inline greenc_fd sys_open_read(const char* path) {
    return open(path, O_RDONLY);
}

static inline isize sys_read(greenc_fd fd, void* buffer, usize length) {
    ssize_t count;

    do {
        count = read(fd, buffer, (size_t)length);
    } while (count < 0 && errno == EINTR);

    return (isize)count;
}

static inline void sys_close(greenc_fd fd) {
    close(fd);
}

static inline greenc_fd sys_open_write_create(const char* path) {
    return open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

static inline isize sys_write_fd(greenc_fd fd, const void* buffer, usize length) {
    size_t written = 0;

    while (written < length) {
        ssize_t count = write(fd, (const char*)buffer + written, (size_t)length - written);

        if (count < 0 && errno == EINTR) { continue; }
        if (count <= 0) { return -1; }

        written += (size_t)count;
    }

    return (isize)written;
}

static inline int greenc_mkdir_one(const char* path) {
    return mkdir(path, 0755) == 0 || errno == EEXIST ? 0 : -1;
}
