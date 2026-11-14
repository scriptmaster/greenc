#pragma once
#include "../builtin/types_intrinsic.h"
#include <stdbool.h>

enum { SYS_PATH_CAPACITY = 260 };
typedef bool (*sys_directory_visit)(void* context, const char* name, bool directory, bool hidden);
typedef struct sys_folder_watch sys_folder_watch;
sys_folder_watch* sys_folder_watch_create(const char* path);
bool sys_folder_watch_changed(sys_folder_watch* watch);
void sys_folder_watch_destroy(sys_folder_watch* watch);

bool sys_parent_directory(const char* path, char result[SYS_PATH_CAPACITY]);
bool sys_directory_each(const char* path, sys_directory_visit visit, void* context);
