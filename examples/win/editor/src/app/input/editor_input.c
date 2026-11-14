/*
 ** editor_input.c - 2 functions.
 ** editor_input - Apply document keyboard shortcuts and text input.
 ** editor_clipboard - Copy, cut or paste the current selection.
 ** See also: editor_input.h.
 */
#include "editor_input.h"

void editor_input(editor_document* document, ui_input input) {
    if (input.control && input.key == UI_KEY_A) {
        document->anchor = 0;
        document->cursor = document->length;
        document->selecting = true;

        return;
    }

    if (input.key >= UI_KEY_LEFT && input.key <= UI_KEY_END) {
        if (input.shift && !document->selecting) {
            document->anchor = document->cursor;
        }

        if (!input.shift && document->selecting && document->anchor != document->cursor &&
            (input.key == UI_KEY_LEFT || input.key == UI_KEY_RIGHT)) {
            size_t start, end;
            editor_selection(document, &start, &end);
            document->cursor = input.key == UI_KEY_LEFT ? start : end;
            document->selecting = false;

            return;
        }

        document->selecting = input.shift;
        if (input.control && (input.key == UI_KEY_HOME || input.key == UI_KEY_END)) {
            document->cursor = input.key == UI_KEY_HOME ? 0 : document->length;

            return;
        }
    }

    switch (input.key) {
        case UI_KEY_LEFT:
            editor_move_cursor(document, EDITOR_MOVE_LEFT);

            return;

        case UI_KEY_RIGHT:
            editor_move_cursor(document, EDITOR_MOVE_RIGHT);

            return;

        case UI_KEY_UP:
            editor_move_cursor(document, EDITOR_MOVE_UP);

            return;

        case UI_KEY_DOWN:
            editor_move_cursor(document, EDITOR_MOVE_DOWN);

            return;

        case UI_KEY_HOME:
            editor_move_cursor(document, EDITOR_MOVE_LINE_START);

            return;

        case UI_KEY_END:
            editor_move_cursor(document, EDITOR_MOVE_LINE_END);

            return;

        case UI_KEY_DELETE:
            editor_delete_character(document, false);

            return;

        default:
            break;
    }

    if (input.control) {
        return;
    }

    if (input.character == '\b') {
        editor_delete_character(document, true);
    } else {
        editor_insert_character(document, input.character);
    }
}

void editor_clipboard(editor_document* document, ui_window* window, ui_key key) {
    size_t start, end;
    editor_selection(document, &start, &end);
    if (key != UI_KEY_V) {
        if (start == end) { return; }

        if (!ui_clipboard_write(window, document->text + start, end - start)) {
            document->status = "Clipboard unavailable";
        } else if (key == UI_KEY_X) {
            editor_replace_selection(document, "", 0);
        }

        return;
    }

    char input[EDITOR_CLIPBOARD_CAPACITY * 2], output[EDITOR_CLIPBOARD_CAPACITY];
    if (!ui_clipboard_read(window, input, sizeof(input))) {
        document->status = "Paste requires ASCII text within the document limit";

        return;
    }

    size_t count = 0;
    for (size_t i = 0; input[i]; ++i) {
        char c = input[i];
        if (c == '\r') {
            if (input[i + 1] == '\n') { ++i; }
            c = '\n';
        }

        size_t copies = c == '\t' ? 4 : 1;
        if (count + copies >= sizeof(output) ||
            (c != '\n' && c != '\t' && (c < 32 || c > 126))) {
            document->status = "Paste exceeds document limits or contains unsupported text";

            return;
        }

        while (copies--) { output[count++] = c == '\t' ? ' ' : c; }
    }

    if (count) { editor_replace_selection(document, output, count); }
}
