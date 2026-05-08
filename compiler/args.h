#pragma once

#include "libgreenc.h"

typedef enum {
    OPT_UNSPECIFIED = -1,
    OPT_O0 = 0,
    OPT_O1 = 1,
    OPT_O2 = 2,
    OPT_O3 = 3
} greenc_opt_level;

typedef struct {
    string_list input_files;
    char* output_file;
    greenc_opt_level optimize_level;
    int error;
} greenc_args;


// -------------------------------------
// convert generic args -> greenc args
// -------------------------------------

static inline greenc_args greenc_args_parse(
    args_result args
) {

    greenc_args cli = {0};

    cli.optimize_level =
        OPT_UNSPECIFIED;

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
        if (greenc_streq(opt, "-O0")) {
            cli.optimize_level = OPT_O0;
            continue;
        }

        if (greenc_streq(opt, "-O1")) {
            cli.optimize_level = OPT_O1;
            continue;
        }

        if (greenc_streq(opt, "-O2")) {
            cli.optimize_level = OPT_O2;
            continue;
        }

        if (greenc_streq(opt, "-O3")) {
            cli.optimize_level = OPT_O3;
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
    if (!cli.output_file) {

#ifdef _WIN32
        cli.output_file = "a.exe";
#else
        cli.output_file = "a.out";
#endif
    }

    return cli;
}
