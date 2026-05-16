//#pragma file: compiler/main.c
#include "libgreenc.h" /// freestanding shims and alloc helpers
#include "lang/lang.h" /// lexer parser emitter keywords
#include "args.h" /// CLI -o and input file list
#include "lang/compile_pipeline.h" /// merge inputs emit write and pending source lifetime

#define VERSION "v1.1.0" /// printed banner version string

static int greenc_compile_and_emit(greenc_args* cli);

int greenc_main(int argc, char** argv, char** envp) {
    (void)envp; /// not used in this build

    print("greenc (" VERSION ")\n"); /// startup banner

    args_result args = args_parse(argc, argv); /// split argv into flags and positionals
    greenc_args cli = greenc_args_parse(args); /// map to greenc -o and file list

    if (cli.error) { /// unknown flag or bad -o value
        return 1; /// exit non-zero
    }

    if (cli.input_files.count == 0) { /// need at least one .c or .gc input
        print("greenc: error: no input files"); /// tell user
        return 1; /// exit non-zero
    }

    return greenc_compile_and_emit(&cli); /// full parse merge emit write path
}

static int greenc_compile_and_emit(greenc_args* cli) {
    greenc_keywords keywords; /// keyword table for lexer ident resolution
    greenc_keywords_init(&keywords); /// load fn let return int into dict

    greenc_program* combined = greenc_program_new(); /// empty TU collecting merged funcs

    if (!combined) { /// malloc failed for program root
        print("greenc: out of memory"); /// user-visible OOM
        return 1; /// abort whole compile
    }

    char* pending_sources[GREENC_PENDING_SOURCES_MAX]; /// stack of raw source buffers still owned until emit finishes
    int pending_sources_n = 0; /// how many slots in pending_sources are filled AST slices point into these

    char* main_file = NULL; /// path of first input file that contains main — set during merge
    if (greenc_merge_input_files(cli, &keywords, combined, pending_sources, &pending_sources_n, &main_file) != 0) { /// parse each file merge AST keep buffers pinned
        return 1; /// parse or merge failed pending and combined freed inside
    }

    /// derive output name from the file that owns main if -o was not given
    char* derived_output = NULL;
    if (!cli->output_file) {
        if (main_file) {
            derived_output = greenc_derive_exe_name_from_path(main_file);
        }
        cli->output_file = derived_output ? derived_output :
#ifdef _WIN32
            "a.exe";
#else
            "a.out";
#endif
    }

    int ret = greenc_emit_write_output(cli, combined, pending_sources, pending_sources_n); /// codegen write .c then release all pending source buffers
    greenc_free(derived_output); /// safe to free — output_file pointer no longer used after emit
    return ret;
}
