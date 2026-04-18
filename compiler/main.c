#include "libgreenc.h"

#define VERSION "v1.0.5"


static int filter_c_files(const char* name) {
    return string_endswith(name, ".c") || string_endswith(name, ".h");
}

int gc_main() {
    int result = 1;
    print("green c (" VERSION ")\n");

    char* msg = greenc_concat("Version: ", VERSION);
    // defer msg;

    if (msg) {
        print(msg);
        free(msg);
    }

    char* cwd = gc_getcwd_alloc();
    if (cwd) {
        print("cwd:");
        print(cwd);

        tree(cwd);
    }

    char* content = gc_read_file("main.c");
    if (content) {
        print("file read ok");
        int len = greenc_strlen(content);
        free(content);
    } else {
        print("file not found");
    }

    string_list files = gc_files_collect_filter(cwd, filter_c_files);

    for (int i = 0; i < files.count; i++) {
        sys_println(files.items[i]);
    }

    gc_string_list_free(&files);

    print("done.");

    return 0;
}
