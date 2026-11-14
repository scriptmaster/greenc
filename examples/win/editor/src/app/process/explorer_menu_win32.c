/*
 ** explorer_menu_win32.c - 4 functions.
 ** menu_paths - Format quoted Explorer command and embedded icon references.
 ** put_string - Write one complete registry string.
 ** explorer_menu_enabled - Check the installed command against this executable.
 ** explorer_classic_set - Update Explorer and notify the shell when registration changes.
 ** See also: explorer_menu.h, editor.rc.
 */
#include "explorer_menu.h"
#include "explorer_package.h"
#include <windows.h>
#include <shlobj.h>
#include <wchar.h>

#define BACKGROUND_VERB L"Software\\Classes\\Directory\\Background\\shell\\GreenC.Editor"
#define FOLDER_VERB L"Software\\Classes\\Directory\\shell\\GreenC.Editor"

static bool menu_paths(wchar_t command[MAX_PATH + 32], wchar_t icon[MAX_PATH + 8]) {
    wchar_t exe[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, exe, MAX_PATH);

    if (!length || length >= MAX_PATH) { return false; }

    return swprintf(command, MAX_PATH + 32, L"\"%ls\" --folder \"%%1\"", exe) > 0 &&
           swprintf(icon, MAX_PATH + 8, L"\"%ls\",0", exe) > 0;
}

static bool put_string(HKEY hive, const wchar_t* key, const wchar_t* name, const wchar_t* value) {
    DWORD bytes = (DWORD)((wcslen(value) + 1) * sizeof(*value));

    return RegSetKeyValueW(hive, key, name, REG_SZ, value, bytes) == ERROR_SUCCESS;
}

bool explorer_menu_enabled(void) {
    wchar_t command[MAX_PATH + 32], icon[MAX_PATH + 8], installed[MAX_PATH + 32];
    DWORD bytes = sizeof(installed);

    bool folder = menu_paths(command, icon) &&
           RegGetValueW(HKEY_CURRENT_USER, FOLDER_VERB L"\\command", NULL, RRF_RT_REG_SZ, NULL, installed, &bytes) == ERROR_SUCCESS &&
           !_wcsicmp(command, installed);
    if (!folder) { return false; }

    command[wcslen(command) - 2] = L'V';
    bytes = sizeof(installed);

    return RegGetValueW(HKEY_CURRENT_USER, BACKGROUND_VERB L"\\command", NULL, RRF_RT_REG_SZ, NULL, installed, &bytes) == ERROR_SUCCESS &&
           !_wcsicmp(command, installed);
}

bool explorer_classic_set(HKEY hive, bool enabled) {
    if (enabled) {
        wchar_t command[MAX_PATH + 32], icon[MAX_PATH + 8];

        if (!menu_paths(command, icon) || !put_string(hive, FOLDER_VERB, NULL, L"Open in GreenC Editor") ||
            !put_string(hive, FOLDER_VERB, L"Icon", icon) || !put_string(hive, FOLDER_VERB L"\\command", NULL, command)) { return false; }
        command[wcslen(command) - 2] = L'V';
        if (!put_string(hive, BACKGROUND_VERB, NULL, L"Open in GreenC Editor") ||
            !put_string(hive, BACKGROUND_VERB, L"Icon", icon) || !put_string(hive, BACKGROUND_VERB L"\\command", NULL, command)) { return false; }
    } else {
        LSTATUS background = RegDeleteTreeW(hive, BACKGROUND_VERB);
        if (background != ERROR_SUCCESS && background != ERROR_FILE_NOT_FOUND) { return false; }
        LSTATUS status = RegDeleteTreeW(hive, FOLDER_VERB);
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) { return false; }
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    return true;
}
