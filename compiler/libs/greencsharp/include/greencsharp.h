/*
 ** greencsharp.h - 1 function declaration.
 ** greencsharp_get_api - Return the versioned native C# translation interface.
 ** All result strings are borrowed until destroy; allocation never crosses the ABI.
 ** See also: ../src/api.c, ../README.md.
 */
#pragma once
#include "sys/builtin/types_intrinsic.h"

#define GREENC_CSHARP_ABI 2u
#define GCS_ANALYZE 0u
#define GCS_BUILD 1u
#define GCS_EMIT_C 2u
#define GCS_REBUILD 3u
#define GCS_CLEAN 4u
#if !defined(GREENC_CSHARP_BUILD)
#define GCS_EXPORT
#elif defined(_WIN32)
#define GCS_EXPORT __declspec(dllexport)
#else
#define GCS_EXPORT __attribute__((visibility("default")))
#endif

typedef struct {
    u32 size;
    u32 version;
    const char* input_path;
    const char* output_path;
    const char* configuration;
    u32 mode;
} greencsharp_request;

typedef struct {
    u32 size;
    i32 status;
    u32 artifact_kind; /* 1: native executable; 2: target-specific static archive. */
    const char* diagnostic;
    const char* program;
    const char* ast;
    const char* dependencies;
    const char* output_path;
} greencsharp_result;

typedef struct {
    u32 size;
    u32 version;
    void* (*create)(const greencsharp_request* request);
    const greencsharp_result* (*result)(void* compilation);
    void (*destroy)(void* compilation);
} greencsharp_api;

typedef const greencsharp_api* (*greencsharp_api_entry)(u32 version);
GCS_EXPORT const greencsharp_api* greencsharp_get_api(u32 version);
