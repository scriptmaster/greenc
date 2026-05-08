#pragma once

// 1. language coloring (greenc): expands to attributed comments
#define own     /*@own*/
#define borrow  /*@borrow*/
#define edit    /*@edit*/

#define defer    /*@defer*/
#define option   /*@option*/
#define fn       (void*)
#define proc     void
// #define default  /*@default*/
#define discard  /*@discard*/

// 2. function coloring: expands to attributed comments too
#define async     /*@async*/
#define await     /*@await*/
#define chan      /*@chan*/
// async and await impl from libco

// 3. expands to: extern void* greenc_use_llama
#define use(name) extern void* greenc_use_##name

// 4. import llama; expands to: extern greenc_import llama;
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
