/*
 ** document_navigation.c - 2 functions.
 ** editor_move_between_lines - Move vertically while retaining the preferred column when possible.
 ** editor_move_cursor - Apply a document navigation command.
 ** See also: document_navigation.h, editor_document.h.
 */
#include "editor_document.h"

static void editor_move_between_lines(editor_document* document, bool down) {
    size_t start = editor_line_start(document, document->cursor);
    size_t column = document->cursor - start;
    size_t destination;
    size_t end;

    if (down) {
        end = editor_line_end(document, document->cursor);
        if (end == document->length) {
            if (document->selecting) { document->cursor = end; }

            return;
        }

        destination = end + 1;
        end = editor_line_end(document, destination);
    } else {
        if (!start) {
            return;
        }

        destination = editor_line_start(document, start - 1);
        end = start - 1;
    }

    size_t line_length = end - destination;
    document->cursor = destination + (column < line_length ? column : line_length);
}

void editor_move_cursor(editor_document* document, editor_movement movement) {
    switch (movement) {
        case EDITOR_MOVE_LEFT:
            if (document->cursor) {
                --document->cursor;
            }

            break;
        case EDITOR_MOVE_RIGHT:
            if (document->cursor < document->length) {
                ++document->cursor;
            }

            break;
        case EDITOR_MOVE_UP:
            editor_move_between_lines(document, false);

            break;
        case EDITOR_MOVE_DOWN:
            editor_move_between_lines(document, true);

            break;
        case EDITOR_MOVE_LINE_START:
            document->cursor = editor_line_start(document, document->cursor);

            break;
        case EDITOR_MOVE_LINE_END:
            document->cursor = editor_line_end(document, document->cursor);

            break;
    }
}
