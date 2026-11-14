#pragma once
#include "builtin/types_intrinsic.h"
#include <stdbool.h>

typedef struct sys_shell sys_shell;
sys_shell* sys_shell_create(const char* executable);
/* Options are a trusted, Windows-quoted argument string. */
sys_shell* sys_shell_create_with_options(const char* executable, const char* options);
bool sys_shell_send(sys_shell* shell, const char* line, size_t length);
size_t sys_shell_read(sys_shell* shell, char* output, size_t capacity);
bool sys_shell_running(sys_shell* shell);
void sys_shell_destroy(sys_shell* shell);
