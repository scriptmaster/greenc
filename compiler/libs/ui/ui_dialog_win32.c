#include "ui_window_win32.h"
#include "ui_dialog.h"
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>

bool ui_choose_file(ui_window* window, bool save, const char* extension, char path[260]) {
    char selected[260] = {0}, filter[128] = {0};
    int length = snprintf(filter, sizeof(filter), "%s files%c*.%s%cAll files%c*.*", extension, 0, extension, 0, 0);
    if (length < 0 || length >= (int)sizeof(filter) - 1) { return false; }
    if (save) { snprintf(selected, sizeof(selected), "%s", path); }

    OPENFILENAMEA dialog = {.lStructSize = sizeof(dialog), .hwndOwner = window->handle,
        .lpstrFilter = filter, .lpstrFile = selected, .nMaxFile = sizeof(selected),
        .lpstrDefExt = extension, .Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
            (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST)};
    if (!(save ? GetSaveFileNameA(&dialog) : GetOpenFileNameA(&dialog))) { return false; }

    strcpy(path, selected);

    return true;
}

u16 ui_popup_menu(ui_window* window, i32 x, i32 y, const ui_popup_item* items, size_t count) {
    HMENU menu = CreatePopupMenu();
    if (!menu) { return 0; }

    POINT pos = {x, y};
    bool ready = ClientToScreen(window->handle, &pos);
    for (size_t i = 0; ready && i < count; ++i) {
        ready = AppendMenuA(menu, MF_STRING, items[i].command, items[i].label);
    }

    u16 selected = ready ? (u16)TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                                              pos.x, pos.y, 0, window->handle, NULL) : 0;
    DestroyMenu(menu);

    return selected;
}

bool ui_choose_folder(ui_window* window, char path[260]) {
    HRESULT initialized = OleInitialize(NULL);
    BROWSEINFOA options = {.hwndOwner = window->handle, .lpszTitle = "Open folder",
                           .ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE};
    PIDLIST_ABSOLUTE folder = SHBrowseForFolderA(&options);
    char selected[MAX_PATH];
    bool ok = folder && SHGetPathFromIDListA(folder, selected);
    if (ok) { strcpy(path, selected); }

    CoTaskMemFree(folder);
    if (SUCCEEDED(initialized)) { OleUninitialize(); }

    return ok;
}

static bool ui_shell_action(ui_window* window, const char* verb, const char* path) {
    HRESULT initialized = OleInitialize(NULL);
    SHELLEXECUTEINFOA action = {.cbSize = sizeof(action), .fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_NOASYNC,
                                .hwnd = window->handle, .lpVerb = verb, .lpFile = path, .nShow = SW_SHOWNORMAL};
    bool ok = ShellExecuteExA(&action);
    if (SUCCEEDED(initialized)) { OleUninitialize(); }

    return ok;
}

bool ui_show_properties(ui_window* window, const char* path) { return ui_shell_action(window, "properties", path); }
bool ui_open_folder(ui_window* window, const char* path) { return ui_shell_action(window, "open", path); }

void ui_show_info(ui_window* window, const char* title, const char* message) {
    MessageBoxA(window->handle, message, title, MB_OK | MB_ICONINFORMATION);
}

bool ui_choose_color(ui_window* window, u32* rgb) {
    static COLORREF custom[16];
    CHOOSECOLORA dialog = {.lStructSize = sizeof(dialog), .hwndOwner = window->handle,
        .lpCustColors = custom, .Flags = CC_FULLOPEN | CC_RGBINIT,
        .rgbResult = RGB(*rgb >> 16, *rgb >> 8, *rgb)};
    if (!ChooseColorA(&dialog)) { return false; }

    *rgb = (GetRValue(dialog.rgbResult) << 16) | (GetGValue(dialog.rgbResult) << 8) | GetBValue(dialog.rgbResult);

    return true;
}
