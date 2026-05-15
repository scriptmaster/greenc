#ifdef _WIN32

#include <windows.h>
#include "sys_write.h"
#include "builtin/strings.h"

int greenc_main(
    int argc,
    char** argv,
    char** envp
);


// -------------------------------------
// tiny argv parser
// -------------------------------------
static int greenc_parse_cmdline(
    char* cmd,
    char** argv,
    int max_args
) {

    int argc = 0;

    while (*cmd && argc < max_args) {

        // -------------------------
        // skip whitespace
        // -------------------------
        while (*cmd == ' ' || *cmd == '\t') {
            cmd++;
        }

        if (!*cmd) {
            break;
        }

        // -------------------------
        // quoted string
        // -------------------------
        if (*cmd == '"') {

            cmd++;

            argv[argc++] = cmd;

            while (*cmd && *cmd != '"') {
                cmd++;
            }

            if (*cmd) {
                *cmd = 0;
                cmd++;
            }

            continue;
        }

        // -------------------------
        // normal arg
        // -------------------------
        argv[argc++] = cmd;

        while (
            *cmd &&
            *cmd != ' ' &&
            *cmd != '\t'
        ) {
            cmd++;
        }

        if (*cmd) {
            *cmd = 0;
            cmd++;
        }
    }

    return argc;
}


// -------------------------------------
// entrypoint
// -------------------------------------
__attribute__((noreturn))
void greenc_start() {

    static char* argv[64];

    // -----------------------------
    // copy cmdline to writable buf
    // -----------------------------
    char* raw = GetCommandLineA();

    static char cmdline[8192];

    int i = 0;

    while (raw[i] && i < 8191) {
        cmdline[i] = raw[i];
        i++;
    }

    cmdline[i] = 0;

    // -----------------------------
    // parse
    // -----------------------------
    int argc =
        greenc_parse_cmdline(
            cmdline,
            argv,
            64
        );

    // sys_write("debug argc=", 11);
    // buf32 s = tostring(argc);
    // sys_write(s.str, greenc_strlen(s.str));
    // sys_write("\n", 1);

    // -----------------------------
    // call main
    // -----------------------------
    int ret =
        greenc_main(
            argc,
            argv,
            NULL
        );

    ExitProcess(ret);
}

#endif
