/*
 ** editor_start.c - 1 function.
 ** editor_start - Initialize the document, window and renderer and run the editor.
 ** See also: editor_start.h, main_internal.h.
 */
#include "main_internal.h"
#include "explorer_menu.h"
#include "folder_receiver.h"
#include <string.h>

int editor_start(int argc, char** argv) {
    if (argc == 2 && !strcmp(argv[1], "--register-explorer")) { return explorer_menu_set(true) ? 0 : 1; }
    if (argc == 2 && !strcmp(argv[1], "--unregister-explorer")) { return explorer_menu_set(false) ? 0 : 1; }

    if (argc == 2 && !strcmp(argv[1], "--install-machine-menu")) { return explorer_machine_set(true) ? 0 : 1; }
    if (argc == 2 && !strcmp(argv[1], "--remove-machine-menu")) { return explorer_machine_set(false) ? 0 : 1; }

    editor_settings settings = editor_settings_init(); ///stack
    if (!editor_settings_parse(&settings, argc, argv)) {
        return ui_report_error("Options: --cpu --gpu --fps=0..1000 --windowed --fullscreen --borderless [file]");
    }

    if (settings.launch_folder && editor_forward_folder(settings.launch_folder)) { return 0; }

    editor_document document = {.status = "Ready"};


    ui_window* window = ui_window_create(&settings.window); ///move
    if (!window) {
        editor_document_release(&document);
        return ui_report_error("Cannot create editor window.");
    }

    if (!ui_window_set_menu(window, editor_menu, sizeof(editor_menu) / sizeof(editor_menu[0]))) {
        editor_document_release(&document);
        ui_window_destroy(window);

        return ui_report_error("Cannot create editor menu.");
    }

    ui_renderer* renderer = ui_renderer_create(window, &settings.renderer); ///move
    if (!renderer) {
        editor_document_release(&document);
        ui_window_destroy(window);

        return ui_report_error("Cannot create editor renderer.");
    }

    i32 result = editor_run(window, renderer, &document, &settings);

    editor_document_release(&document);
    ui_renderer_destroy(renderer);
    ui_window_destroy(window);

    return result ? ui_report_error("Editor rendering or frame pacing failed.") : 0;
}
