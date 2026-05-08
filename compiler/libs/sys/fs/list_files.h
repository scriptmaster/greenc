#pragma once

#include "../builtin/types.h"
#include "../builtin/alloc.h"
#include "../builtin/strings.h"
#include "path.h"
#include "listdir.h"

typedef int (*greenc_file_filter)(const char* name);

// ----------------------------
// internal recursion
// ----------------------------
static void greenc_files_list_internal(
    const char* path,
    string_list* list,
    greenc_file_filter filter
) {
    greenc_dirent* entries =
        (greenc_dirent*)greenc_malloc(sizeof(greenc_dirent) * 128);

    if (!entries) return;

    int count = greenc_listdir(path, entries, 128);

    for (int i = 0; i < count; i++) {

        char* full = greenc_path_join(path, entries[i].name);

        if (entries[i].is_dir) {
            greenc_files_list_internal(full, list, filter);
            greenc_free(full);
        } else {

            if (filter && !filter(entries[i].name)) {
                greenc_free(full);
                continue;
            }

            greenc_string_list_push(list, full);
        }
    }

    greenc_free(entries);
}

// ----------------------------
// public API
// ----------------------------
static inline string_list greenc_files_collect_filter(
    const char* root,
    greenc_file_filter filter
) {
    string_list list;
    greenc_string_list_init(&list);

    greenc_files_list_internal(root, &list, filter);

    return list;
}

static inline string_list greenc_files_collect(
    const char* root
) {
    string_list list;
    greenc_string_list_init(&list);

    greenc_files_list_internal(root, &list, NULL);

    return list;
}
