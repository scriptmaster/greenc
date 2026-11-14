/*
 ** process_stats_win32.c - 2 functions.
 ** ticks - Convert a Windows file time to a 64-bit counter.
 ** sys_process_sample - Sample the current process without owning a process handle.
 ** See also: process_stats.h.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include "process_stats.h"

static u64 ticks(FILETIME t) { return ((u64)t.dwHighDateTime << 32) | t.dwLowDateTime; }

bool sys_process_sample(sys_process_stats* sample) {
    HANDLE process = GetCurrentProcess();
    /* MinGW lacks the EX2 typedef. Its ABI extends EX with these two fields. */
    struct {
        PROCESS_MEMORY_COUNTERS_EX base;
        SIZE_T private_working_set;
        ULONG64 shared_commit;
    } memory = {.base.cb = sizeof(memory), .private_working_set = (SIZE_T)-1};
    FILETIME created, exited, kernel, user;
    if (!GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&memory, sizeof(memory)) ||
        !GetProcessTimes(process, &created, &exited, &kernel, &user)) { return false; }

    SYSTEM_INFO system;
    GetSystemInfo(&system);
    *sample = (sys_process_stats){memory.private_working_set == (SIZE_T)-1 ? 0 : memory.private_working_set, ticks(kernel) + ticks(user), system.dwNumberOfProcessors};

    return true;
}
