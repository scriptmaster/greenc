#pragma once
#include "types.h"
#include "alloc.h"
#include "strings.h"

typedef struct {
    char* key;
    void* val;
    int used; // 0 empty, 1 used
} greenc_dict_entry;

typedef struct {
    greenc_dict_entry* entries;
    int cap;
    int count;
} greenc_dict;

static inline unsigned long greenc_hash(const char* s) {
    unsigned long h = (unsigned long)1469598103934665603ull; // FNV-1a
    for (int i = 0; s[i]; i++) {
        h ^= (unsigned char)s[i];
        h *= 1099511628211ull;
    }
    return h;
}

static inline void greenc_dict_init(greenc_dict* d) {
    d->cap = 16;
    d->count = 0;
    d->entries = (greenc_dict_entry*)greenc_malloc(sizeof(greenc_dict_entry) * d->cap);
    for (int i = 0; i < d->cap; i++) d->entries[i].used = 0;
}

static void greenc_dict_resize(greenc_dict* d) {
    int old_cap = d->cap;
    greenc_dict_entry* old = d->entries;

    d->cap *= 2;
    d->entries = (greenc_dict_entry*)greenc_malloc(sizeof(greenc_dict_entry) * d->cap);
    for (int i = 0; i < d->cap; i++) d->entries[i].used = 0;
    d->count = 0;

    for (int i = 0; i < old_cap; i++) {
        if (!old[i].used) continue;
        // reinsert
        unsigned long h = greenc_hash(old[i].key);
        int idx = h % d->cap;
        while (d->entries[idx].used) idx = (idx + 1) % d->cap;

        d->entries[idx] = old[i];
        d->entries[idx].used = 1;
        d->count++;
    }
    greenc_free(old);
}

static inline void greenc_dict_put(greenc_dict* d, char* key, void* val) {
    if (d->count * 2 >= d->cap) greenc_dict_resize(d);

    unsigned long h = greenc_hash(key);
    int idx = h % d->cap;

    while (d->entries[idx].used) {
        if (greenc_strcmp(d->entries[idx].key, key) == 0) {
            d->entries[idx].val = val;
            return;
        }
        idx = (idx + 1) % d->cap;
    }

    d->entries[idx].key = key;
    d->entries[idx].val = val;
    d->entries[idx].used = 1;
    d->count++;
}

static inline void* greenc_dict_get(greenc_dict* d, const char* key) {
    unsigned long h = greenc_hash(key);
    int idx = h % d->cap;

    while (d->entries[idx].used) {
        if (greenc_strcmp(d->entries[idx].key, key) == 0)
            return d->entries[idx].val;
        idx = (idx + 1) % d->cap;
    }
    return 0;
}

static void* greenc_dict_get_len(
    greenc_dict* d,
    const char* s,
    int len
) {
    for (int i = 0; i < d->cap; i++) {
        if (!d->entries[i].used) continue;

        if (greenc_strlen(d->entries[i].key) != len)
            continue;

        int match = 1;
        for (int j = 0; j < len; j++) {
            if (d->entries[i].key[j] != s[j]) {
                match = 0; break;
            }
        }

        if (match) return d->entries[i].val;
    }
    return 0;
}
