#pragma once
#include "directory.h"

/* Join a single ordinary Windows filename to its directory. Mutations never overwrite a destination. */
bool sys_child_path(const char* folder, const char* name, char path[SYS_PATH_CAPACITY]);
bool sys_file_create_empty(const char* path);
bool sys_folder_create(const char* path);
bool sys_file_copy_new(const char* source, const char* target);
bool sys_entry_rename(const char* source, const char* target);
bool sys_file_delete(const char* path);
/* Follow file links. Modification time is expressed in Unix seconds. */
bool sys_file_details(const char* path, u64* bytes, i64* modified);
bool sys_file_size(const char* path, u64* bytes);
