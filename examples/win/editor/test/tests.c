/*
 ** tests.c - 4 functions.
 ** exercise_console - Verify command execution in a detached shell.
 ** test_console - Start and join the detached console test process.
 ** test_tab_strip - Check scrolling, selection and close targets.
 ** main - Run document, project, console and editor regression checks.
 ** See also: editor_document.h, editor_projects.h.
 */
#include "editor_tabs.h"
#include "editor_projects.h"
#include "sys/fs/entries.h"
#include "editor_scrollbar.h"
#include "row_index.h"
#include "page_navigation.h"
#include "editor_hex.h"
#include "editor_undo.h"
#include "editor_dispatch.h"
#include "ui/ui_window_win32.h"
#include "file_size.h"
#include "editor_status.h"
#include "editor_themes.h"
#include "settings_json.h"
#include "workspace_session.h"
#include "terminal_internal.h"
#include "byte_text.h"
#include "editor_tree.h"
#include "editor_document.h"
#include "sys/shell.h"
#include "sys/console.h"
#include "editor_input.h"
#include "editor_settings.h"
#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sys/fs/write_atomic.h"
#include <assert.h>
#include "modified_time.h"
#include "editor_debug.h"
#include "settings_tab.h"

/* Capture editor clipboard writes without replacing the desktop clipboard. */
static char copied_hex[1024];
bool __wrap_ui_clipboard_write(ui_window* window, const char* text, size_t length) {
    (void)window;
    if (length >= sizeof(copied_hex)) { return false; }

    memcpy(copied_hex, text, length);
    copied_hex[length] = 0;

    return true;
}

static int exercise_console(const char* profile) {
    sys_console* console = sys_console_start(profile);
    sys_console_view view;
    bool ready = false;
    for (unsigned i = 0; console && i < 100 && !ready; ++i) {
        ready = sys_console_viewport(console, 80, 16, &view);
        if (!ready) { Sleep(20); }
    }

    if (ready && !strcmp(profile, "powershell.exe")) {
        bool prompt = false;
        for (unsigned i = 0; i < 500 && !prompt; ++i) {
            Sleep(20);
            char line[81];
            if (sys_console_viewport(console, 80, 16, &view) &&
                sys_console_row(console, view.left, view.top + view.cursor_y, line, 80)) {
                if (strstr(line, "default is") && strchr(line, 'D')) {
                    sys_console_input(console, SYS_CHAR, 'D', false, false);
                    sys_console_input(console, SYS_CHAR, '\r', false, false);
                    Sleep(200);
                }

                prompt = strstr(line, "PS ") && strchr(line, '>');
            }
        }

        ready = prompt;
    }

    const char* command = "echo terminal-ready\r";
    for (const char* c = command; ready && *c; ++c) {
        ready = sys_console_input(console, SYS_CHAR, *c, false, false);
    }

    bool matched = false;
    for (unsigned i = 0; ready && !matched && i < 250; ++i) {
        Sleep(20);
        if (!sys_console_viewport(console, 80, 16, &view)) { break; }
        for (int y = 0; y < 16; ++y) {
            char line[81];
            if (sys_console_row(console, view.left, view.top + y, line, 80) && !strcmp(line, "terminal-ready")) {
                matched = true;

                break;
            }
        }
    }

    sys_console_destroy(console);

    return matched ? 0 : 1;
}

