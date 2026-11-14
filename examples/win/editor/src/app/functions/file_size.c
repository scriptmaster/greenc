/*
 ** file_size.c - 1 function.
 ** editor_format_size - Show exact bytes below 1000 and one decimal place for larger units.
 ** See also: file_size.h.
 */
#include "file_size.h"
#include <stdio.h>

void editor_format_size(char result[16], u64 bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    if (bytes < 1000) {
        snprintf(result, 16, "%u B", (u32)bytes);

        return;
    }

    size_t unit = 1;
    u64 divisor = 1000;
    while (bytes / divisor >= 1000) {
        divisor *= 1000;
        ++unit;
    }

    u32 tenths = (u32)(bytes / divisor) * 10 + (u32)(((bytes % divisor) + divisor / 20) / (divisor / 10));
    if (tenths == 10000) {
        tenths = 10;
        ++unit;
    }

    snprintf(result, 16, "%u.%u %s", tenths / 10, tenths % 10, units[unit]);
}
