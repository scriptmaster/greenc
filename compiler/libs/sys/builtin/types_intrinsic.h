//#file libs/sys/builtin/types_intrinsic.h
#pragma once

// ----------------------------
// Core types (portable)
// ----------------------------
typedef unsigned long long usize;
typedef unsigned long long u64;
typedef long long isize;
typedef long long i64;
/*
 * Replace usize and isize?
 * 		No — keep usize and isize.
 * They are the correct types for syscall/platform/runtime code.
 * u64 is NOT semantically the same thing.
 * 
 * */

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


#define KB(x) ((x) * 1024)
// -------------------------------------
// fixed stack buffer
// -------------------------------------
typedef struct { char data[32]; } buf32;
typedef struct { char data[64]; } buf64;
typedef struct { char data[128]; } buf128;
typedef struct { char data[256]; } buf256;
typedef struct { char data[KB(1)]; } buf1k;
typedef struct { char data[KB(4)]; } buf4k;
typedef struct { char data[KB(8)]; } buf8k;
typedef struct { char data[KB(16)]; } buf16k;
typedef struct { char data[KB(64)]; } buf64k;
