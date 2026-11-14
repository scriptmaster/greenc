/*
 ** terminal_sessions.c - 5 functions.
 ** new_session - Start a shell and apply the configured Bash fallback.
 ** editor_terminal_toggle - Show or hide the terminal and start its first session.
 ** editor_terminal_destroy - Release all owned console sessions.
 ** editor_terminal_key - Forward terminal keystrokes to the active shell.
 ** close_session - Confirm and stop the selected shell session.
 ** See also: terminal_internal.h, terminal_sessions.h.
 */
#include "terminal_internal.h"

const char* const profiles[3] = {"cmd.exe", "powershell.exe", "bash.exe"};

bool new_session(editor_terminal* panel, const char* shell) {
    if (panel->count == EDITOR_TERMINAL_LIMIT) {
        panel->notice = "Terminal tab limit reached";

        return false;
    }

    sys_console* console = sys_console_start(shell);
    panel->notice = NULL;
    if (!console && !strcmp(shell, "bash.exe")) {
        shell = "cmd.exe";
        console = sys_console_start(shell);
        panel->notice = "Bash unavailable - opened cmd.exe";
    }

    if (!console) {
        panel->notice = "Cannot start shell";

        return false;
    }

    panel->scroll.reveal_active = true;
    panel->active = panel->count++;
    panel->sessions[panel->active] = (editor_session){.console = console, .name = shell};
    panel->focused = true;

    return true;
}

bool editor_terminal_toggle(editor_terminal* panel, const editor_settings* settings) {
    if (!panel->visible && !panel->count && !new_session(panel, settings->shell)) { return false; }

    panel->visible = !panel->visible;
    panel->focused = panel->visible;
    panel->resizing = panel->choosing = false;

    return true;
}

void editor_terminal_destroy(editor_terminal* panel) {
    for (size_t i = 0; i < panel->count; ++i) {
        sys_console_destroy(panel->sessions[i].console);
    }
}

void editor_terminal_key(editor_terminal* panel, ui_input input) {
    if (!panel->count) { return; }

    sys_console_key key = SYS_CHAR;
    switch (input.key) {
        case UI_KEY_LEFT: key = SYS_LEFT; break;
        case UI_KEY_RIGHT: key = SYS_RIGHT; break;
        case UI_KEY_UP: key = SYS_UP; break;
        case UI_KEY_DOWN: key = SYS_DOWN; break;
        case UI_KEY_HOME: key = SYS_HOME; break;
        case UI_KEY_END: key = SYS_END; break;
        case UI_KEY_DELETE: key = SYS_DELETE; break;
        default: break;
    }

    if (!input.character && key == SYS_CHAR) { return; }
    char character = input.character == '\n' ? '\r' : (char)input.character;
    if (!sys_console_input(panel->sessions[panel->active].console, key, character, input.control, input.shift)) {
        panel->notice = "Shell input unavailable";
    }
}

void close_session(editor_terminal* panel, size_t index) {
    editor_session* session = &panel->sessions[index]; ///read
    if (sys_console_running(session->console) && !ui_confirm(session->name, "Stop this shell and close its tab?")) {
        return;
    }

    sys_console_destroy(session->console);
    --panel->count;
    memmove(session, session + 1, (panel->count - index) * sizeof(*session));
    panel->sessions[panel->count] = (editor_session){0};
    if (!panel->count) {
        panel->visible = panel->focused = false;
        panel->active = 0;
    } else if (index < panel->active || panel->active == panel->count) {
        --panel->active;
    }

    panel->notice = NULL;
    panel->scroll.reveal_active = true;
}
