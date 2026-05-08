//#pragma file: compiler/main.c
#include "libgreenc.h"
#include "lang/lang.h"
#include "args.h"

#define VERSION "v1.0.5"


static int filter_c_files(const char* name) {
    return string_endswith(name, ".c") ||
           string_endswith(name, ".h") ||
           string_endswith(name, ".gc");
}

int greenc_main(
    int argc,
    char** argv,
    char** envp
) {

    print("greenc (" VERSION ")\n");

    // ---------------------------------
    // parse args
    // ---------------------------------
    args_result args = args_parse(argc, argv);
    greenc_args cli = greenc_args_parse(args);

    if (cli.error) {
        return 1;
    }

    // ---------------------------------
    // require input files
    // ---------------------------------
    if (cli.input_files.count == 0) {
        print("greenc: error: no input files");
        return 1;
    }

    // ---------------------------------
    // debug print
    // ---------------------------------
    print("output:");
    print(cli.output_file);

    print("optimization:");
    print(str(cli.optimize_level));

    // ---------------------------------
    // shared keyword table
    // ---------------------------------
    greenc_keywords keywords;

    greenc_keywords_init(
        &keywords
    );

    
    char* cwd = greenc_getcwd_alloc();
    if (cwd) {
        print("cwd:");
        print(cwd);
    }
    
    // ---------------------------------
    // parse input files
    // ---------------------------------
    for (
        int i = 0;
        i < cli.input_files.count;
        i++
    ) {

        char* path =
            cli.input_files.items[i];

        print("input:");
        print(path);

        // -----------------------------
        // read source
        // -----------------------------
        char* source =
            greenc_read_file(path);

        if (!source) {
            print("greenc: unable to read input file");
            print(path);
            continue;
        }

        // -----------------------------
        // lexer
        // -----------------------------
        greenc_lexer lexer = {
            source,
            0
        };


        while (1) {

            greenc_token tok =
                greenc_next_token(
                    &lexer,
                    &keywords
                );

            if (
                tok.kind ==
                TOK_EOF
            ) {
                break;
            }

            sys_write("Token=", 6);

            sys_write(
                tok.start,
                tok.len
            );

            sys_write("\n", 1);
        }


        // -----------------------------
        // cleanup
        // -----------------------------
        release(source);
    }


    return 0;
}
