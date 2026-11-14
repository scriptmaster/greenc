/*
 ** tab_documents.c - 4 functions.
 ** editor_tabs_save - Save the current tab and associate it with its chosen path.
 ** editor_tabs_close - Close a tab while protecting unsaved edits.
 ** editor_tabs_dirty - Check all documents for pending edits.
 ** editor_tabs_step - Select an adjacent document tab.
 ** See also: tab_documents.h, tabs_internal.h.
 */
#include "tabs_internal.h"

void editor_tabs_save(editor_tabs* tabs, editor_document* document, editor_settings* settings, ui_window* window, bool save_as) {
    editor_tab* tab = &tabs->items[tabs->active]; ///read
    if (tab->settings_page) { return; }

    if (!tab->untitled && !save_as) {
        editor_save(document, settings->path);

        return;
    }

    char path[260] = {0};
    if (!tab->untitled) { snprintf(path, sizeof(path), "%s", tab->path); }
    if (!ui_choose_file(window, true, "txt", path)) { return; }
    for (size_t i = 0; i < tabs->count; ++i) {
        if (i != tabs->active && !tabs->items[i].untitled && !_stricmp(path, tabs->items[i].path)) {
            document->status = "File already open in another tab";

            return;
        }
    }

    editor_save(document, path);
    if (!strcmp(document->status, "Saved")) {
        strcpy(tab->path, path);
        tab->untitled = false;
    }
}

void editor_tabs_close(editor_tabs* tabs, size_t index, editor_document* document, editor_settings* settings) {
    if (index >= tabs->count) { return; }

    bool dirty = index == tabs->active ? document->dirty : tabs->items[index].saved_state.dirty;
    if (dirty && !ui_confirm(tabs->items[index].path, "Discard unsaved edits and close this tab?")) { return; }

    tabs->items[tabs->active].saved_state = *document;
    editor_document_release(&tabs->items[index].saved_state);
    --tabs->count;
    memmove(tabs->items + index, tabs->items + index + 1, (tabs->count - index) * sizeof(editor_tab));
    tabs->items[tabs->count] = (editor_tab){0};
    if (!tabs->count) {
        tabs->active = 0;
        editor_tabs_new(tabs, document, settings);
    } else if (index < tabs->active || tabs->active == tabs->count) {
        --tabs->active;
    }

    *document = tabs->items[tabs->active].saved_state;
    tabs->items[tabs->active].saved_state = (editor_document){0};
    settings->path = tabs->items[tabs->active].path;
    document->open_error = NULL;
    tabs->scroll.reveal_active = true;
}

bool editor_tabs_dirty(const editor_tabs* tabs, const editor_document* document) {
    if (document->dirty) { return true; }

    for (size_t i = 0; i < tabs->count; ++i) {
        if (i != tabs->active && tabs->items[i].saved_state.dirty) { return true; }
    }

    return false;
}

void editor_tabs_step(editor_tabs* tabs, editor_document* document, editor_settings* settings, int direction) {
    if (direction < 0 && tabs->active) { select_tab(tabs, tabs->active - 1, document, settings); }
    if (direction > 0 && tabs->active + 1 < tabs->count) { select_tab(tabs, tabs->active + 1, document, settings); }
}
