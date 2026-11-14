/*
 ** debug_events.c - 3 functions.
 ** debug_value - Decode a quoted GDB machine-interface value.
 ** debug_event - Activate the source document for a stop notification.
 ** editor_debug_poll - Consume bounded chunks without blocking the frame.
 ** See also: debug_session.c, editor_debug.h.
 */
#include "editor_debug.h"
#include <stdlib.h>
#include <string.h>

static bool debug_value(const char* record, const char* key, char* output, size_t capacity) {
    const char* value = strstr(record, key);

    if (!value) { return false; }

    value += strlen(key);
    size_t used = 0;

    while (*value && *value != '"' && used + 1 < capacity) {
        if (*value == '\\' && value[1]) { ++value; }
        output[used++] = *value++;
    }

    output[used] = 0;

    return *value == '"';
}

static void debug_event(editor_view* view, editor_document* doc, editor_settings* settings) {
    editor_debug* debug = &view->debug;
    const char* record = debug->record;

    if (!strncmp(record, "^error", 6)) {
        editor_debug_stop(debug);
        doc->status = "Debugger command failed. Check executable, symbols and breakpoint locations.";

        return;
    }

    if (strncmp(record, "*stopped", 8)) { return; }

    if (strstr(record, "reason=\"exited")) {
        editor_debug_stop(debug);
        doc->status = "Debug target exited";

        return;
    }

    debug->paused = true;
    debug->line = 0;
    char path[260], number[24];

    if (!debug_value(record, "fullname=\"", path, sizeof(path)) ||
        !debug_value(record, ",line=\"", number, sizeof(number))) {
        doc->status = "Paused without source information";

        return;
    }

    size_t line = strtoul(number, NULL, 10);

    if (!line || !editor_tabs_open(&view->tabs, doc, settings, path)) { return; }

    strcpy(debug->source, settings->path);
    debug->line = line;
    size_t pos = 0;

    for (size_t current = 1; current < line && pos < doc->length; ++current) {
        pos = editor_line_end(doc, pos);
        if (pos < doc->length) { ++pos; }
    }

    doc->cursor = doc->anchor = pos;
    doc->selecting = view->manual_scroll = false;
    doc->status = "Paused: F5 continue, F10 over, F11 into, Shift+F5 stop";
}

void editor_debug_poll(editor_view* view, editor_document* document, editor_settings* settings) {
    editor_debug* debug = &view->debug;

    if (!debug->process) { return; }

    char chunk[4096];
    size_t count = sys_shell_read(debug->process, chunk, sizeof(chunk));

    for (size_t i = 0; i < count && debug->process; ++i) {
        if (chunk[i] == '\n') {
            debug->record[debug->length] = 0;
            if (!debug->discard) { debug_event(view, document, settings); }
            debug->length = 0;
            debug->discard = false;
        } else if (debug->length + 1 < sizeof(debug->record)) {
            debug->record[debug->length++] = chunk[i];
        } else {
            debug->discard = true;
        }
    }

    if (!count && debug->process && !sys_shell_running(debug->process)) {
        editor_debug_stop(debug);
        document->status = "Debugger exited";
    }
}
