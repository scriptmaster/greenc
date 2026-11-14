/*
 ** explorer_package_win32.c - 1 function.
 ** explorer_package_set - Run the installed registration script hidden and report deployment failure.
 ** See also: explorer_package.h, package-menu.ps1.
 */
#include "explorer_package.h"
#include <windows.h>
#include <wchar.h>

bool explorer_package_set(bool enabled) {
    wchar_t directory[MAX_PATH], system[MAX_PATH], command[3 * MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, directory, MAX_PATH);
    if (!length || length >= MAX_PATH) { return false; }

    wchar_t* slash = wcsrchr(directory, L'\\');
    if (!slash) { return false; }
    *slash = 0;

    length = GetSystemDirectoryW(system, MAX_PATH);
    if (!length || length >= MAX_PATH) { return false; }
    int count = swprintf(command, 3 * MAX_PATH,
                         L"\"%ls\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile -NonInteractive -ExecutionPolicy RemoteSigned -File \"%ls\\..\\explorer\\package-menu.ps1\" %ls",
                         system, directory, enabled ? L"Install" : L"Remove");
    if (count < 0) { return false; }

    STARTUPINFOW startup = {.cb = sizeof(startup)};
    PROCESS_INFORMATION process = {0};
    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) { return false; }

    DWORD result = 1;
    WaitForSingleObject(process.hProcess, INFINITE);
    GetExitCodeProcess(process.hProcess, &result);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    return result == 0;
}
