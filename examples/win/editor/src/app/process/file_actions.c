/*
 ** file_actions.c - 3 functions.
 ** duplicate_file - Copy the selected file to an unused sibling name.
 ** delete_file - Confirm deletion and reconcile open document tabs.
 ** editor_files_context - Offer filesystem actions appropriate to the clicked tree row.
 ** See also: editor_file_actions.h, file_actions_internal.h.
 */
#include "file_actions_internal.h"
#include "file_size.h"

static void duplicate_file(editor_tree* tree, editor_document* document, const char* folder) {
    const char* leaf = strrchr(tree->selected, '\\');
    leaf = leaf ? leaf + 1 : tree->selected;
    const char* extension = strrchr(leaf, '.');
    if (!extension || extension == leaf) { extension = leaf + strlen(leaf); }

    for (u32 number = 1; number < 10000; ++number) {
        char name[SYS_PATH_CAPACITY], path[SYS_PATH_CAPACITY];
        i32 length = snprintf(name, sizeof(name), "%.*s - Copy %u%s", (i32)(extension - leaf), leaf, number, extension);
        if (length < 0 || (size_t)length >= sizeof(name) || !sys_child_path(folder, name, path)) { break; }

        u64 ignored;
        if (sys_file_size(path, &ignored)) { continue; }
        if (sys_file_copy_new(tree->selected, path)) {
            refresh_entry(tree, folder, path, false, document);

            return;
        }

        break;
    }

    document->status = "Could not duplicate file";
}

static void delete_file(editor_view* view, editor_document* document, editor_settings* settings, const char* folder) {
    editor_tree* tree = &view->tree; ///read
    size_t index = file_tab(&view->tabs, tree->selected);
    if (index < view->tabs.count && (index == view->tabs.active ? document->dirty : view->tabs.items[index].saved_state.dirty)) {
        document->status = "Save or close the modified tab before deleting its file";

        return;
    }

    if (!ui_confirm(tree->selected, "Permanently delete this file?")) { return; }
    if (!sys_file_delete(tree->selected)) {
        document->status = "Could not delete file";

        return;
    }

    if (index < view->tabs.count) { editor_tabs_close(&view->tabs, index, document, settings); }
    refresh_entry(tree, folder, "", false, document);
}

void editor_files_context(editor_view* view, editor_document* document, editor_settings* settings,
                          ui_renderer* renderer, ui_window* window, ui_input input) {
    editor_tree* tree = &view->tree; ///read
    editor_tree_select_at(tree, renderer, input.y);
    char folder[SYS_PATH_CAPACITY];
    if (!target_folder(tree, folder)) { return; }

    ui_popup_item menu[12] = {{"Open Folder in Panel...", OPEN_DIRECTORY}, {"New File Here...", NEW_FILE},
                              {"New Folder Here...", NEW_FOLDER}};
    size_t count = 3;
    if (_stricmp(folder, tree->root)) { menu[count++] = (ui_popup_item){"New File in Panel Root...", ROOT_FILE}; }
    if (tree->selected[0] && !tree->selected_folder) {
        menu[count++] = (ui_popup_item){"Duplicate File", DUPLICATE};
        menu[count++] = (ui_popup_item){"Duplicate As...", COPY_AS};
        menu[count++] = (ui_popup_item){"Rename...\tF2", RENAME};
        menu[count++] = (ui_popup_item){"Delete File...", DELETE_FILE};
        menu[count++] = (ui_popup_item){"File Info", FILE_INFO};
    }

    menu[count++] = (ui_popup_item){"System Properties", PROPERTIES};
    menu[count++] = (ui_popup_item){"Open Containing Folder in Explorer", SHOW_DIRECTORY};
    u16 command = ui_popup_menu(window, input.x, input.y, menu, count);
    const char* selected = tree->selected[0] ? tree->selected : tree->root;
    if (command == OPEN_DIRECTORY) {
        char chosen[SYS_PATH_CAPACITY];
        if (ui_choose_folder(window, chosen) && !editor_project_open(&view->projects, tree, chosen)) { document->status = "Cannot list folder"; }
    } else if (command == SHOW_DIRECTORY) {
        if (tree->selected_folder && !sys_parent_directory(selected, folder)) { return; }
        if (!ui_open_folder(window, folder)) { document->status = "Cannot open Explorer"; }
    } else if (command == PROPERTIES) {
        if (!ui_show_properties(window, selected)) { document->status = "Properties unavailable"; }
    } else if (command == FILE_INFO) {
        u64 bytes;
        if (sys_file_size(selected, &bytes)) {
            char info[512];
            char size[16];
            editor_format_size(size, bytes);
            snprintf(info, sizeof(info), "%s\n\nSize: %s (%llu bytes)", selected, size, (unsigned long long)bytes);
            ui_show_info(window, "File Info", info);
        } else {
            document->status = "File information unavailable";
        }
    } else if (command == DELETE_FILE) {
        delete_file(view, document, settings, folder);
    } else if (command == DUPLICATE) {
        duplicate_file(tree, document, folder);
    } else if (command) {
        begin_name(tree, command);
    }
}
