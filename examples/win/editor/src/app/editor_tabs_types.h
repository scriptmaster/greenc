/*
 ** editor_tabs_types.h - 0 functions.
 ** Collect the related module declarations.
 ** See also: editor_document.h, editor_tab_strip.h, document_strip.h, document_tabs.h, editor_tabs.h, tab_documents.h.
 */
#pragma once
#include "editor_document.h"
#include "editor_tab_strip.h"
#include "sys/fs/directory.h"

enum { EDITOR_TAB_LIMIT = 24 };
typedef struct {
    char path[SYS_PATH_CAPACITY];
    bool untitled, settings_page, settings_json;
    editor_document saved_state;
} editor_tab;

typedef struct {
    editor_tab items[EDITOR_TAB_LIMIT];
    size_t count, active, untitled_serial;
    editor_tab_scroll scroll;
} editor_tabs;
