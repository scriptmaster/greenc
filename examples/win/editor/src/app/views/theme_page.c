/*
 ** theme_page.c - 2 functions.
 ** draw_control - Render a settings action with its pointer state.
 ** editor_themes_draw - Render the theme navigation, actions and editable fields.
 ** See also: editor_themes.h, themes_internal.h.
 */
#include "themes_internal.h"

static void draw_control(editor_themes* page, ui_renderer* renderer, const editor_theme* theme,
                         i32 id, const char* label, editor_control_kind kind) {
    editor_bounds b = control_bounds(id, page->first);
    b.y += page->top;
    bool hover = editor_contains(b, page->pointer_x, page->pointer_y);
    editor_control_draw(renderer, b, label, kind, theme, hover, page->controls.pressed == id, &page->transitions[id]);
}

void editor_themes_draw(editor_themes* page, ui_renderer* renderer, u32 width, u32 height, const editor_theme* theme) {
    if (height <= (u32)page->top) { return; }
    height -= (u32)page->top;
    ui_renderer_rect(renderer, 0, page->top, width, height, theme->background);
    i32 rows = visible_rows(height);
    scroll_fields(page, rows, 0);
    ui_renderer_rect(renderer, 0, page->top, SIDEBAR, height - 1, theme->panel);
    ui_renderer_text(renderer, 12, page->top + 10, "Settings", theme->heading);
    editor_disclosure(renderer, 16, page->top + 52, page->expanded, page->changed, theme->text, theme->panel);
    draw_control(page, renderer, theme, CATEGORY, "Editor", EDITOR_LINK);
    if (page->expanded) { draw_control(page, renderer, theme, THEMES, "Themes", EDITOR_SECONDARY_LINK); }

    if (page->expanded) { draw_control(page, renderer, theme, DEBUGGER, "Debugger", EDITOR_SECONDARY_LINK); }

    if (page->expanded) { draw_control(page, renderer, theme, KEYS, "Keybindings", EDITOR_SECONDARY_LINK); }

    if (page->expanded) {
        draw_control(page, renderer, theme, GENERAL, "General", EDITOR_SECONDARY_LINK);
    }

    if (page->general && !page->debug) {
        ui_renderer_text(renderer, LEFT, page->top + 10, "Editor / General", theme->heading);
        draw_control(page, renderer, theme, EXPLORER,
                     page->explorer_enabled ? "[x] Classic menu for this user" : "[ ] Classic menu for this user",
                     EDITOR_SECONDARY);
        draw_control(page, renderer, theme, INSTALL_ADMIN, "Install Windows 11 menu (administrator)", EDITOR_SECONDARY);
        draw_control(page, renderer, theme, REMOVE_ADMIN, "Remove Windows 11 menu (administrator)", EDITOR_SECONDARY);
        if (page->explorer_status) {
            ui_renderer_text(renderer, LEFT, page->top + 100, page->explorer_status, theme->muted);
        }

        return;
    }

    if (page->keybindings && !page->debug) {
        const char* lines[KEYBINDING_ROWS] = {
            "Editor / Keybindings", "Ctrl+T / Ctrl+W    Open / close focused tab",
            "Ctrl+O / Ctrl+S    Open / save file", "Ctrl+Shift+S       Save as",
            "Ctrl+Z / Ctrl+Y    Undo / redo", "Ctrl+Shift+Z       Redo",
            "PgUp / PgDn        Move three quarters of a page", "Ctrl+B             Toggle Files",
            "Ctrl+grave         Toggle terminal", "F5 / Shift+F5      Debug / stop",
            "F10 / F11          Step over / into", "Suggested additions",
            "Ctrl+F   Find     Ctrl+H   Replace     Ctrl+G   Go to line"
        };

        i32 y = 10;
        for (i32 i = page->first_keybinding; i < KEYBINDING_ROWS && y + KEYBINDING_HEIGHT <= (i32)height; ++i) {
            ui_renderer_text(renderer, LEFT, page->top + y, lines[i], i == 0 || i == 11 ? theme->heading : theme->text);
            y += KEYBINDING_HEIGHT;
        }

        return;
    }

    if (page->debug) {
        ui_renderer_text(renderer, LEFT, page->top + 10, "Editor / Debugger", theme->heading);
        const char* path = page->debug_path;
        size_t length = strlen(path);
        if (length > 45) { path += length - 45; }
        ui_renderer_text(renderer, LEFT, page->top + 38, length ? path : "Choose a debugger executable", theme->text);
        if (page->debug_save_failed) { ui_renderer_text(renderer, LEFT, page->top + 102, "Could not save debugger preference", theme->muted); }
        draw_control(page, renderer, theme, BROWSE, "Browse...", EDITOR_SECONDARY);
        return;
    }
    ui_renderer_text(renderer, LEFT, page->top + 10, "Editor / Themes", theme->heading);
    ui_renderer_text(renderer, LEFT, page->top + 38, page->path[0] ? page->path : "Built-in theme", theme->muted);
    draw_control(page, renderer, theme, BROWSE, "Browse...", EDITOR_SECONDARY);
    draw_control(page, renderer, theme, SAVE, "Save As...", EDITOR_PRIMARY);
    if (page->status) { ui_renderer_text(renderer, LEFT + 224, page->top + 68, page->status, theme->muted); }
    draw_control(page, renderer, theme, GROUP, theme_groups[page->group], EDITOR_SECONDARY);
    if (control_visible(page, PREVIOUS, rows)) { draw_control(page, renderer, theme, PREVIOUS, "Prev", EDITOR_LINK); }
    if (control_visible(page, NEXT, rows)) { draw_control(page, renderer, theme, NEXT, "Next", EDITOR_LINK); }

    for (i32 i = page->first; i < page->first + rows && i < group_edges[page->group + 1]; ++i) {
        i32 id = FIRST_FIELD + 2 * i;
        editor_bounds b = control_bounds(id, page->first);
        u32 value = *(const u32*)((const char*)theme + fields[i].offset);
        char label[64];
        if (fields[i].maximum) {
            snprintf(label, sizeof(label), "%s %u%s", fields[i].name, value,
                     fields[i].maximum == 100 ? "%" : fields[i].maximum == 24 ? "px" : "");
            ui_renderer_text(renderer, LEFT, page->top + b.y + 4, label, theme->text);
            draw_control(page, renderer, theme, id, "-", EDITOR_SECONDARY);
            draw_control(page, renderer, theme, id + 1, "+", EDITOR_SECONDARY);
        } else {
            snprintf(label, sizeof(label), "%s  #%06X", fields[i].name, value);
            draw_control(page, renderer, theme, id, label, EDITOR_SECONDARY);
            ui_renderer_rect(renderer, b.x + b.width - 24, page->top + b.y + 6, 14, 14, value);
        }
    }
}
