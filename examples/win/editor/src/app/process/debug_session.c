/*
 ** debug_session.c - 5 functions.
 ** editor_debug_stop - Release the debugger and its process tree.
 ** debug_send - Send one machine-interface command.
 ** debug_path_command - Quote a Windows path for GDB.
 ** editor_debug_start - Load symbols and register breakpoints from open documents.
 ** editor_debug_key - Dispatch run, step, continue and stop.
 ** See also: editor_debug.h, debug_events.c.
 */
#include "editor_debug.h"
#include "settings_tab.h"
#include "ui/ui_dialog.h"
#include <stdio.h>
#include <string.h>

void editor_debug_stop(editor_debug* debug) {
    sys_shell_destroy(debug->process);
    *debug = (editor_debug){0};
}

static bool debug_send(editor_debug* debug, const char* command) {
    return sys_shell_send(debug->process, command, strlen(command));
}

static bool debug_path_command(editor_debug* debug, const char* command, const char* path, size_t line) {
    char normalized[260], message[640];
    size_t length = strlen(path);

    if (length >= sizeof(normalized) || strpbrk(path, "\"\r\n")) { return false; }

    for (size_t i = 0; i <= length; ++i) {
        normalized[i] = path[i] == '\\' ? '/' : path[i];
    }

    if (line) {
        snprintf(message, sizeof(message), "%s --source \"%s\" --line %zu\n", command, normalized, line);
    } else {
        snprintf(message, sizeof(message), "%s \"%s\"\n", command, normalized);
    }

    return debug_send(debug, message);
}

bool editor_debug_start(editor_view* view, editor_document* active, const char* executable) {
    editor_debug* debug = &view->debug;
    debug->process = sys_shell_create_with_options(view->themes.debug_path, "--nx --quiet --interpreter=mi2");

    if (!debug->process || !debug_path_command(debug, "-file-exec-and-symbols", executable, 0)) { return false; }

    for (size_t i = 0; i < view->tabs.count; ++i) {
        editor_tab* tab = &view->tabs.items[i];
        editor_document* doc = i == view->tabs.active ? active : &tab->saved_state;

        if (tab->untitled || !doc->folded) { continue; }

        size_t line = 1;
        for (size_t pos = 0; pos <= doc->length; ++pos) {
            if ((doc->folded[pos] & 2) && !debug_path_command(debug, "-break-insert", tab->path, line)) { return false; }
            if (pos < doc->length && doc->text[pos] == '\n') { ++line; }
        }
    }

    return debug_send(debug, "-exec-run\n");
}

void editor_debug_key(editor_view* view, editor_document* document, editor_settings* settings,
                      ui_window* window, ui_input input) {
    editor_debug* debug = &view->debug;

    if (input.shift && input.key == UI_KEY_F5) {
        editor_debug_stop(debug);
        document->status = "Debugging stopped";

        return;
    }

    if (!debug->process) {
        if (input.key != UI_KEY_F5) { return; }

        if (!view->themes.debug_path[0]) {
            view->themes.debug = true;
            editor_settings_tab(view, document, settings);

            return;
        }

        char path[260] = {0};

        if (!ui_choose_file(window, false, "exe", path)) { return; }

        if (!editor_debug_start(view, document, path)) {
            editor_debug_stop(debug);
            document->status = "Could not start GDB";
        }

        return;
    }

    if (!debug->paused) { return; }

    const char* command = input.key == UI_KEY_F10 ? "-exec-next\n" :
                          input.key == UI_KEY_F11 ? "-exec-step\n" : "-exec-continue\n";

    if (debug_send(debug, command)) { debug->paused = false; }
}