static void test_console(const char* profile) {
    char executable[MAX_PATH], command[128];
    snprintf(command, sizeof(command), "tests.exe --console %s", profile);
    assert(GetModuleFileNameA(NULL, executable, sizeof(executable)));
    STARTUPINFOA startup = {.cb = sizeof(startup)};
    PROCESS_INFORMATION process;
    assert(CreateProcessA(executable, command, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &startup, &process));
    assert(WaitForSingleObject(process.hProcess, 30000) == WAIT_OBJECT_0);
    DWORD result;
    assert(GetExitCodeProcess(process.hProcess, &result) && result == 0);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

static void test_tab_strip(void) {
    for (u32 picker = 0; picker <= 1; ++picker) {
        editor_tab_scroll scroll = {0};
        editor_tab_strip strip = editor_strip_layout(&scroll, 1, 0, 600, 8, picker);
        size_t index = 99;
        assert(strip.plus_x == 2 * strip.button + strip.tab);
        assert(editor_strip_click(&strip, &scroll, strip.plus_x, &index) == TAB_ADD);
        assert(editor_strip_click(&strip, &scroll, strip.plus_x + strip.button, &index) ==
               (picker ? TAB_PICKER : TAB_NONE));

        strip = editor_strip_layout(&scroll, 6, 0, 360, 8, picker);
        assert(strip.visible == 2);
        editor_strip_click(&strip, &scroll, strip.button, &index);
        strip = editor_strip_layout(&scroll, 6, 0, 360, 8, picker);
        assert(strip.first == 1);
        assert(editor_strip_click(&strip, &scroll, 2 * strip.button, &index) == TAB_SELECT && index == 1);

        assert(editor_strip_click(&strip, &scroll, 2 * strip.button + strip.tab - 1, &index) == TAB_CLOSE);
        assert(index == 1);

        scroll.reveal_active = true;
        strip = editor_strip_layout(&scroll, 7, 6, 360, 8, picker);
        assert(strip.first + strip.visible == 7);
        editor_strip_click(&strip, &scroll, 0, &index);
        assert(scroll.first == strip.first - 1);

        strip = editor_strip_layout(&scroll, 7, 6, 0, 8, picker);
        assert(!strip.visible && editor_strip_click(&strip, &scroll, 0, &index) == TAB_NONE);
    }
}

static void check_modified_labels(void) {
    const struct { int year, month, day, hour, minute; const char* label; } cases[] = {
        {2026, 5, 14, 12, 0, "A few seconds ago."}, {2026, 5, 14, 11, 58, "A few minutes ago."},
        {2026, 5, 14, 9, 0, "A few hours ago."}, {2026, 5, 13, 0, 0, "Yesterday."},
        {2026, 5, 12, 0, 0, "Day before yesterday."}, {2026, 5, 11, 0, 0, "Last Monday."},
        {2026, 5, 7, 0, 0, "Last week."}, {2026, 4, 30, 0, 0, "Two weeks ago."},
        {2026, 4, 23, 0, 0, "Three weeks ago."}, {2026, 4, 14, 0, 0, "Last month."},
        {2025, 6, 14, 0, 0, "Eleven months ago."}, {2025, 5, 14, 0, 0, "Last year."},
        {2017, 5, 14, 0, 0, "Nine years ago."}, {2016, 5, 14, 0, 0, "A decade ago."},
        {2006, 5, 14, 0, 0, "Two decades ago."}, {1996, 5, 14, 0, 0, "Many decades ago."}
    };
    struct tm reference = {.tm_year = 126, .tm_mon = 4, .tm_mday = 14, .tm_hour = 12, .tm_isdst = -1};
    time_t now = mktime(&reference);
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        struct tm date = {.tm_year = cases[i].year - 1900, .tm_mon = cases[i].month - 1,
                          .tm_mday = cases[i].day, .tm_hour = cases[i].hour,
                          .tm_min = cases[i].minute, .tm_isdst = -1};
        char actual[64];
        editor_modified_label(actual, mktime(&date), now);
        assert(!strcmp(actual, cases[i].label));
    }
}

static void dispatch_history_key(ui_window* window, ui_renderer* renderer, editor_view* view,
                                 editor_document* document, editor_settings* settings, UINT key, bool shift) {
    BYTE original[256], pressed[256];
    assert(GetKeyboardState(original));
    memcpy(pressed, original, sizeof(pressed));
    pressed[VK_CONTROL] = 0x80;
    pressed[VK_SHIFT] = shift ? 0x80 : 0;
    assert(SetKeyboardState(pressed));
    SendMessageA(window->handle, WM_KEYDOWN, key, 0);
    assert(SetKeyboardState(original));
    assert(editor_process_input(renderer, window, document, view, settings));
}

static void check_fold_pointer(void) {
    editor_settings settings = editor_settings_init();
    settings.window.mode = UI_WINDOW_WINDOWED;
    ui_window* window = ui_window_create(&settings.window);
    assert(window);
    ui_renderer* renderer = ui_renderer_create(window, &settings.renderer);
    assert(renderer);
    editor_document document = {0};
    assert(editor_document_reserve(&document, 256));
    strcpy(document.text, "before\nouter\n  one\n  two\nafter");
    document.length = strlen(document.text);
    document.cursor = 3;
    document.anchor = 2;
    document.selecting = true;
    editor_view view = {.first_line = 1, .manual_scroll = true};
    editor_viewport viewport = {0};
    assert(editor_measure_viewport(renderer, window, &document, &settings, &view, &viewport));
    ui_input click = {.mouse = UI_MOUSE_DOWN, .x = (i32)viewport.text_left - 12, .y = (i32)settings.layout.text_top + 2};
    editor_mouse_select(&view, &document, renderer, window, &settings, click);
    assert(view.manual_scroll && view.first_line == 1 && document.anchor == 2 && document.cursor == 3 && !view.dragging);
    assert(editor_measure_viewport(renderer, window, &document, &settings, &view, &viewport));
    char label[48];
    click.x = editor_fold_label(&viewport, &view, 1, label);
    assert(!strcmp(label, "... 2 more lines"));
    editor_mouse_select(&view, &document, renderer, window, &settings, click);
    assert(!document.folded[7] && document.selecting && document.anchor == 2);
    click.x = 10;
    editor_mouse_select(&view, &document, renderer, window, &settings, click);
    assert(document.folded[7] == 2 && view.themes.open && view.themes.debug);
    click.y += (i32)(6 * viewport.cell_height);
    click.shift = true;
    editor_mouse_select(&view, &document, renderer, window, &settings, click);
    assert(document.cursor == document.length && document.anchor == 2);
    click.shift = false;
    editor_mouse_select(&view, &document, renderer, window, &settings, click);
    assert(document.anchor == document.length && document.cursor == document.length);
    assert(editor_tabs_new(&view.tabs, &document, &settings));
    assert(editor_settings_tab(&view, &document, &settings));
    assert(view.tabs.count == 2 && view.tabs.items[view.tabs.active].settings_page);
    assert(editor_settings_tab(&view, &document, &settings) && view.tabs.count == 2);
    editor_tabs_close(&view.tabs, view.tabs.active, &document, &settings);
    for (int binary = 0; binary <= 1; ++binary) {
        document.binary = document.full_hex = binary != 0;
        document.cursor = document.anchor = 0;
        document.selecting = document.low_nibble = false;
        char previous = document.text[0];
        view.hex_columns = 16;

        if (binary) { editor_hex_key(&view, &document, (ui_input){.character = 'F'}); }
        else { editor_insert_character(&document, 'Z'); }

        char edited = document.text[0];
        assert(previous != edited);
        dispatch_history_key(window, renderer, &view, &document, &settings, 'Z', false);
        assert(document.text[0] == previous);
        dispatch_history_key(window, renderer, &view, &document, &settings, 'Y', false);
        assert(document.text[0] == edited);
        dispatch_history_key(window, renderer, &view, &document, &settings, 'Z', false);
        dispatch_history_key(window, renderer, &view, &document, &settings, 'Z', true);
        assert(document.text[0] == edited);
    }

    free(view.row_map);
    editor_document_release(&document);
    ui_renderer_destroy(renderer);
    ui_window_destroy(window);
}

