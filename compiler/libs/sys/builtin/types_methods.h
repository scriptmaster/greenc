#pragma once
#include "alloc.h"
#include "types_intrinsic.h"
#include "../sys_write.h"
#include "strings3.h"

// ----------------------------
// string_list helpers
// ----------------------------
static inline void greenc_string_list_init(string_list* list) {
    list->count = 0;
    list->cap = 16;
    list->items = (char**)greenc_malloc(sizeof(char*) * list->cap);
}

static inline void greenc_string_list_push(string_list* list, char* path) {

	if (!list->items) { // && list->cap == 0) {
		greenc_string_list_init(list);
	}

    if (list->count >= list->cap) {
        int new_cap = list->cap * 2;

        char** new_items =
            (char**)greenc_malloc(sizeof(char*) * new_cap);

        if (!new_items) return;

        for (int i = 0; i < list->count; i++)
            new_items[i] = list->items[i];

        greenc_free(list->items);

        list->items = new_items;
        list->cap = new_cap;
    }

    list->items[list->count++] = path;
}

static inline void greenc_string_list_free(string_list* list) {
    for (int i = 0; i < list->count; i++)
        greenc_free(list->items[i]);

    greenc_free(list->items);
}
