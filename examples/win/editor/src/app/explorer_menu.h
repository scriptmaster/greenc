/*
 ** explorer_menu.h - 4 functions.
 ** explorer_machine_set - Change the all-users classic commands from an elevated helper.
 ** explorer_request_admin - Request elevation for an explicit all-users settings action.
 ** explorer_menu_enabled - Check whether this executable owns the folder command.
 ** explorer_menu_set - Register or remove the current user's folder command.
 ** See also: explorer_menu_win32.c, theme_input.c.
 */
#pragma once
#include <stdbool.h>

bool explorer_menu_enabled(void);
bool explorer_menu_set(bool enabled);

bool explorer_machine_set(bool enabled);
bool explorer_request_admin(bool enabled);
