/* POSIX hosted directory enumeration; symlinks are reported without following them. */
#pragma once
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    char name[256];
    int is_dir;
} greenc_dirent;

static inline int greenc_listdir(const char* path, greenc_dirent* output, int capacity) {
    DIR* directory = opendir(path);

    if (!directory) { return 0; }

    int count = 0;
    struct dirent* entry;

    while (count < capacity && (entry = readdir(directory)) != NULL) {
        const char* name = entry->d_name;

        if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]))) { continue; }

        size_t length = greenc_strlen(name);

        if (length >= sizeof(output[count].name)) { continue; }

        struct stat attributes;
        greenc_memcpy(output[count].name, name, length + 1);
        output[count].is_dir = fstatat(dirfd(directory), name, &attributes, AT_SYMLINK_NOFOLLOW) == 0 &&
                               S_ISDIR(attributes.st_mode);
        ++count;
    }

    closedir(directory);

    return count;
}
