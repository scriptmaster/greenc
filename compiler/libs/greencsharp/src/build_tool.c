/* Native compiler/archiver processes receive arguments directly, never through a shell. */
#include "internal.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
extern char** environ;
#endif

static bool gcs_tool_argument(gcs_compilation* c, gcs_buffer* command, const char* argument) {
    gcs_append(c, command, "\"");
    size_t slashes = 0;

    for (const char* p = argument; ; ++p) {
        if (*p == '\\') { ++slashes; continue; }

        size_t count = (*p == '"' || !*p) ? slashes * 2 : slashes;

        for (size_t i = 0; i < count; ++i) { gcs_append(c, command, "\\"); }

        slashes = 0;

        if (!*p) { break; }
        if (*p == '"') { gcs_append(c, command, "\\"); }

        gcs_append(c, command, "%c", *p);
    }

    return gcs_append(c, command, "\" ");
}

bool gcs_response_argument(gcs_compilation* c, gcs_buffer* output, const char* argument) {
    gcs_append(c, output, "\"");

    for (const char* p = argument; *p; ++p) {
        if (*p == '\r' || *p == '\n') {
            gcs_error(c, argument, 0, "newline in native tool argument");

            return false;
        }

        if (*p == '"' || *p == '\\') { gcs_append(c, output, "\\"); }

        gcs_append(c, output, "%c", *p);
    }

    return gcs_append(c, output, "\"\n");
}

bool gcs_tool(gcs_compilation* c, const char* const* arguments, const char* capture) {
    gcs_buffer command = {0};

    for (size_t i = 0; arguments[i]; ++i) { gcs_tool_argument(c, &command, arguments[i]); }

    if (!capture) {
        printf("%s\n", command.data ? command.data : "");
        fflush(stdout);
    }

    bool success = false;
#ifdef _WIN32
    SECURITY_ATTRIBUTES security = {sizeof(security), NULL, TRUE};
    HANDLE output = capture ? CreateFileA(capture, GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) : INVALID_HANDLE_VALUE;
    STARTUPINFOA startup = {.cb = sizeof(startup)};
    PROCESS_INFORMATION process = {0};
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = capture ? output : GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    if (command.data && (!capture || output != INVALID_HANDLE_VALUE) &&
        CreateProcessA(NULL, command.data, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
        DWORD code = 1;

        if (WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &code)) {
            success = code == 0;
        }

        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }

    if (output != INVALID_HANDLE_VALUE) { CloseHandle(output); }
#else
    posix_spawn_file_actions_t actions;
    bool initialized = posix_spawn_file_actions_init(&actions) == 0;
    bool redirected = !capture || (initialized && posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, capture, O_WRONLY | O_CREAT | O_TRUNC, 0666) == 0);
    pid_t child;

    if (initialized && redirected && posix_spawnp(&child, arguments[0], &actions, NULL, (char* const*)arguments, environ) == 0) {
        int status;
        pid_t waited;

        do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);

        success = waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    if (initialized) { posix_spawn_file_actions_destroy(&actions); }
#endif
    free(command.data);

    if (!success) { gcs_error(c, arguments[0], 0, "native build tool failed or could not start"); }

    return success;
}
