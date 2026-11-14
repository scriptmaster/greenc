#include <stdbool.h>
#pragma once /// single include for driver compile steps

#include "../args.h" /// greenc_args and args_result types
#include "cache_output.h" /// resolve .cache/greenc output path under cwd or HOME
#include "lang.h" /// parse emit lexer types
#include "../libs/sys/exec.h" /// greenc_exec_wait for invoking gcc after emit

#define GREENC_PENDING_SOURCES_MAX 64 /// max input files per run stack size for pending_sources in main.c

static void greenc_pending_release(char** pending, int n) { /// free every buffered source still held
    for (int j = 0; j < n; j++) { /// iterate filled slots only
        release(pending[j]); /// greenc_release each read_file buffer
    }
}

static int greenc_merge_abort( /// fatal path during merge frees everything partial
    greenc_program* combined, /// merged AST root to drop
    char** pending, /// same stack as main pending_sources
    int pending_n, /// count of entries to release
    char* extra_release /// optional buffer not yet pushed e.g. too-many-files case
) {
    if (extra_release) { /// e.g. source read but not admitted to pending
        release(extra_release); /// free that orphan buffer
    }

    greenc_pending_release(pending, pending_n); /// free all admitted sources
    greenc_free(combined); /// drop partial program heap
    return 1; /// signal error to caller
}

static int greenc_emit_abort( /// fatal path after emit started frees emit buffers and program
    greenc_program* combined, /// program to free
    char** pending, /// still must release sources AST may reference until here
    int pending_n, /// pending length
    char* out_c_path, /// transpile path malloced by cache_output
    char* emitted /// generated C text or NULL
) {
    if (emitted) { /// emit string allocated
        greenc_free(emitted); /// drop generated source
    }

    if (out_c_path) { /// output path string
        greenc_free(out_c_path); /// drop path
    }

    greenc_pending_release(pending, pending_n); /// sources safe to free after failed emit
    greenc_free(combined); /// free merged program
    return 1; /// error exit
}

static bool greenc_check_annotated_source(char* source, const char* path);

static int greenc_merge_append_file( /// read one path parse one TU merge into combined
    char* path, /// argv path string owned by args not freed here
    char* source, /// full file text malloced by greenc_read_file
    greenc_keywords* kw, /// shared keyword dict
    greenc_program* combined, /// grows with merged funcs
    char** pending, /// stack to retain source pointers
    int* pending_n, /// in-out count of retained buffers
    char** main_file_out /// set to path of first file that contains main (if not already set)
) {
    if (*pending_n >= GREENC_PENDING_SOURCES_MAX) { /// stack overflow guard
        print("greenc: too many input files"); /// user limit message
        return greenc_merge_abort(combined, pending, *pending_n, source); /// free source and prior state
    }

    pending[*pending_n] = source; /// record buffer so main can defer free until emit
    (*pending_n)++; /// bump retained count

    if (!greenc_check_annotated_source(source, path)) {
        return greenc_merge_abort(combined, pending, *pending_n, NULL);
    }

    greenc_program* one = greenc_parse_file(source, kw); /// build AST with slices into source

    if (!one) { /// parse error
        print("greenc: parse failed"); /// generic message
        print(path); /// which file failed
        return greenc_merge_abort(combined, pending, *pending_n, NULL); /// source already in pending gets released in abort
    }

    /// detect main before merging while we still know which file it came from
    if (main_file_out && !*main_file_out) {
        for (int fi = 0; fi < one->func_count; fi++) {
            greenc_token n = one->funcs[fi]->name;
            if (greenc_str_equals(n.start, n.len, "main")) {
                *main_file_out = path;
                break;
            }
        }
    }

    greenc_program_merge(combined, one); /// steal func pointers into combined free shell of one
    return 0; /// success
}

static int greenc_merge_input_files( /// loop all CLI inputs
    const greenc_args* cli, /// input paths and output exe path
    greenc_keywords* kw, /// lexer keywords
    greenc_program* combined, /// out merged program
    char** pending, /// stack array from main
    int* pending_n, /// in-out retained source count
    char** main_file_out /// set to path of first file that contains main
) {
    int processed = 0; /// inputs successfully merged
    *pending_n = 0; /// reset stack before loop

    for (int i = 0; i < cli->input_files.count; i++) { /// each positional path
        char* path = cli->input_files.items[i]; /// borrowed pointer
        char* source = greenc_read_file(path); /// malloc file contents or NULL

        if (!source) { /// missing or unreadable file
            print("greenc: unable to read input file"); /// warn
            print(path); /// show which
            continue; /// skip this input others may succeed
        }

        if (greenc_merge_append_file(path, source, kw, combined, pending, pending_n, main_file_out) != 0) { /// parse merge or overflow
            return 1; /// abort whole compile
        }

        processed++; /// counted good input
    }

    if (processed == 0) { /// every path skipped or failed read
        print("greenc: no input processed"); /// nothing to emit
        greenc_free(combined); /// drop empty program
        return 1; /// error
    }

    return 0; /// merged at least one file
}

