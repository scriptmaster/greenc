#pragma once

// ----------------------------
// Core types (portable)
// ----------------------------
typedef unsigned long long usize;
typedef long long isize;


// {
// typedef unsigned long long size_t;
//
#ifndef __SIZE_TYPE__
typedef unsigned long size_t;   // fallback (most common ABI)
#else
typedef __SIZE_TYPE__ size_t;   // compiler-provided (best)
#endif
//
// size_t;
// }

typedef char* cstring;        // owned heap string
typedef const char* cstr;     // borrowed null-terminated string

typedef struct {
    char* data;               // owned
    size_t len;
} string;

typedef struct {
    const char* data;         // borrowed
    size_t len;
} string_view;

#define strview string_view


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
