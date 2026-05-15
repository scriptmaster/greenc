#pragma once
#include "alloc.h"

typedef struct {
    char* buf;
    int cap;
    int r;
    int w;
} greenc_ring;

static inline void greenc_ring_init(greenc_ring* rb, int cap) {
    rb->buf = (char*)greenc_malloc(cap);
    rb->cap = cap;
    rb->r = rb->w = 0;
}

static inline int greenc_ring_write(greenc_ring* rb, const char* data, int len) {
    int written = 0;
    for (int i = 0; i < len; i++) {
        int next = (rb->w + 1) % rb->cap;
        if (next == rb->r) break; // full
        rb->buf[rb->w] = data[i];
        rb->w = next;
        written++;
    }
    return written;
}

static inline int greenc_ring_read(greenc_ring* rb, char* out, int len) {
    int read = 0;
    while (rb->r != rb->w && read < len) {
        out[read++] = rb->buf[rb->r];
        rb->r = (rb->r + 1) % rb->cap;
    }
    return read;
}
