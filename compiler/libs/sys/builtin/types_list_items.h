//#file libs/sys/builtin/types_list_items.h
#pragma once

// ----------------------------
// Heap Allocated Lists
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
