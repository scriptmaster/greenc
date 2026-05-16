//#pragma file "compiler/libs/sys/exec.h"
#pragma once

#include "builtin/strings1.h"
#include "builtin/alloc.h"

// -------------------------------------------------------------------------
// greenc_exec_wait(cmd)
// Spawns `cmd` as a subprocess and waits for it to finish.
// Returns the process exit code, or -1 on spawn failure.
// -------------------------------------------------------------------------

#ifdef _WIN32

#include <windows.h>

static int greenc_exec_wait(const char* cmd) {
    if (!cmd) return -1;

    size_t len = greenc_strlen(cmd);
    char* buf = (char*)greenc_malloc(len + 1); /// CreateProcess needs mutable buffer
    if (!buf) return -1;

    greenc_memcpy(buf, cmd, len);
    buf[len] = '\0';

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(
            NULL,   /// let Windows resolve the exe via PATH
            buf,    /// mutable command line
            NULL, NULL,
            FALSE,  /// don't inherit handles
            0, NULL, NULL,
            &si, &pi
    )) {
        greenc_free(buf);
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    greenc_free(buf);
    return (int)code;
}

#else

/// Linux stub — freestanding fork+execve via syscalls not yet implemented
static int greenc_exec_wait(const char* cmd) {
    (void)cmd;
    return -1;
}

#endif
