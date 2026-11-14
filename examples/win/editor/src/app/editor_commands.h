/*
 ** editor_commands.h - 0 functions.
 ** Collect the related module declarations.
 ** See also: editor_document.h, editor_input.h, editor_file_actions.h, editor_settings.h, editor_view.h, editor_start.h, editor_dispatch.h, editor_session.h.
 */
#pragma once
#include "editor_document.h"
#include <stdlib.h>
#include "ui/ui_dialog.h"
#include "editor_input.h"
#include "editor_file_actions.h"
#include "editor_settings.h"
#include "editor_view.h"
#include "editor_start.h"

enum { MENU_SAVE = 1, MENU_QUIT, MENU_CUT, MENU_COPY, MENU_PASTE, MENU_ALL, MENU_FILES, MENU_TERMINAL, MENU_SETTINGS, MENU_NEW, MENU_OPEN, MENU_SAVE_AS };

extern const ui_menu_item editor_menu[12];
