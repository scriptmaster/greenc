#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "shell.h"

struct sys_shell {
    HANDLE process, job, input, output;
};

sys_shell* sys_shell_create(const char* executable) {
    return sys_shell_create_with_options(executable, "");
}

sys_shell* sys_shell_create_with_options(const char* executable, const char* options) {
    sys_shell* shell = calloc(1, sizeof(*shell));
    if (!shell) { return NULL; }

    SECURITY_ATTRIBUTES inherit = {sizeof(inherit), NULL, TRUE};
    HANDLE reader = NULL, writer = NULL;
    STARTUPINFOA startup = {.cb = sizeof(startup), .dwFlags = STARTF_USESTDHANDLES};
    PROCESS_INFORMATION process = {0};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {0};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    shell->job = CreateJobObjectA(NULL, NULL);

    if (!shell->job || !SetInformationJobObject(shell->job, JobObjectExtendedLimitInformation,
        &limits, sizeof(limits)) || !CreatePipe(&reader, &shell->input, &inherit, 0) ||
        !CreatePipe(&shell->output, &writer, &inherit, 0) ||
        !SetHandleInformation(shell->input, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(shell->output, HANDLE_FLAG_INHERIT, 0)) {
        goto failure;
    }

    startup.hStdInput = reader;
    startup.hStdOutput = startup.hStdError = writer;
    char resolved[MAX_PATH];
    DWORD size = SearchPathA(NULL, executable, ".exe", MAX_PATH, resolved, NULL);
    if (!size || size >= MAX_PATH) { goto failure; }

    char command[1024];
    int length = snprintf(command, sizeof(command), "\"%s\" %s", resolved, options);

    if (length < 0 || (size_t)length >= sizeof(command)) { goto failure; }
    if (!CreateProcessA(resolved, command, NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                        NULL, NULL, &startup, &process)) { goto failure; }

    shell->process = process.hProcess;
    if (!AssignProcessToJobObject(shell->job, process.hProcess) ||
        ResumeThread(process.hThread) == (DWORD)-1) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);

        goto failure;
    }

    CloseHandle(process.hThread);
    CloseHandle(reader);
    CloseHandle(writer);

    return shell;

failure:
    if (reader) { CloseHandle(reader); }
    if (writer) { CloseHandle(writer); }
    sys_shell_destroy(shell);

    return NULL;
}

bool sys_shell_running(sys_shell* shell) {
    return WaitForSingleObject(shell->process, 0) == WAIT_TIMEOUT;
}

bool sys_shell_send(sys_shell* shell, const char* line, size_t length) {
    DWORD written;
    return length <= MAXDWORD && WriteFile(shell->input, line, (DWORD)length, &written, NULL) &&
           written == length;
}

size_t sys_shell_read(sys_shell* shell, char* output, size_t capacity) {
    DWORD available, received;
    if (!PeekNamedPipe(shell->output, NULL, 0, NULL, &available, NULL) || !available) { return 0; }
    if (available > capacity) { available = (DWORD)capacity; }

    return ReadFile(shell->output, output, available, &received, NULL) ? received : 0;
}

void sys_shell_destroy(sys_shell* shell) {
    if (!shell) { return; }
    if (shell->job) { CloseHandle(shell->job); }
    if (shell->process) { CloseHandle(shell->process); }
    if (shell->input) { CloseHandle(shell->input); }
    if (shell->output) { CloseHandle(shell->output); }
    free(shell);
}
