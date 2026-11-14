/*
 ** scroll_rows.c - 1 function.
 ** editor_scroll_rows - Apply wheel, track and captured thumb input to a row offset.
 ** See also: editor_view.h, editor_scrollbar.h.
 */
#include "editor_view.h"

bool editor_scroll_rows(editor_view* view, editor_scrollbar bar, u32 width, size_t* first_line, ui_input input) {
    if (!bar.maximum) {
        view->scrolling = false;

        return false;
    }

    if (!view->scrolling) {
        if (input.x < 0 || input.x >= (i32)width || input.y < bar.top || input.y >= bar.top + bar.height) {
            return false;
        }

        if (input.mouse == UI_MOUSE_WHEEL) {
            view->wheel_remainder += input.wheel_delta;
            i64 first = (i64)*first_line - 3 * (view->wheel_remainder / 120);
            view->wheel_remainder %= 120;
            *first_line = first < 0 ? 0 : (size_t)first > bar.maximum ? bar.maximum : (size_t)first;
            view->manual_scroll = true;

            return true;
        }

        if (input.mouse != UI_MOUSE_DOWN || input.x < bar.x || input.x >= bar.x + 10) { return false; }

        i32 offset = input.y - editor_thumb_top(bar, *first_line);
        view->thumb_offset = offset >= 0 && offset < bar.thumb ? offset : bar.thumb / 2;
        view->scrolling = true;
    }

    view->manual_scroll = true;
    *first_line = editor_scroll_position(bar, input.y - view->thumb_offset);
    if (input.mouse == UI_MOUSE_UP) { view->scrolling = false; }

    return true;
}
