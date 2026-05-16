#pragma once

#include "libgreenc.h"

typedef struct {
    string_list input_files;
    char* output_file;
    char* opt_flag;   /// -Os (default) / -O0 / -O1 / -O2 / -O3 / -Oz / any -O... passed through to gcc
    int strip;        /// -s: strip symbols (passed to gcc as -s)
    int freestanding; /// -gc / -nolibc / -greenc / -nostdlib / -ffreestanding / -fno-builtin / -static-libgcc
    int nogc;         /// -nogc / -libc: opt out of freestanding mode (overrides default)
    int error;
} greenc_args;


// -------------------------------------
// convert generic args -> greenc args
// -------------------------------------

static inline greenc_args greenc_args_parse(
    args_result args
) {

    greenc_args cli = {0};

    // -----------------------------
    // positional -> input files
    // -----------------------------
    cli.input_files =
        args.positional;

    // -----------------------------
    // parse options
    // -----------------------------
    for (int i = 0; i < args.options.count; i++) {

    	char* opt = args.options.items[i];
        char* val = args.option_values.items[i];

        if (greenc_streq(opt, "-o")) {
            if (!val) {
                print("greenc: missing filename after -o");
                cli.error = 1;
                return cli;
            }

            cli.output_file = val;
            continue;
        }

        // -------------------------
        // optimization
        // -------------------------
        // -------------------------
        // optimization: any -O... flag is stored verbatim and passed to gcc
        // -------------------------
        if (greenc_string_startswith(opt, "-O")) {
            cli.opt_flag = opt;
            continue;
        }

        // -------------------------
        // strip symbols
        // -------------------------
        if (greenc_streq(opt, "-s")) {
            cli.strip = 1;
            continue;
        }

        // -------------------------
        // opt-out of freestanding/gc mode
        // -------------------------
        if (greenc_streq(opt, "-nogc") || greenc_streq(opt, "-libc")) {
            cli.nogc = 1;
            continue;
        }

        // -------------------------
        // freestanding / no-libc modes
        //   -gc | -nolibc | -greenc  : high-level aliases
        //   -nostdlib                : expands to full freestanding set
        //   -ffreestanding           : expands to full freestanding set
        //   -fno-builtin             : expands to full freestanding set
        //   -static-libgcc           : expands to full freestanding set
        // -------------------------
        if (greenc_streq(opt, "-gc")          ||
            greenc_streq(opt, "-nolibc")      ||
            greenc_streq(opt, "-greenc")      ||
            greenc_streq(opt, "-nostdlib")    ||
            greenc_streq(opt, "-ffreestanding") ||
            greenc_streq(opt, "-fno-builtin") ||
            greenc_streq(opt, "-static-libgcc")
        ) {
            cli.freestanding = 1;
            continue;
        }

        // -------------------------
        // unknown option
        // -------------------------
        print("greenc: unknown option");
        print(opt);

        cli.error = 1;

        return cli;
    }

    // -----------------------------
    // defaults
    // -----------------------------

    /// gc/freestanding is the default — opt out with -nogc or -libc
    if (!cli.nogc) {
        cli.freestanding = 1;
    }

    if (!cli.output_file) {

#ifdef _WIN32
        cli.output_file = "a.exe";
#else
        cli.output_file = "a.out";
#endif
    }

    return cli;
}
