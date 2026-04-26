#pragma once

// 1. expands to comments
#define own     /*@own*/
#define borrow  /*@borrow*/
#define edit    /*@edit*/

// 2. expands to: extern void* greenc_use_llama
#define use(name) extern void* greenc_use_##name

// 3. import llama; expands to: extern greenc_import llama;
typedef struct {
    const char* path;
    const char* name;
    const char* version;
    int flags;
} greenc_module_meta;

// module
typedef struct {
    greenc_module_meta meta;
    void* handle;
} greenc_module;

#define import extern greenc_module
