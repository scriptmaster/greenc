/*
 ** workspace_save.c - 1 function.
 ** editor_workspace_save - Save tab identities and active panels without writing document contents.
 ** See also: workspace_session.h, workspace_restore.c.
 */
#include "workspace_session.h"
#include "sys/fs/write_atomic.h"
#include <stdlib.h>
#include <string.h>

bool editor_workspace_save(const editor_view* view, const char* path) {
    workspace_snapshot saved = {.version = "GCtabs1"};
    if (!_fullpath(saved.folder, ".", sizeof(saved.folder))) { return false; }

    saved.documents = (u32)view->tabs.count;
    saved.projects = (u32)view->projects.count;
    saved.terminals = (u32)view->terminal.count;
    saved.document = (u32)view->tabs.active;
    saved.project = (u32)view->projects.active;
    saved.terminal = (u32)view->terminal.active;
    saved.files_visible = view->tree.visible;
    saved.terminal_visible = view->terminal.visible;

    for (size_t i = 0; i < saved.documents; ++i) {
        const editor_tab* tab = &view->tabs.items[i];
        saved.tabs[i].type = tab->settings_json ? 'J' : tab->settings_page ? 'S' : tab->untitled ? 'U' : 'F';
        strcpy(saved.tabs[i].path, tab->path);
    }

    for (size_t i = 0; i < saved.projects; ++i) { strcpy(saved.roots[i], view->projects.roots[i]); }

    for (size_t i = 0; i < saved.terminals; ++i) {
        if (strlen(view->terminal.sessions[i].name) >= SYS_PATH_CAPACITY) { return false; }
        strcpy(saved.shells[i], view->terminal.sessions[i].name);
    }

    return sys_file_write_atomic(path, (const char*)&saved, sizeof(saved));
}
