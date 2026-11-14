/*
 ** menu_commands.c - 1 function.
 ** editor_menu_input - Translate native menu commands into editor bindings.
 ** See also: editor_dispatch.h, main_internal.h.
 */
#include "main_internal.h"

const ui_menu_item editor_menu[] = {
    {"&File", "&New File\tCtrl+N", MENU_NEW},
    {"&File", "&Open File...\tCtrl+O", MENU_OPEN},
    {"&File", "&Save\tCtrl+S", MENU_SAVE},
    {"&File", "Save &As...\tCtrl+Shift+S", MENU_SAVE_AS},
    {"&File", "&Quit\tCtrl+Q", MENU_QUIT},
    {"&Edit", "Cu&t\tCtrl+X", MENU_CUT}, {"&Edit", "&Copy\tCtrl+C", MENU_COPY},
    {"&Edit", "&Paste\tCtrl+V", MENU_PASTE}, {"&Edit", "Select &All\tCtrl+A", MENU_ALL},
    {"&Edit", "&Settings...", MENU_SETTINGS},
    {"&View", "&Files\tCtrl+B", MENU_FILES}, {"&View", "&Terminal", MENU_TERMINAL}
};

ui_input editor_menu_input(u16 command) {
    static const ui_key keys[] = {UI_KEY_NONE, UI_KEY_S, UI_KEY_Q, UI_KEY_X, UI_KEY_C,
                                  UI_KEY_V, UI_KEY_A, UI_KEY_B, UI_KEY_GRAVE, UI_KEY_NONE,
                                  UI_KEY_N, UI_KEY_O, UI_KEY_S};
    return (ui_input){.control = true, .shift = command == MENU_SAVE_AS, .key = command < sizeof(keys) / sizeof(keys[0]) ? keys[command] : UI_KEY_NONE};
}
