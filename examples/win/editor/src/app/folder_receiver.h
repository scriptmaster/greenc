/*
 ** folder_receiver.h - 3 functions.
 ** editor_forward_folder - Ask an existing editor to activate a folder.
 ** editor_listen_folders - Accept Explorer requests while the workspace is live.
 ** editor_stop_folders - Release the receiver before workspace teardown.
 ** See also: folder_receiver_win32.c, editor_loop.c.
 */
#pragma once
#include "editor_view.h"

bool editor_forward_folder(const char* folder);
void editor_listen_folders(editor_view* view);
void editor_stop_folders(void);
