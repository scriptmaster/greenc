//#file libs/sys/builtin/types_intrinsic.h
#pragma once

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef float f32;
typedef double f64;

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

// typedef char* cstring;        /// owned    \0 string
// typedef const char* cstr;     /// borrowed \0 string
#define cstring char*
#define cstr const char*
#define conststring const char*

typedef struct {
    char* str;               /// owned
    size_t len;
} string;

typedef struct {
    const char* str;         /// borrowed
    size_t len;
} string_view;

typedef struct {
    char* str;               /// owned
    size_t len;
    size_t cap;
} string_builder;


#define strview string_view


// ----------------------------
// NULL
// ----------------------------
#ifndef NULL
#define NULL ((void*)0)
#endif

// @see: buffers.h for greenc_queue and greenc_ring

#define KB(x) ((x) * 1024)
// -------------------------------------
// fixed stack buffer
// -------------------------------------
typedef struct { char str[32]; } buf32;
typedef struct { char str[64]; } buf64;
typedef struct { char str[128]; } buf128;
typedef struct { char str[256]; } buf256;
typedef struct { char str[KB(1)]; } buf1k;
typedef struct { char str[KB(4)]; } buf4k;
typedef struct { char str[KB(8)]; } buf8k;
// beyond this use arena/page allocations


#define buffer buf256
#define bigbuffer buf1k
#define hugebuffer buf4k
#define largebuffer buf8k
