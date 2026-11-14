#pragma once
#include "../builtin/types_intrinsic.h"
#include <stdbool.h>

/* Windows native C implementation. Borrowed path/data for this call only.
 * Replaces the destination only after a complete, flushed sibling-temp write.
 * Returns false on failure and removes the temporary file where possible.
 * ANSI paths up to MAX_PATH; no promise of crash consistency on all filesystems. */
bool sys_file_write_atomic(const char* path, const char* data, size_t length);
