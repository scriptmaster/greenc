#pragma once
#include "../libs/libgreenc.h"
#include "types.h"

static greenc_file_kind greenc_classify(const char* name) {
    if (greenc_string_endswith(name, ".c"))  return GREENC_FILE_C;
    if (greenc_string_endswith(name, ".h"))  return GREENC_FILE_H;
    if (greenc_string_endswith(name, ".gc")) return GREENC_FILE_GC;
    return GREENC_FILE_OTHER;
}


static greenc_build_graph greenc_build_graph_from(string_list* files) {
    greenc_build_graph g;
    greenc_string_list_init(&g.c_files);
    greenc_string_list_init(&g.h_files);
    greenc_string_list_init(&g.gc_files);

    for (int i = 0; i < files->count; i++) {
        char* f = files->items[i];
        switch (greenc_classify(f)) {
            case GREENC_FILE_C:  greenc_string_list_push(&g.c_files,  greenc_concat(f)); break;
            case GREENC_FILE_H:  greenc_string_list_push(&g.h_files,  greenc_concat(f)); break;
            case GREENC_FILE_GC: greenc_string_list_push(&g.gc_files, greenc_concat(f)); break;
            default: break;
        }
    }
    return g;
}
