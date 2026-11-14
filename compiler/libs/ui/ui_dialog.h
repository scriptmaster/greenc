#pragma once
#include "ui_window.h"

/* Paths use the native ANSI path limit. Cancellation leaves outputs unchanged. */
bool ui_choose_file(ui_window* window, bool save, const char* extension, char path[260]);
typedef struct { const char* label; u16 command; } ui_popup_item;
u16 ui_popup_menu(ui_window* window, i32 x, i32 y, const ui_popup_item* items, size_t count);
bool ui_choose_folder(ui_window* window, char path[260]);
bool ui_show_properties(ui_window* window, const char* path);
bool ui_open_folder(ui_window* window, const char* path);
void ui_show_info(ui_window* window, const char* title, const char* message);
bool ui_choose_color(ui_window* window, u32* rgb);
