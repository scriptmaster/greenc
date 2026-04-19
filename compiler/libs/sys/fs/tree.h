#pragma once

#include "../builtin/strings.h"
#include "../builtin/alloc.h"
#include "../sys_write.h"
#include "path.h"
#include "listdir.h"

// forward
static void greenc_tree_internal(
    const char* path,
    const char* prefix,
    int* dir_count,
    int* file_count
);

// public entry
static inline void greenc_tree(const char* path) {

    int dir_count = 0;
    int file_count = 0;

    greenc_tree_internal(path, "", &dir_count, &file_count);

    // newline if something printed
    if (dir_count > 0 || file_count > 0)
        sys_println("");

    // summary
    char* summary = greenc_concat(
        greenc_itoa(dir_count), " directories, ",
        greenc_itoa(file_count), " files"
    );

    if (summary) {
        sys_println(summary);
        greenc_free(summary);
    }
}

// recursive worker
static void greenc_tree_internal(
    const char* path,
    const char* prefix,
    int* dir_count,
    int* file_count
) {

    greenc_dirent* entries =
        (greenc_dirent*)greenc_malloc(sizeof(greenc_dirent) * 128);

    if (!entries) return;

    int count = greenc_listdir(path, entries, 128);

    for (int i = 0; i < count; i++) {

        int is_last = (i == count - 1);

        // prefix + branch
        sys_write(prefix, greenc_strlen(prefix));

        if (is_last)
            sys_write("`-- ", 4);
        else
            sys_write("+-- ", 4);

        sys_println(entries[i].name);

        if (entries[i].is_dir) {

            (*dir_count)++;

            char* new_path = greenc_path_join(path, entries[i].name);

            char* new_prefix;
            if (is_last)
                new_prefix = greenc_concat(prefix, "    ");
            else
                new_prefix = greenc_concat(prefix, "|   ");

            greenc_tree_internal(new_path, new_prefix, dir_count, file_count);

            greenc_free(new_path);
            greenc_free(new_prefix);

        } else {
            (*file_count)++;
        }
    }

    greenc_free(entries);
}
