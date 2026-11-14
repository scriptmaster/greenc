/*
 ** document_strip.c - 3 functions.
 ** file_strip - Measure the visible document tabs.
 ** editor_tabs_draw - Paint document labels and unsaved indicators.
 ** editor_tabs_click - Dispatch document tab and add-button clicks.
 ** See also: document_strip.h, tabs_internal.h.
 */
#include "tabs_internal.h"

static editor_tab_strip file_strip(editor_tabs* tabs, ui_renderer* renderer, u32 width) {
    u32 cw, ch;
    ui_renderer_cell_size(renderer, &cw, &ch);

    return editor_strip_layout(&tabs->scroll, tabs->count, tabs->active, width, cw, false);
}

void editor_tabs_draw(editor_tabs* tabs, const editor_document* document, ui_renderer* renderer,
                      u32 width, const editor_settings* settings) {
    editor_tab_strip strip = file_strip(tabs, renderer, width);
    editor_tab_label labels[EDITOR_TAB_LIMIT];
    for (size_t i = strip.first; i < strip.first + strip.visible; ++i) {
        const char* path = tabs->items[i].path;
        const char* slash = strrchr(path, '\\');
        const editor_document* doc = i == tabs->active ? document : &tabs->items[i].saved_state;
        labels[i] = (editor_tab_label){tabs->items[i].settings_page ? "Settings" : slash ? slash + 1 : path, doc->dirty};
    }

    editor_strip_draw(&strip, renderer, settings, 0, 0, settings->layout.header_height, tabs->active, labels);
}

void editor_tabs_click(editor_tabs* tabs, editor_document* document, editor_settings* settings,
                       ui_renderer* renderer, u32 width, i32 x) {
    editor_tab_strip strip = file_strip(tabs, renderer, width);
    size_t index;
    editor_tab_action action = editor_strip_click(&strip, &tabs->scroll, x, &index);
    if (action == TAB_SELECT) {
        select_tab(tabs, index, document, settings);
    } else if (action == TAB_CLOSE) {
        editor_tabs_close(tabs, index, document, settings);
    } else if (action == TAB_ADD && !editor_tabs_new(tabs, document, settings)) {
        document->status = "File tab limit reached";
    }
}