static void test_debugger(const char* debugger) {
    editor_view view = {0};
    editor_document doc = {0};
    editor_settings settings = editor_settings_init();
    assert(editor_tabs_new(&view.tabs, &doc, &settings));
    assert(editor_tabs_open(&view.tabs, &doc, &settings, "test/debug_target.gc"));
    strcpy(view.themes.debug_path, debugger);
    doc.folded = calloc(doc.capacity, 1);
    assert(doc.folded);
    doc.folded[strstr(doc.text, "    value = increment") - doc.text] = 2;
    doc.folded[strstr(doc.text, "    return value ==") - doc.text] = 2;
    assert(editor_debug_start(&view, &doc, "bin/debug_target.exe"));

    const size_t expected[] = {7, 2, 3, 9};
    const ui_key commands[] = {UI_KEY_F11, UI_KEY_F10, UI_KEY_F5, UI_KEY_F5};
    for (size_t i = 0; i < 4; ++i) {
        for (int poll = 0; view.debug.process && !view.debug.paused && poll < 1000; ++poll) {
            editor_debug_poll(&view, &doc, &settings);
            Sleep(10);
        }

        fprintf(stderr, "GDB stop %zu: line %zu (%s)\n", i + 1, view.debug.line, doc.status);
        assert(view.debug.paused && view.debug.line == expected[i]);
        editor_debug_key(&view, &doc, &settings, NULL, (ui_input){.key = commands[i], .shift = i == 3});
    }

    assert(!view.debug.process);
    editor_document_release(&doc);
}

static void test_page_navigation(void) {
    editor_line_entry lines[15] = {0};
    editor_viewport viewport = {.rows = 8, .visible_lines = 15, .map = lines, .cursor_column = 3};
    editor_view view = {0};
    editor_document doc = {.cursor = 3};
    ui_input down = {.key = UI_KEY_PAGE_DOWN, .shift = true};
    ui_input up = {.key = UI_KEY_PAGE_UP};

    lines[6].offset = 40;
    lines[6].end = 42;
    editor_text_page(&view, &doc, &viewport, down);
    assert(doc.cursor == 42 && doc.selecting && doc.anchor == 3 && view.first_line == 6);

    viewport.cursor_line = 6;
    editor_text_page(&view, &doc, &viewport, up);
    assert(doc.cursor == 0 && !doc.selecting && view.first_line == 0);

    view.hex_columns = 4;
    view.hex_rows = 4;
    doc.full_hex = true;
    doc.length = 14;
    doc.cursor = 3;
    editor_hex_page(&view, &doc, down);
    assert(doc.cursor == 13 && doc.anchor == 3 && doc.selecting);
    editor_hex_page(&view, &doc, up);
    assert(doc.cursor == 1 && !doc.selecting && doc.hex_first == 0);
}

