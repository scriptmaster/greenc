/*
 ** editor_view.h - 4 functions.
 ** editor_scroll_rows - Apply shared scrollbar input to a row offset.
 ** editor_scroll_input - Apply document wheel, track and thumb input.
 ** editor_mouse_select - Map document pointer gestures to selection and folding.
 ** editor_draw - Compose and present a complete editor frame.
 ** See also: document_scroll.c, document_pointer.c, editor_frame.c, editor_document.h, editor_settings.h, editor_terminal.h, editor_tree.h, editor_projects.h.
 */
#pragma once
#include "editor_document.h"
#include "debug_types.h"
#include "editor_settings.h"
#include "editor_terminal.h"
#include "editor_tree.h"
#include "editor_projects.h"
#include "editor_themes.h"
#include "editor_tabs.h"
#include "editor_status.h"
#include "editor_scrollbar.h"
#include "editor_control.h"

typedef struct {
    struct editor_line_entry* row_map;
    size_t row_capacity, mapped_rows, longest_line;
    bool horizontal_drag;
    u64 mapped_revision;
    editor_tabs tabs;
    editor_debug debug;
    editor_status_bar status_bar;
    editor_control_input hex_button;
    editor_transition hex_transition;
    editor_bounds hex_action;
    editor_scrollbar hex_bar;
    u32 hex_columns, hex_rows, hex_cell_width, hex_cell_height;
    i32 hex_top;
    editor_terminal terminal;
    editor_tree tree;
    editor_projects projects;
    editor_themes themes;
    bool dragging, scrolling, manual_scroll;
    bool auto_scroll;
    i32 scroll_anchor_y;
    u64 scroll_tick;
    i32 thumb_offset, wheel_remainder;
    size_t first_line;
    u64 caret_visible_since_ms;
    size_t first_column;
} editor_view;

bool editor_scroll_rows(editor_view* view, editor_scrollbar bar, u32 width, size_t* first_line, ui_input input);
bool editor_scroll_input(editor_view* view, editor_document* document, ui_renderer* renderer,
                         ui_window* window, const editor_settings* settings, ui_input input);
void editor_mouse_select(editor_view* view, editor_document* document, ui_renderer* renderer,
                         ui_window* window, const editor_settings* settings, ui_input input);
bool editor_draw(ui_renderer* renderer, ui_window* window, editor_document* document,
                 editor_view* view, const editor_settings* settings);
