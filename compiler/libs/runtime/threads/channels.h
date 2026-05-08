/*
 * Cross platform Channel Implementation
 * 
 * This implementation uses a circular buffer (First-In, First-Out) to act as a thread-safe channel.
 * 
 * Slow alternatives: zmq, odin, futures in other languages.
 * 
 * gcc -std=c17 -O2 main.c -o my_app -lpthread
 * 
 */

#include <stdio.h>
#include <threads.h>
#include <stdlib.h>

#define CHAN_SIZE 5

typedef struct {
    int buffer[CHAN_SIZE];
    int head; // Points to the next slot to write
    int tail; // Points to the next slot to read
    int count;
    mtx_t lock;
    cnd_t can_produce;
    cnd_t can_consume;
} Channel;

void chan_init(Channel *c) {
    c->head = 0;
    c->tail = 0;
    c->count = 0;
    mtx_init(&c->lock, mtx_plain);
    cnd_init(&c->can_produce);
    cnd_init(&c->can_consume);
}

void chan_send(Channel *c, int val) {
    mtx_lock(&c->lock);
    while (c->count == CHAN_SIZE) {
        cnd_wait(&c->can_produce, &c->lock);
    }
    c->buffer[c->head] = val;
    c->head = (c->head + 1) % CHAN_SIZE;
    c->count++;
    
    cnd_signal(&c->can_consume);
    mtx_unlock(&c->lock);
}

int chan_receive(Channel *c) {
    mtx_lock(&c->lock);
    while (c->count == 0) {
        cnd_wait(&c->can_consume, &c->lock);
    }
    int val = c->buffer[c->tail];
    c->tail = (c->tail + 1) % CHAN_SIZE;
    c->count--;

    cnd_signal(&c->can_produce);
    mtx_unlock(&c->lock);
    return val;
}
