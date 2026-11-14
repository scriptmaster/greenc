/*
 ** tab_activity.c - 1 function.
 ** editor_tab_activity - Sample live resources and acknowledge focused tabs.
 ** See also: tab_activity.h, editor_loop.c.
 */
#include "tab_activity.h"

void editor_tab_activity(editor_view* view) {
    u64 now = ui_time_ms();
    bool sample = now - view->projects.sampled >= 3000;

    if (sample) { view->projects.sampled = now; }

    for (size_t i = 0; i < view->projects.count; ++i) {
        bool focused = i == view->projects.active && view->tree.visible && view->tree.focused;

        if ((sample || (focused && view->projects.changed[i])) && sys_folder_watch_changed(view->projects.watches[i])) {
            view->projects.changed[i] = true;
        }

        if (sample && focused && view->projects.changed[i] && editor_tree_root(&view->tree, view->projects.roots[i])) {
            view->projects.changed[i] = false;
        }
    }

    for (size_t i = 0; i < view->terminal.count; ++i) {
        editor_session* session = &view->terminal.sessions[i];
        u64 next;

        if (sample && sys_console_snapshot(session->console, &next)) {
            if (session->snapshot && session->snapshot != next) { session->changed = true; }
            session->snapshot = next;
        }

        if (i == view->terminal.active && view->terminal.focused) { session->changed = false; }
    }
}
