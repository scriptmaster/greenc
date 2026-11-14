/*
 ** theme_layout.c - 4 functions.
 ** visible_rows - Count theme fields that fit in the settings pane.
 ** scroll_fields - Clamp the first visible theme field.
 ** control_bounds - Compute a settings control's hit and draw rectangle.
 ** control_visible - Determine whether a settings control is displayed.
 ** See also: themes_internal.h.
 */
#include "themes_internal.h"

const char* const theme_groups[6] = {"Editor colors", "Buttons", "Links", "Debugger", "Syntax / General", "Syntax / Languages"};
const i32 group_edges[7] = {0, 8, 15, 21, 23, 28, FIELD_COUNT};

i32 visible_rows(u32 height) {
    i32 rows = height > ROW_TOP ? (i32)((height - ROW_TOP) / ROW_HEIGHT) : 0;

    return rows > FIELD_COUNT ? FIELD_COUNT : rows;
}

void scroll_fields(editor_themes* page, i32 rows, i32 step) {
    page->first += step;
    i32 end = group_edges[page->group + 1];

    if (page->first > end - rows) { page->first = end - rows; }
    if (page->first < group_edges[page->group]) { page->first = group_edges[page->group]; }
}

editor_bounds control_bounds(i32 id, i32 first) {
    switch (id) {
        case CATEGORY: return (editor_bounds){20, 38, 112, 28};
        case THEMES: return (editor_bounds){30, 158, 102, 28};
        case DEBUGGER: return (editor_bounds){30, 98, 102, 28};
        case KEYS: return (editor_bounds){30, 128, 112, 28};
        case GROUP: return (editor_bounds){LEFT + 120, 112, 190, 26};
        case GENERAL: return (editor_bounds){30, 68, 102, 28};
        case INSTALL_ADMIN: return (editor_bounds){LEFT, 142, 320, 30};
        case REMOVE_ADMIN: return (editor_bounds){LEFT, 180, 320, 30};
        case EXPLORER: return (editor_bounds){LEFT, 54, 320, 30};
        case DONE: return (editor_bounds){12, 198, 100, 28};
        case BROWSE: return (editor_bounds){LEFT, 62, 102, 30};
        case SAVE: return (editor_bounds){LEFT + 110, 62, 102, 30};
        case PREVIOUS: return (editor_bounds){LEFT, 112, 48, 26};
        case NEXT: return (editor_bounds){LEFT + 54, 112, 56, 26};
    }

    i32 field = (id - FIRST_FIELD) / 2;
    i32 y = ROW_TOP + (field - first) * ROW_HEIGHT;
    if (fields[field].maximum) {
        return (editor_bounds){LEFT + 208 + ((id - FIRST_FIELD) % 2) * 34, y, 30, 26};
    }

    return (editor_bounds){LEFT, y, 280, 26};
}

bool control_visible(const editor_themes* page, i32 id, i32 rows) {
    if (id == DONE) { return false; }
    if (id == EXPLORER || id == INSTALL_ADMIN || id == REMOVE_ADMIN) { return page->general && !page->debug; }
    if (id == GENERAL || id == THEMES || id == DEBUGGER || id == KEYS) { return page->expanded; }
    if ((page->general || page->keybindings) && !page->debug) { return id == CATEGORY; }
    if (page->debug) { return id == CATEGORY || id == DONE || id == BROWSE; }
    if (id == PREVIOUS) { return page->first > group_edges[page->group]; }
    if (id == NEXT) { return page->first + rows < group_edges[page->group + 1]; }
    if (id < FIRST_FIELD) { return true; }

    i32 field = (id - FIRST_FIELD) / 2;
    return field >= page->first && field < group_edges[page->group + 1] && field < page->first + rows &&
           (fields[field].maximum || (id - FIRST_FIELD) % 2 == 0);
}
