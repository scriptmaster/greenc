#pragma once

// 1. Platform loader
#ifdef _WIN32
#include <windows.h>
typedef HMODULE dynlib;
#define load_dynlib(name) LoadLibraryA(name)
#else
#include <dlfcn.h>
typedef void* dynlib;
#define load_dynlib(name) dlopen(name, RTLD_NOW)
#endif

// 2. Token helpers
#define _libname(name) #name
#define _dllname(name) #name ".dll"
#define _soname(name) "lib" #name ".so"

#define _bindfn(name) name##_bind

// 3. The macro
#ifdef _WIN32
#define bind_library(name)                                      \
    dynlib name##_lib = load_dynlib(_dllname(name));            \
    if (!name##_lib) return 1;                                  \
    if (!_bindfn(name)(name##_lib)) return 2;
#else
#define bind_library(name)                                      \
    dynlib name##_lib = load_dynlib(_soname(name));             \
    if (!name##_lib) return 1;                                  \
    if (!_bindfn(name)(name##_lib)) return 2;
#endif

// 4. Sample usage
/*
 * llama.bindings.h:
 * 
 * bool llama_bind(void* handle) {
 *   // ... bind all functions in the dll with the loaded handle
 *   return 0;
 * }
 * 
#include "llama.bindings.h"

int greenc_main(void) {
    bind_library(llama);

    llama_model m = llama_load_model("model.gguf", 0);
    llama_ctx ctx = llama_new_ctx(m, 0);

    return 0;
}
*/
