#pragma once
#include "alloc.h"

typedef struct {
    void** data;
    int cap;
    int head;
    int tail;
} greenc_queue;

static inline void greenc_queue_init(greenc_queue* q, int cap) {
    q->cap = cap;
    q->data = (void**)greenc_malloc(sizeof(void*) * cap);
    q->head = q->tail = 0;
}

static inline int greenc_queue_empty(greenc_queue* q) {
    return q->head == q->tail;
}

static inline void greenc_queue_push(greenc_queue* q, void* v) {
    q->data[q->tail] = v;
    q->tail = (q->tail + 1) % q->cap;
}

static inline void* greenc_queue_pop(greenc_queue* q) {
    if (greenc_queue_empty(q)) return 0;
    void* v = q->data[q->head];
    q->head = (q->head + 1) % q->cap;
    return v;
}

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
