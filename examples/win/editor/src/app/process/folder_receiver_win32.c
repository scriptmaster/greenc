/*
 ** folder_receiver_win32.c - 4 functions.
 ** receive_folder - Validate a folder request and activate its Files tab.
 ** editor_forward_folder - Deliver a local folder to an existing workspace.
 ** editor_listen_folders - Create a message window on the editor UI thread.
 ** editor_stop_folders - Release the window and borrowed workspace.
 ** See also: folder_receiver.h, editor_projects.c.
 */
#include "folder_receiver.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

static const char receiver_class[] = "GreenC.Editor.FolderReceiver.1";
static HWND receiver;
static editor_view* workspace;

static LRESULT CALLBACK receive_folder(HWND window, UINT message, WPARAM sender, LPARAM data) {
    if (message != WM_COPYDATA) { return DefWindowProcA(window, message, sender, data); }

    const COPYDATASTRUCT* request = (const COPYDATASTRUCT*)data;
    if (!workspace || !request || request->dwData != 1 || !request->lpData ||
        !request->cbData || request->cbData > SYS_PATH_CAPACITY) { return FALSE; }

    const char* path = request->lpData;
    if (memchr(path, 0, request->cbData) != path + request->cbData - 1 ||
        !editor_project_open(&workspace->projects, &workspace->tree, path)) { return FALSE; }

    workspace->tree.visible = workspace->tree.focused = true;
    workspace->terminal.focused = false;


    return TRUE;
}

bool editor_forward_folder(const char* folder) {
    HWND existing = FindWindowExA(HWND_MESSAGE, NULL, receiver_class, NULL);
    char full[SYS_PATH_CAPACITY];

    if (!existing || !_fullpath(full, folder, sizeof(full))) { return false; }


    COPYDATASTRUCT request = {.dwData = 1, .cbData = (DWORD)strlen(full) + 1, .lpData = full};
    DWORD_PTR accepted = 0;

    return SendMessageTimeoutA(existing, WM_COPYDATA, 0, (LPARAM)&request, SMTO_ABORTIFHUNG, 3000, &accepted) && accepted;
}

void editor_listen_folders(editor_view* view) {
    WNDCLASSA registration = {.lpfnWndProc = receive_folder, .hInstance = GetModuleHandleA(NULL), .lpszClassName = receiver_class};
    if (!RegisterClassA(&registration) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) { return; }

    workspace = view;

    receiver = CreateWindowExA(0, receiver_class, "", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, registration.hInstance, NULL);
}

void editor_stop_folders(void) {
    if (receiver) { DestroyWindow(receiver); }
    receiver = NULL;
    workspace = NULL;
}
