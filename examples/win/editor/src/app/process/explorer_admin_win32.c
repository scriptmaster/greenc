/*
 ** explorer_admin_win32.c - 3 functions.
 ** explorer_menu_set - Update the per-user classic Explorer commands.
 ** explorer_machine_set - Install or remove the modern package and machine-wide classic verbs.
 ** explorer_request_admin - Ask Windows to elevate only the registration helper.
 ** See also: explorer_menu_win32.c, explorer_package_win32.c.
 */
#include "explorer_menu.h"
#include "explorer_package.h"
#include <windows.h>
#include <shellapi.h>

extern bool explorer_classic_set(HKEY hive, bool enabled);

bool explorer_menu_set(bool enabled) {
    return explorer_classic_set(HKEY_CURRENT_USER, enabled);
}

bool explorer_machine_set(bool enabled) {
    return explorer_package_set(enabled) && explorer_classic_set(HKEY_LOCAL_MACHINE, enabled);
}

bool explorer_request_admin(bool enabled) {
    wchar_t exe[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (!length || length >= MAX_PATH) { return false; }

    SHELLEXECUTEINFOW request = {.cbSize = sizeof(request), .fMask = SEE_MASK_NOCLOSEPROCESS,
        .lpVerb = L"runas", .lpFile = exe, .lpParameters = enabled ? L"--install-machine-menu" : L"--remove-machine-menu",
        .nShow = SW_HIDE};
    if (!ShellExecuteExW(&request)) { return false; }

    DWORD result = 1;
    WaitForSingleObject(request.hProcess, INFINITE);
    GetExitCodeProcess(request.hProcess, &result);
    CloseHandle(request.hProcess);

    return result == 0;
}
