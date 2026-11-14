/*
 ** file_size.h - 1 function.
 ** editor_format_size - Format a byte count using decimal units.
 ** See also: file_size.c.
 */
#pragma once
#include "sys/builtin/types_intrinsic.h"

void editor_format_size(char result[16], u64 bytes);
