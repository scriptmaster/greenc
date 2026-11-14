/*
 ** tabs_internal.h - 1 function.
 ** select_tab - Transfer document ownership when switching active tabs.
 ** See also: document_tabs.c, editor_tabs.h.
 */
#pragma once
#include "editor_tabs.h"
#include "ui/ui_dialog.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void select_tab(editor_tabs* tabs, size_t index, editor_document* document, editor_settings* settings);
