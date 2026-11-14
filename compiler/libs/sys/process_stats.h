/*
 ** process_stats.h - 1 function.
 ** sys_process_sample - Read private working-set bytes, process CPU time and logical processor count.
 ** See also: process_stats_win32.c.
 */
#pragma once
#include "builtin/types_intrinsic.h"
#include <stdbool.h>

typedef struct { u64 private_resident_bytes, cpu_100ns; u32 processors; } sys_process_stats;
bool sys_process_sample(sys_process_stats* sample);