/// create each component of path incrementally so the output dir always exists
static void greenc_ensure_output_dir(const char* output_file) {
    char* dir = greenc_dirname_alloc(output_file);
    if (!dir) return;

    size_t len = greenc_strlen(dir);
    /// nothing to do for current directory
    if (len == 0 || (len == 1 && dir[0] == '.')) {
        greenc_free(dir);
        return;
    }

    /// mkdir -p: create each prefix in turn, ignoring intermediate errors
    char* tmp = (char*)greenc_malloc(len + 1);
    if (tmp) {
        greenc_memcpy(tmp, dir, len + 1);
        for (size_t i = 1; i <= len; i++) {
            if (i == len || tmp[i] == '/' || tmp[i] == '\\') {
                char saved = tmp[i];
                tmp[i] = '\0';
                greenc_mkdir_one(tmp); /// ignore per-component errors (may already exist)
                tmp[i] = saved;
            }
        }
        greenc_free(tmp);
    }

    /// final check: greenc_mkdir_one returns 0 for EEXIST, so -1 means a real failure
    if (greenc_mkdir_one(dir) != 0) {
        print("greenc: warning: could not create output directory");
        print(dir);
    }

    greenc_free(dir);
}

static int greenc_emit_write_output( /// resolve output path emit C write disk free pending
    const greenc_args* cli, /// need output_file for cache path and gcc comment
    greenc_program* combined, /// AST to stringify
    char** pending, /// source buffers to release after emit completes
    int pending_n /// number of retained buffers
) {
    char* gcc_arg = NULL; /// second arg to greenc_emit_program gcc hint string
    char* out_c_path;
    if (cli->emit_c_only) {
        out_c_path = greenc_concat(cli->output_file, "");
        gcc_arg = greenc_concat(cli->output_file, "");
    } else {
        out_c_path = greenc_resolve_emit_c_output_paths(cli->output_file, &gcc_arg);
    } /// .cache/greenc key layout

    if (!out_c_path || !gcc_arg) { /// mkdir or concat failure
        print("greenc: unable to create output path (.cache or HOME/.cache)"); /// explain failure
        greenc_free(gcc_arg); /// may be NULL safe free
        greenc_pending_release(pending, pending_n); /// no emit yet but sources pinned until here
        greenc_free(combined); /// drop program
        return 1; /// error
    }

    char* cwd = greenc_getcwd_alloc(); /// base_path for greenc include resolution
    char* cwd_norm = cwd ? greenc_normalize_cache_path_alloc(cwd) : NULL;
    greenc_free(cwd);

    /// opt flag: user-supplied -O flag, or -Os by default
    const char* opt = cli->opt_flag ? cli->opt_flag : "-Os";

    /// build gcc flags: <opt> -std=c17 -s [+ freestanding set on request]
    /// freestanding also passes -DGREENC_MAIN so start.h activates the startup shim,
    /// and on Windows adds the required linker flags for the custom entry point
#ifdef _WIN32
    const char* base_flags = cli->freestanding
        ? " -std=c17 -s -DGREENC_MAIN -static-libgcc -ffreestanding -fno-builtin -nostdlib -lshell32 -lkernel32 -luser32 -Wl,-egreenc_start"
        : " -std=c17 -s";
#else
    const char* base_flags = cli->freestanding
        ? " -std=c17 -s -DGREENC_MAIN -static-libgcc -ffreestanding -fno-builtin -nostdlib"
        : " -std=c17 -s";
#endif
    char* gcc_flags = greenc_concat(opt, base_flags);
    if (!gcc_flags) {
        print("greenc: out of memory building flags");
        greenc_free(gcc_arg);
        greenc_pending_release(pending, pending_n);
        greenc_free(combined);
        return 1;
    }

    char* emitted = greenc_emit_program(combined, gcc_arg, cli->output_file, cwd_norm, gcc_flags, cli->freestanding); /// format C with header comment

    if (!emitted) { /// OOM or emit bug
        print("greenc: emit failed"); /// message
        greenc_free(gcc_flags);
        greenc_free(gcc_arg); /// drop gcc hint string
        return greenc_emit_abort(combined, pending, pending_n, out_c_path, NULL); /// free path not emitted body
    }

    if (greenc_write_file(out_c_path, emitted) != 0) { /// disk write failed
        print("greenc: unable to write output"); /// message
        print(out_c_path); /// show target path
        greenc_free(gcc_flags);
        greenc_free(gcc_arg); /// drop hint before abort frees rest
        return greenc_emit_abort(combined, pending, pending_n, out_c_path, emitted); /// free emitted and path and pending
    }

    //print("wrote:"); /// success label
    print(out_c_path); /// show final .c path

    greenc_ensure_output_dir(cli->output_file); /// create -o target directory if needed

    /// invoke gcc to compile the generated C file into the target binary
    char* gcc_cmd = cli->emit_c_only ? NULL : greenc_concat("gcc -o ", cli->output_file, " ", gcc_arg, " ", gcc_flags);
    greenc_free(gcc_flags);
    if (gcc_cmd) {
        print(gcc_cmd);
        int gcc_ret = greenc_exec_wait(gcc_cmd);
        greenc_free(gcc_cmd);
        if (gcc_ret != 0) {
            print("greenc: gcc returned non-zero"); /// gcc failed
        }
    }

    greenc_free(emitted); /// generated text no longer needed
    greenc_free(cwd_norm); /// base path used for include resolution
    greenc_free(gcc_arg); /// hint string duplicate of rel path case
    greenc_free(out_c_path); /// path string
    greenc_pending_release(pending, pending_n); /// AST done slices invalid safe to release sources
    return 0; /// success
}
