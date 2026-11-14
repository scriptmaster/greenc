/*
 ** workspace_session.h - 2 functions.
 ** editor_workspace_save - Atomically record the tabs present at normal shutdown.
 ** editor_workspace_restore - Rebuild the saved panels and merge the launch location.
 ** See also: workspace_save.c, workspace_restore.c.
 */
#pragma once
#include "editor_view.h"

typedef struct {
    char type;
    char path[SYS_PATH_CAPACITY];
} workspace_tab;

typedef struct {
    char version[8], folder[SYS_PATH_CAPACITY];
    u32 documents, projects, terminals, document, project, terminal;
    u8 files_visible, terminal_visible;
    workspace_tab tabs[EDITOR_TAB_LIMIT];
    char roots[EDITOR_PROJECT_LIMIT][SYS_PATH_CAPACITY];
    char shells[EDITOR_TERMINAL_LIMIT][SYS_PATH_CAPACITY];
} workspace_snapshot;

bool editor_workspace_save(const editor_view* view, const char* path);
bool editor_workspace_restore(editor_view* view, editor_document* doc, editor_settings* settings, const char* path);
