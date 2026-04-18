#pragma once

// ----------------------------
// Core types (portable)
// ----------------------------
typedef unsigned long long usize;
typedef long long isize;
typedef unsigned long long size_t;

// ----------------------------
// NULL
// ----------------------------
#ifndef NULL
#define NULL ((void*)0)
#endif



// ----------------------------
// Heap Allocated Types
// ----------------------------
typedef struct {
    char** items;
    int count;
    int cap;
} string_list;

typedef struct {
    int* items;
    int count;
    int cap;
} int_list;

typedef struct {
    float* items;
    int count;
    int cap;
} float_list;

typedef struct {
    float** items;
    int count;
    int cap;
} float_dimensions;

// typedef struct {
//     char** items;
//     int count;
//     int cap;
// } queue;
// ringbuffer, string etc.
