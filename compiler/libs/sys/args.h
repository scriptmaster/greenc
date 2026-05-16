#pragma once

#include "sys_write.h"

typedef struct {
    string_list positional;

    string_list options;
    string_list option_values;

    int error;
} args_result;


// -------------------------------------
// generic arg parser
// -------------------------------------
static inline args_result args_parse(
    int argc,
    char** argv
) {

    args_result res = {0};
    
    greenc_string_list_init(&res.positional);
    greenc_string_list_init(&res.options);
    greenc_string_list_init(&res.option_values);

    for (int i = 1; i < argc; i++) {

        char* arg = argv[i];

        // -----------------------------
        // option
        // -----------------------------
        if (arg[0] == '-') {

            greenc_string_list_push(
                &res.options,
                arg
            );

            // -------------------------
            // consume ONE optional value
            // -------------------------
            if (
                i + 1 < argc &&
                argv[i + 1][0] != '-'
            ) {

                greenc_string_list_push(
                    &res.option_values,
                    argv[++i]
                );
            }

            else {

                greenc_string_list_push(
                    &res.option_values,
                    NULL
                );
            }

            continue;
        }

        // -----------------------------
        // positional
        // -----------------------------
        greenc_string_list_push(
            &res.positional,
            arg
        );
    }

    return res;
}
