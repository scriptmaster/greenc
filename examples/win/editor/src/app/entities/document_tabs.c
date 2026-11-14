/*
 ** document_tabs.c - 3 functions.
 ** select_tab - Transfer document ownership when switching active tabs.
 ** editor_tabs_open - Activate or load a named document with a specific open error.
 ** editor_tabs_new - Add an empty untitled document.
 ** See also: document_tabs.h, tabs_internal.h.
 */
#include "tabs_internal.h"

void select_tab(editor_tabs* tabs, size_t index, editor_document* document, editor_settings* settings) {
    if (index != tabs->active) {
        tabs->items[tabs->active].saved_state = *document;
        *document = tabs->items[index].saved_state;
        tabs->items[index].saved_state = (editor_document){0};
        tabs->active = index;
    }

    document->open_error = NULL;
    tabs->scroll.reveal_active = true;
    settings->path = tabs->items[index].path;
}

bool editor_tabs_open(editor_tabs* tabs, editor_document* document, editor_settings* settings, const char* path) {
    char full[SYS_PATH_CAPACITY];
    if (!_fullpath(full, path, sizeof(full))) {
        document->open_error = "Invalid path or path exceeds Windows path limit";
        return false;
    }

    for (size_t i = 0; i < tabs->count; ++i) {
        if (!_stricmp(full, tabs->items[i].path)) {
            select_tab(tabs, i, document, settings);

            return true;
        }
    }

    if (tabs->count == EDITOR_TAB_LIMIT) {
        document->open_error = "All 24 document tabs are in use";
        return false;
    }

    editor_tab* next = &tabs->items[tabs->count]; ///read
    if (tabs->count && !editor_load_existing(&next->saved_state, full)) {
        document->open_error = next->saved_state.open_error;
        return false;
    }

    strcpy(next->path, full);
    size_t index = tabs->count++;
    select_tab(tabs, index, document, settings);

    return true;
}

bool editor_tabs_new(editor_tabs* tabs, editor_document* document, editor_settings* settings) {
    if (tabs->count == EDITOR_TAB_LIMIT) {
        document->open_error = "All 24 document tabs are in use";
        return false;
    }

    size_t index = tabs->count++;
    editor_tab* tab = &tabs->items[index]; ///read
    tab->untitled = true;
    tab->saved_state = (editor_document){.status = "New file"};
    snprintf(tab->path, sizeof(tab->path), "Untitled %zu", ++tabs->untitled_serial);
    select_tab(tabs, index, document, settings);

    return true;
}
