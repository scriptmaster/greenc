/*
 ** active_file.c - 1 function.
 ** follow_active_file - Keep the appropriate project active for the current document.
 ** See also: editor_session.h, main_internal.h.
 */
#include "main_internal.h"

void follow_active_file(editor_view* view, editor_document* document, const editor_settings* settings) {
    if (view->tabs.items[view->tabs.active].untitled) {
        view->projects.followed[0] = 0;
    } else if (!editor_project_follow(&view->projects, &view->tree, settings->path)) {
        document->status = "Could not list active file folder";
    }
}
