/*
 ** document_tabs.h - 2 functions.
 ** editor_tabs_open - Activate or load a named document with a specific open error.
 ** editor_tabs_new - Add an empty untitled document.
 ** See also: document_tabs.c, editor_tabs_types.h.
 */
#pragma once
#include "editor_tabs_types.h"

bool editor_tabs_open(editor_tabs* tabs, editor_document* document, editor_settings* settings, const char* path);
bool editor_tabs_new(editor_tabs* tabs, editor_document* document, editor_settings* settings);