int main(int argc, char** argv) {
    test_page_navigation();
    if (argc == 3 && !strcmp(argv[1], "--debugger")) {
        test_debugger(argv[2]);

        return 0;
    }

    check_modified_labels();
    if (argc == 4 && !strcmp(argv[1], "--check-link")) {
        editor_document linked = {0};
        u64 target, link;
        assert(sys_file_size(argv[3], &target) && target > 0);
        assert(sys_file_size(argv[2], &link) && link == target);
        assert(editor_load_existing(&linked, argv[2]));
        assert(linked.disk_bytes == target && linked.modified[0]);
        printf("Symlink and target both report %llu bytes.\\n", (unsigned long long)target);
        editor_document_release(&linked);

        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--open-large")) {
        editor_document large = {0};
        assert(editor_load(&large, argv[2]) && large.length > 4095);
        assert(large.text[large.length] == 0);
        editor_document_release(&large);

        return 0;
    }

    test_tab_strip();
    assert(editor_marquee_offset(0, 4000) == 0);
    assert(editor_marquee_offset(50, 1200) == 0);
    assert(editor_marquee_offset(50, 1520) == 10);
    assert(editor_marquee_offset(50, 2800) == 50);
    assert(editor_marquee_offset(50, 5600) == 0);
    editor_status_bar footer = {.fps_left = 540, .fps_right = 590, .top = 400, .bottom = 428};
    assert(editor_status_hit(&footer, 540, 400) && !editor_status_hit(&footer, 590, 412));
    editor_scrollbar bar = editor_scrollbar_layout(600, 40, 240, 15, 90);
    assert(bar.maximum == 75 && editor_scroll_position(bar, editor_thumb_top(bar, 75)) == 75);
    assert(editor_scroll_position(bar, -100) == 0 && editor_scroll_position(bar, 999) == 75);
    assert(!editor_scrollbar_layout(600, 40, 240, 15, 15).maximum);

    editor_control_input control = {0};
    assert(!editor_control_activate(&control, (ui_input){.mouse = UI_MOUSE_DOWN}, 1));
    assert(!editor_control_activate(&control, (ui_input){.mouse = UI_MOUSE_UP}, 2));
    assert(!editor_control_activate(&control, (ui_input){.mouse = UI_MOUSE_DOWN}, 2));
    assert(editor_control_activate(&control, (ui_input){.mouse = UI_MOUSE_UP}, 2));
    if (argc >= 2 && !strcmp(argv[1], "--console")) { return exercise_console(argc == 3 ? argv[2] : "cmd.exe"); }
    check_fold_pointer();
    test_console(argc == 3 && !strcmp(argv[1], "--profile") ? argv[2] : "cmd.exe");

    editor_document folded = {.text = "outer\n  inner\n    body\nnext"};
    folded.length = strlen(folded.text);
    assert(editor_fold_end(&folded, 0) == 23);
    assert(editor_fold_end(&folded, 6) == 23);
    assert(editor_fold_end(&folded, 23) == folded.length + 1);

    editor_document navigation = {.text = "first\nlast", .length = 10};
    editor_input(&navigation, (ui_input){.key = UI_KEY_DOWN, .shift = true});
    editor_input(&navigation, (ui_input){.key = UI_KEY_DOWN, .shift = true});
    assert(navigation.anchor == 0 && navigation.cursor == 10);
    editor_input(&navigation, (ui_input){.key = UI_KEY_HOME, .control = true});
    assert(navigation.cursor == 0);
    editor_input(&navigation, (ui_input){.key = UI_KEY_END, .control = true});
    assert(navigation.cursor == 10);

    sys_shell* shell = sys_shell_create("cmd.exe");
    assert(shell);
    const char command[] = "@echo result=%COMSPEC%\r\n";
    assert(sys_shell_send(shell, command, sizeof(command) - 1));
    char capture[2048] = {0};
    size_t received = 0;
    for (size_t attempt = 0; attempt < 200 && !strstr(capture, "result=C:"); ++attempt) {
        received += sys_shell_read(shell, capture + received, sizeof(capture) - 1 - received);
        Sleep(10);
    }

    sys_shell_destroy(shell);
    assert(strstr(capture, "result=C:"));

    editor_document document = {.status = "Ready"};
    editor_input(&document, (ui_input){.character = 'a'});
    editor_input(&document, (ui_input){.character = '\n'});
    editor_input(&document, (ui_input){.character = 'b'});
    assert(strcmp(document.text, "a\nb") == 0 && document.cursor == 3 && document.dirty);
    editor_input(&document, (ui_input){.key = UI_KEY_UP});
    assert(document.cursor == 1);
    editor_input(&document, (ui_input){.key = UI_KEY_HOME});
    editor_input(&document, (ui_input){.character = 'x'});
    assert(strcmp(document.text, "xa\nb") == 0);
    editor_input(&document, (ui_input){.key = UI_KEY_DOWN});
    assert(document.cursor == 4);
    editor_input(&document, (ui_input){.character = '\b'});
    assert(strcmp(document.text, "xa\n") == 0);
    editor_input(&document, (ui_input){.character = '\b'});
    assert(strcmp(document.text, "xa") == 0);
    editor_input(&document, (ui_input){.key = UI_KEY_HOME});
    editor_input(&document, (ui_input){.key = UI_KEY_DELETE});
    assert(strcmp(document.text, "a") == 0 && document.length == 1);
    editor_input(&document, (ui_input){.character = '\t'});
    assert(strcmp(document.text, "    a") == 0);
    editor_input(&document, (ui_input){.character = 19, .control = true});
    assert(document.length == 5); /* Ctrl+S must not insert its control character. */
    editor_input(&document, (ui_input){.key = UI_KEY_A, .control = true});
    editor_input(&document, (ui_input){.character = 'z'});
    assert(strcmp(document.text, "z") == 0);
    editor_input(&document, (ui_input){.key = UI_KEY_LEFT, .shift = true});
    assert(document.anchor == 1 && document.cursor == 0);
    editor_input(&document, (ui_input){.key = UI_KEY_DELETE});
    assert(document.length == 0);

    assert(editor_document_reserve(&document, 4096));
    memset(document.text, 'a', 4095);
    document.length = 4095;
    document.cursor = document.length;
    document.text[document.length] = '\0';
    editor_input(&document, (ui_input){.character = 'b'});
    assert(document.length == 4096 && document.text[4095] == 'b');
    editor_settings settings = editor_settings_init();
    assert(settings.window.mode == UI_WINDOW_MAXIMIZED && settings.window.target_fps == 30);
    assert(settings.renderer.preference == UI_RENDERER_CPU);
    assert(editor_next_frame_rate(&settings, 30) == 60);
    char* arguments[] = {"editor", "--cpu", "--fps=144", "--borderless", "draft.txt"};
    assert(editor_settings_parse(&settings, 5, arguments));
    assert(settings.renderer.preference == UI_RENDERER_CPU && settings.window.target_fps == 144);
    assert(!settings.window.title_bar && !settings.window.minimize_button);
    assert(strcmp(settings.path, "draft.txt") == 0);
    assert(editor_next_frame_rate(&settings, 144) == 240);
    char* invalid[] = {"editor", "--fps=1001"};
    assert(!editor_settings_parse(&settings, 2, invalid));

    char directory[MAX_PATH], path[MAX_PATH];
    assert(GetTempPathA(MAX_PATH, directory));
    assert(GetTempFileNameA(directory, "gce", 0, path));
    char session_file[MAX_PATH];
    assert(GetTempFileNameA(directory, "gcs", 0, session_file));
    editor_view workspace = {0};
    editor_document workspace_doc = {0};
    editor_settings startup = editor_settings_init();
    startup.path = path;
    startup.explicit_file = true;
    assert(editor_workspace_restore(&workspace, &workspace_doc, &startup, NULL));
    assert(editor_tabs_new(&workspace.tabs, &workspace_doc, &startup));
    assert(new_session(&workspace.terminal, "cmd.exe"));
    workspace.terminal.visible = true;
    assert(editor_workspace_save(&workspace, session_file));
    editor_terminal_destroy(&workspace.terminal);
    sys_folder_watch_destroy(workspace.projects.watches[0]);
    editor_tree_destroy(&workspace.tree);
    editor_document_release(&workspace.tabs.items[0].saved_state);
    editor_document_release(&workspace_doc);

    workspace = (editor_view){0};
    startup.path = path;
    char* folder_args[] = {"editor.exe", "--folder", directory};
    assert(editor_settings_parse(&startup, 3, folder_args) && startup.launch_folder == directory);
    assert(editor_workspace_restore(&workspace, &workspace_doc, &startup, session_file));
    assert(workspace.tabs.count == 2 && workspace.tabs.active == 0 && workspace.tabs.items[1].untitled);
    assert(workspace.projects.count == 1 && !_stricmp(workspace.tree.selected, startup.path));
    assert(workspace.terminal.count == 1 && workspace.terminal.visible && !strcmp(workspace.terminal.sessions[0].name, "cmd.exe"));
    editor_terminal_destroy(&workspace.terminal);
    sys_folder_watch_destroy(workspace.projects.watches[0]);
    editor_tree_destroy(&workspace.tree);
    editor_document_release(&workspace_doc);
    assert(DeleteFileA(session_file));

    editor_viewport wide = {.width = 200, .height = 200, .text_left = 40, .columns = 16, .cell_width = 8};
    editor_scrollbar horizontal = document_horizontal_bar(&wide, 100, &settings);
    assert(horizontal.maximum == 85 && editor_scroll_position(horizontal, horizontal.top + horizontal.travel) == 85);
    assert(settings.theme.column_guide == 100);
    const char* source = "first\r\nsecond\rthird\tend";
    char markdown[MAX_PATH];
    assert(strlen(path) + 3 < sizeof(markdown));
    strcpy(markdown, path);
    strcat(markdown, ".md");
    char sample[300];
    memset(sample, 'a', sizeof(sample));
    memcpy(sample, "# Header\r\n\t", 11);
    assert(sys_file_write_atomic(markdown, sample, sizeof(sample)));
    assert(editor_load_existing(&document, markdown) && !document.binary);
    assert(!memcmp(document.text, "# Header\n    a", 14));

    sample[299] = 0;
    assert(sys_file_write_atomic(markdown, sample, sizeof(sample)));
    assert(editor_load_existing(&document, markdown) && document.binary);
    assert(document.length == sizeof(sample) && !memcmp(document.text, sample, sizeof(sample)));

    sample[299] = 'a';
    sample[255] = 0x7f;
    assert(sys_file_write_atomic(markdown, sample, sizeof(sample)));
    assert(editor_load_existing(&document, markdown) && document.binary);
    assert(!memcmp(document.text, sample, sizeof(sample)));
    document.cursor = 255;
    editor_open_byte_text(&document);
    assert(document.byte_text && !document.binary && !document.dirty && document.cursor == 255);
    editor_view byte_rows = {0};
    assert(editor_index_rows(&byte_rows, &document));
    assert(!byte_rows.row_map[0].has_unsupported_byte && byte_rows.row_map[1].has_unsupported_byte);
    editor_delete_character(&document, false);
    assert(editor_index_rows(&byte_rows, &document) && !byte_rows.row_map[1].has_unsupported_byte);
    assert(document.length == sizeof(sample) - 1 && document.text[255] == 'a');
    editor_undo(&document);
    assert(document.length == sizeof(sample) && !memcmp(document.text, sample, sizeof(sample)));
    assert(editor_index_rows(&byte_rows, &document) && byte_rows.row_map[1].has_unsupported_byte);
    free(byte_rows.row_map);
    document.selecting = true;
    document.anchor = 255;
    document.cursor = 256;
    editor_insert_character(&document, '?');
    sample[255] = '?';
    editor_save(&document, markdown);
    assert(!document.dirty);
    FILE* saved_bytes = fopen(markdown, "rb");
    char saved_sample[sizeof(sample)];
    assert(saved_bytes && fread(saved_sample, 1, sizeof(saved_sample), saved_bytes) == sizeof(sample));
    assert(!memcmp(saved_sample, sample, sizeof(sample)) && fgetc(saved_bytes) == EOF);
    assert(!fclose(saved_bytes));
    assert(editor_load_existing(&document, markdown) && !document.binary);
    assert(DeleteFileA(markdown));

    char* boundary = malloc(65540);
    assert(boundary);
    memset(boundary, 'x', 65535);
    memcpy(boundary + 65535, "\r\n\tz", 4);
    assert(sys_file_write_atomic(path, boundary, 65539));
    free(boundary);
    assert(editor_load_existing(&document, path));
    assert(document.length == 65541 && !strcmp(document.text + 65535, "\n    z"));
    if (sizeof(size_t) > 4) {
        size_t large_offset = (size_t)UINT32_MAX + 100u;
        editor_line_entry entries[] = {{.offset = 0}, {.offset = large_offset}, {.offset = large_offset + 42}};
        editor_view index = {.row_map = entries, .mapped_rows = 3};
        assert(editor_cursor_row(&index, large_offset + 41) == 1 && editor_cursor_row(&index, large_offset + 42) == 2);
    }

    assert(sys_file_write_atomic(path, source, strlen(source)));
    assert(editor_load(&document, path));
    assert(strcmp(document.text, "first\nsecond\nthird    end") == 0);
    assert(!document.dirty && document.cursor == 0);

    editor_insert_character(&document, 'x');
    editor_save(&document, path);
    assert(!document.dirty && strcmp(document.status, "Saved") == 0);
    editor_document reloaded = {0};
    assert(editor_load(&reloaded, path) && strcmp(document.text, reloaded.text) == 0);

    assert(sys_file_write_atomic(path, "\x80", 1));
    assert(editor_load(&document, path) && document.binary && document.full_hex && document.disk_bytes == 1);
    assert(document.modified[0] && (u8)document.text[0] == 0x80);
    char binary[257];
    memset(binary, 0x80, sizeof(binary));
    memcpy(binary, "A\r\n\t", 4);
    assert(sys_file_write_atomic(path, binary, 256));
    assert(editor_load(&document, path) && document.full_hex && !memcmp(document.text, binary, 256));
    assert(sys_file_write_atomic(path, binary, sizeof(binary)));
    assert(editor_load(&document, path) && document.binary && !document.full_hex && document.disk_bytes == 257);
    assert(!memcmp(document.text, binary, sizeof(binary)));
    editor_view hex_view = {.hex_columns = 16, .hex_rows = 8};
    char formatted[16];
    editor_format_size(formatted, 999);
    assert(!strcmp(formatted, "999 B"));
    editor_format_size(formatted, 1000);
    assert(!strcmp(formatted, "1.0 KB"));
    editor_format_size(formatted, 1250);
    assert(!strcmp(formatted, "1.3 KB"));
    editor_format_size(formatted, 1500000);
    assert(!strcmp(formatted, "1.5 MB"));
    editor_format_size(formatted, 2500000000ULL);
    assert(!strcmp(formatted, "2.5 GB"));

    hex_view.hex_cell_width = 8;
    hex_view.hex_cell_height = 16;
    hex_view.hex_top = 48;
    editor_hex_mouse(&hex_view, &document, (ui_input){.mouse = UI_MOUSE_DOWN, .x = 8 + (11 + 48 + 3) * 8, .y = 65});
    assert(document.cursor == 19);
    editor_hex_mouse(&hex_view, &document, (ui_input){.mouse = UI_MOUSE_DOWN, .x = 8 + (10 + 3 * 7) * 8, .y = 65});
    assert(document.cursor == 23);
    editor_hex_mouse(&hex_view, &document, (ui_input){.mouse = UI_MOUSE_DRAG, .x = 8 + (11 + 48 + 3) * 8, .y = 65});
    editor_hex_mouse(&hex_view, &document, (ui_input){.mouse = UI_MOUSE_UP, .x = 8 + (11 + 48 + 3) * 8, .y = 65});
    assert(document.selecting && document.anchor == 23 && document.cursor == 19 && !hex_view.dragging);
    editor_hex_key(&hex_view, &document, (ui_input){.key = UI_KEY_RIGHT, .shift = true});
    assert(document.selecting && document.anchor == 23 && document.cursor == 20);
    editor_hex_key(&hex_view, &document, (ui_input){.key = UI_KEY_A, .control = true});
    assert(document.selecting && document.anchor == 0 && document.cursor == 255);
    editor_hex_copy(&document, NULL);
    assert(strlen(copied_hex) == 3 * 256 - 1);
    for (size_t i = 0; i < 256; ++i) {
        char pair[3];
        snprintf(pair, sizeof(pair), "%02X", (u8)binary[i]);
        assert(!memcmp(copied_hex + 3 * i, pair, 2));
    }

    editor_hex_key(&hex_view, &document, (ui_input){.character = 'F'});
    assert(!document.dirty && !memcmp(document.text, binary, sizeof(binary)));

    editor_scrollbar hex_bar = editor_scrollbar_layout(640, 48, 128, 8, 17);
    assert(editor_scroll_rows(&hex_view, hex_bar, 640, &document.hex_first,
                              (ui_input){.mouse = UI_MOUSE_DOWN, .x = hex_bar.x, .y = 49}));
    assert(editor_scroll_rows(&hex_view, hex_bar, 640, &document.hex_first,
                              (ui_input){.mouse = UI_MOUSE_UP, .x = 650, .y = 250}));
    assert(document.hex_first == 9 && !hex_view.scrolling);

    document.full_hex = true;
    editor_hex_key(&hex_view, &document, (ui_input){.key = UI_KEY_A, .control = true});
    editor_hex_copy(&document, NULL);
    assert(strlen(copied_hex) == 3 * sizeof(binary) - 1);
    editor_hex_key(&hex_view, &document, (ui_input){.key = UI_KEY_END, .control = true});
    editor_hex_key(&hex_view, &document, (ui_input){.character = 'F'});
    editor_hex_key(&hex_view, &document, (ui_input){.character = '1'});
    assert(document.cursor == 256 && (u8)document.text[256] == 0xf1 && document.dirty);
    editor_save(&document, path);
    editor_undo(&document);
    assert(document.dirty && document.low_nibble);
    editor_undo(&document);
    assert(!memcmp(document.text, binary, sizeof(binary)) && !document.low_nibble && document.dirty);
    editor_tabs mixed_tabs = {0};
    assert(editor_tabs_open(&mixed_tabs, &document, &settings, path));
    assert(editor_tabs_new(&mixed_tabs, &document, &settings));
    editor_insert_character(&document, 'A');
    editor_insert_character(&document, 'B');
    editor_undo(&document);
    assert(!strcmp(document.text, "A"));

    editor_tabs_step(&mixed_tabs, &document, &settings, -1);
    editor_redo(&document);
    assert(document.binary && document.low_nibble && document.dirty);

    editor_tabs_step(&mixed_tabs, &document, &settings, 1);
    editor_redo(&document);
    assert(!strcmp(document.text, "AB") && document.cursor == 2);
    editor_undo(&document);
    editor_insert_character(&document, 'C');
    editor_redo(&document);
    assert(!strcmp(document.text, "AC"));
    editor_undo(&document);
    editor_undo(&document);
    assert(!document.length && !document.dirty);
    editor_tabs_close(&mixed_tabs, mixed_tabs.active, &document, &settings);

    editor_redo(&document);
    assert(document.binary && (u8)document.text[256] == 0xf1 && !document.dirty);
    editor_undo(&document);
    assert(document.low_nibble && document.dirty);
    assert(editor_load(&document, path) && (u8)document.text[256] == 0xf1);
    assert(sys_file_write_atomic(path, "test", 4) && editor_load(&document, path));
    editor_tabs tabs = {0};
    assert(editor_tabs_open(&tabs, &document, &settings, path));
    document.cursor = 1;
    document.anchor = 3;
    document.selecting = true;
    editor_insert_character(&document, 'Q');
    editor_undo(&document);
    assert(!strcmp(document.text, "test") && !document.dirty && document.selecting &&
           document.cursor == 1 && document.anchor == 3);
    document.cursor = 0;
    document.selecting = false;
    editor_insert_character(&document, 'T');
    size_t cursor = document.cursor;
    char second[MAX_PATH];
    assert(GetTempFileNameA(directory, "tab", 0, second));
    assert(editor_tabs_open(&tabs, &document, &settings, second));
    assert(!document.dirty && editor_tabs_dirty(&tabs, &document));
    editor_tabs_step(&tabs, &document, &settings, -1);
    assert(document.dirty && document.cursor == cursor && document.text[cursor - 1] == 'T');
    assert(editor_tabs_open(&tabs, &document, &settings, second) && tabs.count == 2);
    editor_view preferences = {0};
    editor_document preference_doc = {0};
    editor_settings preferences_settings = editor_settings_init();
    assert(editor_settings_tab(&preferences, &preference_doc, &preferences_settings));
    assert(editor_switch_settings(&preferences, &preference_doc, &preferences_settings, 0));
    assert(preferences.tabs.items[0].settings_json && strstr(preference_doc.text, "syntax.c_enabled"));
    preference_doc.text[0] = '!';
    preference_doc.dirty = true;
    assert(!editor_switch_settings(&preferences, &preference_doc, &preferences_settings, 0));
    assert(preferences.tabs.items[0].settings_json && preference_doc.dirty && preference_doc.open_error);
    preference_doc.text[0] = '{';
    preference_doc.dirty = false;
    assert(editor_switch_settings(&preferences, &preference_doc, &preferences_settings, 0));
    assert(preferences.tabs.count == 1 && preferences.tabs.items[0].settings_page && !preference_doc.text);

    char json[4096], debugger_path[260] = "C:\\debug\\gdb.exe";
    editor_theme parsed = {0};
    assert(editor_settings_json_format(&settings.theme, debugger_path, json, sizeof(json)));
    debugger_path[0] = 0;
    assert(editor_settings_json_parse(json, &parsed, debugger_path));
    assert(!memcmp(&parsed, &settings.theme, sizeof(parsed)) && !strcmp(debugger_path, "C:\\debug\\gdb.exe"));
    assert(!editor_settings_json_parse("{\"syntax.enabled\":0,\"syntax.enabled\":1}", &parsed, debugger_path));
    assert(!memcmp(&parsed, &settings.theme, sizeof(parsed)));
    const char* legacy = "background=#101010\npanel=#202020\ntext=#ffffff\nheading=#ffffff\n"
                         "muted=#aaaaaa\ncaret=#ffeeee\nselection=#334455\n";
    assert(sys_file_write_atomic(second, legacy, strlen(legacy)));
    assert(editor_theme_load(&parsed, second) && parsed.background == 0x101010);
    assert(parsed.button.radius == settings.theme.button.radius);
    settings.theme.button.radius = 9;
    settings.theme.link.press_percent = 42;
    assert(editor_theme_save(&settings.theme, second));
    assert(editor_theme_load(&parsed, second));
    assert(!memcmp(&parsed, &settings.theme, sizeof(parsed)));
    assert(sys_file_write_atomic(second, "panel=#ffffff", 13));
    assert(!editor_theme_load(&parsed, second));
    assert(!memcmp(&parsed, &settings.theme, sizeof(parsed)));

    editor_tree tree = {.visible = true};
    assert(editor_tree_resize(&tree, 900, 500, (ui_input){.mouse = UI_MOUSE_DOWN, .x = 620, .y = 40}));
    assert(editor_tree_resize(&tree, 900, 500, (ui_input){.mouse = UI_MOUSE_UP, .x = 450, .y = 40}));
    assert(!tree.resizing && editor_tree_width(&tree, 900) == 450);
    assert(editor_tree_width(&tree, 200) == 40);

    assert(editor_tabs_new(&tabs, &document, &settings));
    assert(!document.length && tabs.items[tabs.active].untitled);
    assert(editor_tabs_dirty(&tabs, &document));

    editor_tabs_close(&tabs, tabs.active, &document, &settings);
    assert(tabs.count == 2 && !strcmp(settings.path, second));
    editor_tabs_close(&tabs, tabs.active, &document, &settings);
    assert(tabs.count == 1 && document.dirty && document.cursor == cursor);
    document.dirty = false;
    editor_tabs_close(&tabs, 0, &document, &settings);
    assert(tabs.count == 1 && tabs.items[0].untitled && !document.length);

    editor_document_release(&reloaded);
    editor_document_release(&document);
    assert(DeleteFileA(second));
    assert(DeleteFileA(path));
    assert(sys_folder_create(path));
    char child[SYS_PATH_CAPACITY], copy[SYS_PATH_CAPACITY], moved[SYS_PATH_CAPACITY];
    assert(!sys_child_path(path, "..", child) && !sys_child_path(path, "a/b", child));
    assert(!sys_child_path(path, "NUL.txt", child));
    assert(sys_child_path(path, "note.txt", child) && sys_child_path(path, "copy.txt", copy));
    assert(sys_child_path(path, "renamed.txt", moved));
    assert(sys_file_create_empty(child) && !sys_file_create_empty(child));
    assert(sys_file_write_atomic(child, "abc", 3));
    assert(sys_file_copy_new(child, copy) && !sys_file_copy_new(child, copy));
    assert(!sys_entry_rename(child, copy) && sys_entry_rename(copy, moved));

    editor_tree explorer = {0};
    editor_tree_init(&explorer, child);
    assert(explorer.visible && explorer.count == 2 && !strcmp(explorer.selected, child));
    assert(editor_tree_root(&explorer, directory));
    assert(editor_tree_follow(&explorer, child) && !strcmp(explorer.root, directory));
    assert(editor_tree_follow(&explorer, moved) && !strcmp(explorer.selected, moved) && explorer.count == 2);
    editor_tree_destroy(&explorer);

    editor_projects projects = {0};
    editor_tree project_tree = {0};
    assert(editor_project_follow(&projects, &project_tree, moved));
    assert(projects.count == 1 && projects.active == 0);
    char nested[SYS_PATH_CAPACITY], nested_file[SYS_PATH_CAPACITY];
    assert(sys_child_path(path, "nested", nested) && sys_folder_create(nested));
    assert(sys_child_path(nested, "nested.c", nested_file));
    assert(editor_project_follow(&projects, &project_tree, nested_file));
    assert(projects.count == 1 && !_stricmp(projects.roots[0], path));
    assert(editor_project_open(&projects, &project_tree, directory) && projects.count == 2);
    assert(editor_project_open(&projects, &project_tree, path) && projects.active == 0);
    assert(RemoveDirectoryA(nested));
    editor_tree_destroy(&project_tree);
    u64 bytes = 0;
    assert(sys_file_size(moved, &bytes) && bytes == 3);
    assert(sys_file_delete(child) && sys_file_delete(moved) && RemoveDirectoryA(path));

    puts("PASS: editor insertion, line navigation, deletion, tabs, control input, capacity, options, load normalization and save.");

    return 0;
}
