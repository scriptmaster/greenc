#include "libgreenc.h"

#define VERSION "v1.0.5"

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

    return 0;
}
