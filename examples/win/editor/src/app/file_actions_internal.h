/*
 ** file_actions_internal.h - 4 functions.
 ** target_folder - Resolve the directory for the current tree action.
 ** file_tab - Locate the document tab associated with a filesystem path.
 ** begin_name - Prepare an inline filename entry for a tree command.
 ** refresh_entry - Refresh the active project listing after a filesystem change.
 ** See also: file_targets.c, file_names.c, file_creation.c, editor_file_actions.h.
 */
#pragma once
#include "editor_file_actions.h"
#include "sys/fs/entries.h"
#include "ui/ui_dialog.h"
#include <stdio.h>
#include <string.h>

enum { OPEN_DIRECTORY = 1, NEW_FILE, ROOT_FILE, NEW_FOLDER, DUPLICATE, COPY_AS, RENAME, DELETE_FILE,
       FILE_INFO, PROPERTIES, SHOW_DIRECTORY };

bool target_folder(const editor_tree* tree, char folder[SYS_PATH_CAPACITY]);
size_t file_tab(const editor_tabs* tabs, const char* path);
void begin_name(editor_tree* tree, u16 command);
void refresh_entry(editor_tree* tree, const char* folder, const char* path, bool directory,
                          editor_document* document);
