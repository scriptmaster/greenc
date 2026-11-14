/*
 ** modified_time.h - 1 function.
 ** editor_modified_label - Describe elapsed time using local calendar dates.
 ** See also: modified_time.c, editor_status.c.
 */
#pragma once
#include <time.h>

void editor_modified_label(char label[64], time_t modified, time_t now);
