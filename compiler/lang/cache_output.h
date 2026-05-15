#pragma once

#include "driver.h"
#include "../libs/sys/fs.h"

#ifdef _WIN32
#include <windows.h>
#endif

static int greenc_path_is_absolute(const char* p) {
    if (!p || !p[0]) {
        return 0;
    }

#ifdef _WIN32
    if (p[0] == '/' || p[0] == '\\') {
        return 1;
    }

    if (greenc_strlen(p) >= 2 && p[1] == ':') {
        return 1;
    }

    return 0;
#else
    return p[0] == '/';
#endif
}

static char* greenc_output_exe_abs_path_alloc(const char* output_exe) {
    if (greenc_path_is_absolute(output_exe)) {
        return greenc_concat(output_exe, "");
    }

    char* cwd = greenc_getcwd_alloc();

    if (!cwd) {
        return NULL;
    }

    char* out = greenc_concat(cwd, "/", output_exe);

    greenc_free(cwd);
    return out;
}

static char* greenc_home_dir_alloc(void) {
#ifdef _WIN32
    char buf[768];
    DWORD n = GetEnvironmentVariableA("HOME", buf, sizeof(buf) - 1);

    if (n == 0 || n >= sizeof(buf) - 1) {
        n = GetEnvironmentVariableA("USERPROFILE", buf, sizeof(buf) - 1);
    }

    if (n == 0 || n >= sizeof(buf) - 1) {
        return NULL;
    }

    buf[n] = '\0';
    return greenc_concat(buf, "");
#else
    return greenc_getcwd_alloc();
#endif
}

static char* greenc_sanitize_cache_key(const char* path) {
    size_t n = greenc_strlen(path);
    char* out = (char*)greenc_malloc(n + 1);

    if (!out) {
        return NULL;
    }

    size_t j = 0;

    for (size_t i = 0; i < n; i++) {
        char c = path[i];

        if (c == '/' || c == '\\' || c == ':') {
            out[j++] = '_';
        } else {
            out[j++] = c;
        }
    }

    out[j] = '\0';
    return out;
}

static char* greenc_emit_c_basename_alloc(const char* output_exe) {
    char* tmp = greenc_transpile_c_path_for_output(output_exe);

    if (!tmp) {
        return NULL;
    }

    const char* bn = greenc_path_basename(tmp);
    char* copy = greenc_concat(bn, "");

    greenc_free(tmp);
    return copy;
}

static void greenc_free_four(char* a, char* b, char* c, char* d) {
    greenc_free(a);
    greenc_free(b);
    greenc_free(c);
    greenc_free(d);
}

static char* greenc_try_cache_greenc_tree(
    const char* base,
    const char* abs_output_exe,
    const char* cbase,
    char** gcc_arg_out,
    int emit_relative_gcc_hint
) {
    char* key = greenc_sanitize_cache_key(abs_output_exe);

    if (!key) {
        return NULL;
    }

    char* d1 = greenc_concat(base, "/.cache");
    char* d2 = greenc_concat(base, "/.cache/greenc");
    char* d3 = greenc_concat(base, "/.cache/greenc/", key);

    if (!d1 || !d2 || !d3) {
        greenc_free_four(d1, d2, d3, key);
        return NULL;
    }

    if (greenc_mkdir_one(d1) != 0 || greenc_mkdir_one(d2) != 0 || greenc_mkdir_one(d3) != 0) {
        greenc_free_four(d1, d2, d3, key);
        return NULL;
    }

    char* full = greenc_concat(d3, "/", cbase);
    char* gcc = NULL;

    if (emit_relative_gcc_hint) {
        gcc = greenc_concat(".cache/greenc/", key, "/", cbase);
    } else {
        gcc = greenc_concat(full, "");
    }

    greenc_free_four(d1, d2, d3, key);

    if (!full || !gcc) {
        greenc_free(full);
        greenc_free(gcc);
        return NULL;
    }

    *gcc_arg_out = gcc;
    return full;
}

static char* greenc_resolve_emit_c_output_paths(
    const char* output_exe,
    char** gcc_arg_out
) {
    *gcc_arg_out = NULL;

    char* cbase = greenc_emit_c_basename_alloc(output_exe);

    if (!cbase) {
        return NULL;
    }

    char* abs_out = greenc_output_exe_abs_path_alloc(output_exe);

    if (!abs_out) {
        greenc_free(cbase);
        return NULL;
    }

    char* cwd = greenc_getcwd_alloc();

    if (!cwd) {
        greenc_free(cbase);
        greenc_free(abs_out);
        return NULL;
    }

    char* full = greenc_try_cache_greenc_tree(cwd, abs_out, cbase, gcc_arg_out, 1);

    if (!full) {
        char* home = greenc_home_dir_alloc();

        if (home) {
            full = greenc_try_cache_greenc_tree(home, abs_out, cbase, gcc_arg_out, 0);
            greenc_free(home);
        }
    }

    greenc_free(cwd);
    greenc_free(cbase);
    greenc_free(abs_out);
    return full;
}
