#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "console.h"
#include <stdlib.h>
#include <string.h>

struct sys_console {
    DWORD pid;
    HANDLE process, job, input, output;
    u32 columns, rows;
};

static sys_console* attached;

static void detach(void) {
    if (!attached) { return; }

    CloseHandle(attached->input);
    CloseHandle(attached->output);
    FreeConsole();
    attached = NULL;
}

static bool attach(sys_console* console) {
    if (attached == console) { return true; }

    detach();
    if (!AttachConsole(console->pid)) { return false; }

    SetConsoleCtrlHandler(NULL, TRUE);
    DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    console->input = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, share, NULL, OPEN_EXISTING, 0, NULL);
    console->output = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, share, NULL, OPEN_EXISTING, 0, NULL);
    attached = console;
    if (console->input == INVALID_HANDLE_VALUE || console->output == INVALID_HANDLE_VALUE) {
        detach();

        return false;
    }

    return true;
}

sys_console* sys_console_start(const char* executable) {
    char resolved[MAX_PATH];
    /* Resolve profile names from the configured search path, excluding the implicit working directory. */
    const char* search = getenv("PATH");
    if (!search) { return NULL; }
    DWORD length = SearchPathA(search, executable, ".exe", MAX_PATH, resolved, NULL);
    if (!length || length >= MAX_PATH) { return NULL; }

    sys_console* console = calloc(1, sizeof(*console));
    if (!console) { return NULL; }

    STARTUPINFOA startup = {.cb = sizeof(startup), .dwFlags = STARTF_USESHOWWINDOW, .wShowWindow = SW_HIDE};
    PROCESS_INFORMATION process = {0};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {0};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    console->job = CreateJobObjectA(NULL, NULL);
    if (!console->job || !SetInformationJobObject(console->job, JobObjectExtendedLimitInformation,
        &limits, sizeof(limits)) || !CreateProcessA(resolved, NULL, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, &startup, &process)) {
        sys_console_destroy(console);

        return NULL;
    }

    console->pid = process.dwProcessId;
    console->process = process.hProcess;
    bool started = AssignProcessToJobObject(console->job, console->process) &&
                   ResumeThread(process.hThread) != (DWORD)-1;
    CloseHandle(process.hThread);
    if (!started) {
        TerminateProcess(console->process, 1);
        sys_console_destroy(console);

        return NULL;
    }

    return console;
}

bool sys_console_running(sys_console* console) {
    return console && WaitForSingleObject(console->process, 0) == WAIT_TIMEOUT;
}

bool sys_console_viewport(sys_console* console, u32 columns, u32 rows, sys_console_view* view) {
    if (!columns || !rows || !attach(console)) { return false; }

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(console->output, &info)) { return false; }

    if (columns != console->columns || rows != console->rows) {
        COORD maximum = GetLargestConsoleWindowSize(console->output);
        SHORT w = (SHORT)(columns < (u32)maximum.X ? columns : (u32)maximum.X);
        SHORT h = (SHORT)(rows < (u32)maximum.Y ? rows : (u32)maximum.Y);
        SHORT top = info.dwCursorPosition.Y >= h ? info.dwCursorPosition.Y - h + 1 : 0;
        SMALL_RECT rect = {0, top, w - 1, top + h - 1};
        COORD size = {w, info.dwSize.Y > h ? info.dwSize.Y : h};
        if (w > info.dwSize.X || h > info.dwSize.Y) {
            COORD grow = {w > info.dwSize.X ? w : info.dwSize.X, size.Y};
            SetConsoleScreenBufferSize(console->output, grow);
        }

        if (SetConsoleWindowInfo(console->output, TRUE, &rect) && SetConsoleScreenBufferSize(console->output, size)) {
            console->columns = columns;
            console->rows = rows;
        }

        if (!GetConsoleScreenBufferInfo(console->output, &info)) { return false; }
    }

    CONSOLE_CURSOR_INFO cursor;
    view->left = info.srWindow.Left;
    view->top = info.srWindow.Top;
    view->cursor_x = info.dwCursorPosition.X - view->left;
    view->cursor_y = info.dwCursorPosition.Y - view->top;
    view->cursor_visible = GetConsoleCursorInfo(console->output, &cursor) && cursor.bVisible;

    return true;
}

bool sys_console_row(sys_console* console, i32 x, i32 y, char* row, u32 width) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!attach(console) || !GetConsoleScreenBufferInfo(console->output, &info) ||
        x >= info.dwSize.X || y >= info.dwSize.Y) { return false; }

    DWORD count = 0;
    if (width > (u32)(info.dwSize.X - x)) { width = info.dwSize.X - x; }
    if (!ReadConsoleOutputCharacterA(console->output, row, width, (COORD){x, y}, &count)) { return false; }

    while (count && row[count - 1] == ' ') { --count; }
    row[count] = 0;

    return true;
}

bool sys_console_input(sys_console* console, sys_console_key key, char character, bool control, bool shift) {
    if (!attach(console)) { return false; }
    if (control && character == 3) { return GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0); }

    static const WORD special[] = {0, VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_HOME, VK_END, VK_DELETE, VK_ESCAPE};
    SHORT mapped = VkKeyScanA(character);
    WORD vk = key == SYS_CHAR ? (WORD)(mapped & 255) : special[key];
    INPUT_RECORD events[2] = {0};
    events[0].EventType = KEY_EVENT;
    KEY_EVENT_RECORD* press = &events[0].Event.KeyEvent;
    press->bKeyDown = TRUE;
    press->wRepeatCount = 1;
    press->wVirtualKeyCode = vk;
    press->wVirtualScanCode = (WORD)MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    press->uChar.UnicodeChar = (unsigned char)character;
    press->dwControlKeyState = (control ? LEFT_CTRL_PRESSED : 0) | (shift ? SHIFT_PRESSED : 0);
    events[1] = events[0];
    events[1].Event.KeyEvent.bKeyDown = FALSE;
    DWORD written;

    return WriteConsoleInputW(console->input, events, 2, &written) && written == 2;
}

void sys_console_destroy(sys_console* console) {
    if (!console) { return; }
    if (console == attached) { detach(); }
    if (console->job) { CloseHandle(console->job); }
    if (console->process) { CloseHandle(console->process); }

    free(console);
}

bool sys_console_snapshot(sys_console* console, u64* fingerprint) {
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!attach(console) || !GetConsoleScreenBufferInfo(console->output, &info)) { return false; }

    u64 hash = (u64)(u16)info.dwCursorPosition.Y << 16 | (u16)info.dwCursorPosition.X;
    wchar_t cells[512];
    for (SHORT row = info.srWindow.Top; row <= info.srWindow.Bottom; ++row) {
        for (int x = info.srWindow.Left; x <= info.srWindow.Right;) {
            DWORD count = 0;
            DWORD width = info.srWindow.Right - x + 1;
            if (width > 512) { width = 512; }

            if (!ReadConsoleOutputCharacterW(console->output, cells, width, (COORD){(SHORT)x, row}, &count) || !count) {
                return false;
            }

            for (DWORD i = 0; i < count; ++i) { hash = (hash ^ cells[i]) * 1099511628211ULL; }
            x += (int)count;
        }
    }

    *fingerprint = hash ? hash : 1;

    return true;
}
