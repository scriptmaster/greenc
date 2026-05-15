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
