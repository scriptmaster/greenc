#pragma once
#include "types.h"

// SEARCH: greenc_str_equals(
static inline int greenc_str_equals(
    const char* a, int a_len,
    const char* b
) {
    int i = 0;
    while (b[i] != 0) {
        if (i >= a_len) return 0;
        if (a[i] != b[i]) return 0;
        i++;
    }
    return i == a_len;
}

static int greenc_strcmp(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        i++;
    }
    return (unsigned char)a[i] - (unsigned char)b[i];
}

static void greenc_string_list_sort(string_list* list) {
    for (int i = 1; i < list->count; i++) {
        char* key = list->items[i];
        int j = i - 1;
        while (j >= 0 && greenc_strcmp(list->items[j], key) > 0) {
            list->items[j + 1] = list->items[j];
            j--;
        }
        list->items[j + 1] = key;
    }
}
