/*
 ** editor_loop.c - 1 function.
 ** editor_run - Run the editor event loop and release its session resources.
 ** See also: editor_session.h, main_internal.h.
 */
#include "main_internal.h"
#include "debug_preferences.h"
#include "editor_debug.h"
#include "tab_activity.h"
#include "workspace_session.h"
#include "folder_receiver.h"

i32 editor_run(ui_window* window, ui_renderer* renderer, editor_document* document,
                       editor_settings* settings) {
    editor_view view = {.caret_visible_since_ms = ui_time_ms()}; ///stack
    char session_path[SYS_PATH_CAPACITY];
    bool session_available = preference_path(session_path, "GreenC-editor-tabs.session");
    editor_debug_load(view.themes.debug_path);
    char preference[260];
    if (editor_theme_preference(preference)) { editor_theme_load(&settings->theme, preference); }

    view.terminal.height = settings->terminal_height;
    bool ready = editor_workspace_restore(&view, document, settings, session_available ? session_path : NULL);
    i32 result = ready ? 0 : 1;

    if (ready) { editor_listen_folders(&view); }

    while (ready && ui_window_poll_events()) {
        if (!editor_process_input(renderer, window, document, &view, settings)) {
            result = 1;

            break;
        }

        editor_tab_activity(&view);
        editor_debug_poll(&view, document, settings);
        ui_window_set_cursor(window, editor_pointer_shape(window, &view, settings));

        if (!ui_window_is_visible(window)) {
            ui_sleep_ms(settings->minimized_pause_ms);

            continue;
        }

        if (!editor_draw(renderer, window, document, &view, settings) || !ui_window_wait_frame(window)) {
            result = 1;

            break;
        }
    }

    if (ready && !result && (!session_available || !editor_workspace_save(&view, session_path))) {
        ui_report_error("Could not save editor tab session");
    }

    editor_stop_folders();
    editor_debug_stop(&view.debug);
    free(view.row_map);
    for (size_t i = 0; i < view.tabs.count; ++i) { editor_document_release(&view.tabs.items[i].saved_state); }
    editor_terminal_destroy(&view.terminal);
    for (size_t i = 0; i < view.projects.count; ++i) {
        sys_folder_watch_destroy(view.projects.watches[i]);
    }

    editor_tree_destroy(&view.tree);

    return result;
}
