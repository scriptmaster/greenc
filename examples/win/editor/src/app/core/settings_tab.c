/*
 ** settings_tab.c - 3 functions.
 ** editor_switch_settings - Switch representations of the same settings tab.
 ** editor_apply_settings_json - Validate and save edited settings.
 ** editor_settings_tab - Activate or create the singleton settings tab.
 ** See also: settings_tab.h, document_tabs.c.
 */
#include "settings_tab.h"
#include "tabs_internal.h"
#include "settings_json.h"
#include "debug_preferences.h"
#include "explorer_menu.h"

bool editor_apply_settings_json(editor_view* view, editor_document* doc, editor_settings* settings) {
    editor_theme candidate = settings->theme;
    char debugger[260], preference[260];
    strcpy(debugger, view->themes.debug_path);

    if (!doc->text || !editor_settings_json_parse(doc->text, &candidate, debugger)) {
        doc->open_error = "Invalid settings JSON: check property names, values and punctuation";

        return false;
    }

    if (!editor_theme_preference(preference) || !editor_theme_save(&candidate, preference) ||
        (strcmp(debugger, view->themes.debug_path) && !editor_debug_save(debugger))) {
        doc->open_error = "Could not save settings preferences";

        return false;
    }

    settings->theme = candidate;
    strcpy(view->themes.debug_path, debugger);
    doc->dirty = view->themes.dirty = false;
    doc->saved_state = doc->edit_state;
    doc->open_error = NULL;
    doc->status = "Settings applied";

    return true;
}

bool editor_switch_settings(editor_view* view, editor_document* doc, editor_settings* settings, size_t index) {
    select_tab(&view->tabs, index, doc, settings);
    editor_tab* tab = &view->tabs.items[index];

    if (tab->settings_json) {
        if (doc->dirty && !editor_apply_settings_json(view, doc, settings)) { return false; }
        editor_document_release(doc);
    } else {
        char json[4096];

        if (!editor_settings_json_format(&settings->theme, view->themes.debug_path, json, sizeof(json)) ||
            !editor_document_reserve(doc, strlen(json) + 1)) {
            doc->open_error = "Could not prepare settings JSON";

            return false;
        }

        strcpy(doc->text, json);
        doc->length = strlen(json);
        editor_document_changed(doc);
    }

    tab->settings_page = tab->settings_json;
    tab->settings_json = !tab->settings_page;
    strcpy(tab->path, tab->settings_json ? "Settings.json" : "Settings");
    view->themes.open = tab->settings_page;

    return true;
}

bool editor_settings_tab(editor_view* view, editor_document* document, editor_settings* settings) {
    view->themes.explorer_enabled = explorer_menu_enabled();

    for (size_t i = 0; i < view->tabs.count; ++i) {
        if (view->tabs.items[i].settings_page || view->tabs.items[i].settings_json) {
            select_tab(&view->tabs, i, document, settings);
            if (view->tabs.items[i].settings_json) { return editor_switch_settings(view, document, settings, i); }
            view->themes.open = true;

            return true;
        }
    }

    if (!editor_tabs_new(&view->tabs, document, settings)) {
        document->status = "File tab limit reached";
        view->themes.open = false;

        return false;
    }

    view->tabs.items[view->tabs.active].settings_page = true;
    view->themes.open = view->themes.expanded = true;

    return true;
}
