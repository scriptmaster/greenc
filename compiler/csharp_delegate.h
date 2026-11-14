/*
 ** csharp_delegate.h - 4 functions.
 ** greenc_csharp_input - Recognize C# source, project and solution requests.
 ** greenc_csharp_library - Load only an explicitly configured or compiler-relative native plugin.
 ** greenc_csharp_unload - Release the platform library after all plugin-owned results are destroyed.
 ** greenc_csharp_dispatch - Validate C# arguments and delegate through the versioned ABI.
 ** See also: csharp_build.h, main.c, libs/greencsharp/include/greencsharp.h.
 */
#pragma once
#include "libs/greencsharp/include/greencsharp.h"
#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif
#include "csharp_build.h"
#define GREENC_CSHARP_UNHANDLED (-2)

static bool greenc_csharp_input(const char* input) {
    const char* extension = input;

    for (const char* character = input; *character; ++character) {
        if (*character == '.') { extension = character; }
    }

#ifdef _WIN32
    return !lstrcmpiA(extension, ".cs") || !lstrcmpiA(extension, ".csproj") || !lstrcmpiA(extension, ".sln");
#else
    return greenc_streq(extension, ".cs") || greenc_streq(extension, ".csproj") || greenc_streq(extension, ".sln");
#endif
}

static const greencsharp_api* greenc_csharp_library(void** module) {
    char executable[1024];
    char configured[1024] = {0};
    const char* extension;
#ifdef _WIN32
    DWORD length = GetModuleFileNameA(NULL, executable, sizeof(executable));
    DWORD setting = GetEnvironmentVariableA("GREENC_CSHARP_LIBRARY", configured, sizeof(configured));

    if (!length || length >= sizeof(executable) || setting >= sizeof(configured)) { return NULL; }

    extension = ".dll";
#else
#ifdef __APPLE__
    u32 length = sizeof(executable);

    if (_NSGetExecutablePath(executable, &length)) { return NULL; }

    extension = ".dylib";
#else
    ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);

    if (length <= 0 || (size_t)length >= sizeof(executable) - 1) { return NULL; }

    executable[length] = 0;
    extension = ".so";
#endif
    const char* setting = getenv("GREENC_CSHARP_LIBRARY");

    if (setting) {
        size_t setting_length = greenc_strlen(setting);

        if (setting_length >= sizeof(configured)) { return NULL; }

        greenc_memcpy(configured, setting, setting_length + 1);
    }
#endif
    char* directory = greenc_dirname_alloc(executable);

    if (!directory) { return NULL; }

    char* candidates[] = {
        configured[0] ? greenc_output_exe_abs_path_alloc(configured) : greenc_concat(directory, "/greencsharp", extension),
        configured[0] ? NULL : greenc_concat(directory, "/compiler/libs/greencsharp/bin/greencsharp", extension)
    };
    greencsharp_api_entry entry = NULL;

    for (size_t i = 0; i < 2 && !*module; ++i) {
        if (!candidates[i]) { continue; }

#ifdef _WIN32
        *module = LoadLibraryExA(candidates[i], NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

        if (*module) {
            FARPROC symbol = GetProcAddress((HMODULE)*module, "greencsharp_get_api");
            greenc_memcpy(&entry, &symbol, sizeof(entry));
        }
#else
        *module = dlopen(candidates[i], RTLD_NOW | RTLD_LOCAL);

        if (*module) {
            void* symbol = dlsym(*module, "greencsharp_get_api");
            greenc_memcpy(&entry, &symbol, sizeof(entry));
        }
#endif
    }

    greenc_free(candidates[0]);
    greenc_free(candidates[1]);
    greenc_free(directory);
    const greencsharp_api* api = entry ? entry(GREENC_CSHARP_ABI) : NULL;

    return api && api->size == sizeof(*api) && api->version == GREENC_CSHARP_ABI && api->create && api->result && api->destroy ? api : NULL;
}

static void greenc_csharp_unload(void* module) {
    if (!module) { return; }

#ifdef _WIN32
    FreeLibrary((HMODULE)module);
#else
    dlclose(module);
#endif
}

static int greenc_csharp_dispatch(int argc, char** argv) {
    int start = argc > 1 && greenc_streq(argv[1], "build") ? 2 : 1;
    int input_index = 0;

    for (int i = start; i < argc; ++i) {
        if (greenc_streq(argv[i], "-o") || greenc_streq(argv[i], "--contracts") ||
            greenc_streq(argv[i], "-c") || greenc_streq(argv[i], "--configuration")) {
            ++i;

            continue;
        }

        if (greenc_csharp_input(argv[i])) {
            input_index = i;

            break;
        }
    }

    if (!input_index) { return GREENC_CSHARP_UNHANDLED; }

    const char* output = NULL;
    const char* configuration = "Release";
    u32 mode = GCS_BUILD;
    bool mode_selected = false;

    for (int i = start; i < argc; ++i) {
        if (i == input_index) { continue; }

        bool mode_option = greenc_streq(argv[i], "--emit-c") || greenc_streq(argv[i], "--rebuild") || greenc_streq(argv[i], "--clean");

        if (mode_option && mode_selected) {
            print("greencsharp: select only one of --emit-c, --rebuild or --clean");

            return 1;
        }

        if (mode_option) { mode_selected = true; }

        if (greenc_streq(argv[i], "--emit-c")) {
            mode = GCS_EMIT_C;
        } else if (greenc_streq(argv[i], "--rebuild")) {
            mode = GCS_REBUILD;
        } else if (greenc_streq(argv[i], "--clean")) {
            mode = GCS_CLEAN;
        } else if ((greenc_streq(argv[i], "-c") || greenc_streq(argv[i], "--configuration")) && i + 1 < argc && i + 1 != input_index) {
            configuration = argv[++i];
        } else if (greenc_streq(argv[i], "-o") && !output && i + 1 < argc && i + 1 != input_index) {
            output = argv[++i];
        } else {
            print("greenc: usage: greenc [build] input.cs|input.csproj|input.sln [-o output] [-c Release|Debug] [--emit-c|--rebuild|--clean]");

            return 1;
        }
    }

    if (mode == GCS_CLEAN && output) {
        print("greencsharp: --clean does not accept -o");

        return 1;
    }

    const char* paths[] = {argv[input_index], output};

    for (size_t i = 0; i < 2; ++i) {
        for (const char* character = paths[i]; character && *character; ++character) {
            if (*character == '"' || *character == '\r' || *character == '\n') {
                print("greenc: invalid character in C# input/output path");

                return 1;
            }
        }
    }

    void* module = NULL;
    const greencsharp_api* api = greenc_csharp_library(&module);
    char* input = greenc_output_exe_abs_path_alloc(argv[input_index]);
    char* destination = output ? greenc_output_exe_abs_path_alloc(output) : NULL;
    int status = 1;

    if (!api) {
        print("greenc cannot compile C# files at the moment because the greencsharp dependency is missing or could not be loaded (or has an incompatible ABI).");
    } else if (input && (!output || destination)) {
        status = greenc_csharp_build(api, input, destination, configuration, mode);
    } else {
        print("greencsharp: cannot allocate normalized build paths");
    }

    greenc_csharp_unload(module);
    greenc_free(destination);
    greenc_free(input);

    return status;
}
