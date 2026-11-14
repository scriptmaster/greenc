/*
 ** greencsharp_api.c - 2 functions.
 ** check_api - Exercise version rejection and repeated compilation ownership within one loaded library.
 ** main - Load a native greencsharp module and release it after API checks.
 ** See also: ../libs/greencsharp/include/greencsharp.h, greencsharp.ps1.
 */
#include <stdio.h>
#include <string.h>
#include "greencsharp/include/greencsharp.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static int check_api(greencsharp_api_entry entry, const char* path) {
    if (!entry || entry(0)) { return 1; }

    const greencsharp_api* api = entry(GREENC_CSHARP_ABI);

    if (!api || api->size != sizeof(*api) || api->version != GREENC_CSHARP_ABI ||
        !api->create || !api->result || !api->destroy || api->create(NULL) || api->result(NULL)) { return 1; }

    api->destroy(NULL);
    greencsharp_request request = {.size = sizeof(request), .version = GREENC_CSHARP_ABI + 1, .input_path = path};

    if (api->create(&request)) { return 1; }

    request.version = GREENC_CSHARP_ABI;
    request.size = 0;

    if (api->create(&request)) { return 1; }

    request.size = sizeof(request);

    for (size_t iteration = 0; iteration < 100; ++iteration) {
        void* compilation = api->create(&request);
        const greencsharp_result* result = compilation ? api->result(compilation) : NULL;
        int failed = !result || result->size != sizeof(*result) || result->status ||
                     !result->program || !result->ast || !result->dependencies || !result->diagnostic || result->diagnostic[0];

        if (compilation) { api->destroy(compilation); }
        if (failed) { return 1; }

        request.input_path = "greencsharp_missing_api_fixture_93874.cs";
        compilation = api->create(&request);
        result = compilation ? api->result(compilation) : NULL;
        failed = !result || !result->status || !result->diagnostic || !result->diagnostic[0];

        if (compilation) { api->destroy(compilation); }
        if (failed) { return 1; }

        request.input_path = path;
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) { return 2; }

    greencsharp_api_entry entry = NULL;
#ifdef _WIN32
    HMODULE module = LoadLibraryA(argv[1]);

    if (!module) { return 1; }

    FARPROC symbol = GetProcAddress(module, "greencsharp_get_api");
    memcpy(&entry, &symbol, sizeof(entry));
    int result = check_api(entry, argv[2]);
    FreeLibrary(module);
#else
    void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);

    if (!module) { return 1; }

    void* symbol = dlsym(module, "greencsharp_get_api");
    memcpy(&entry, &symbol, sizeof(entry));
    int result = check_api(entry, argv[2]);
    dlclose(module);
#endif

    return result;
}
