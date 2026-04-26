#pragma once

// INCLUDE this file ONLY if:   -nostdlib -static-libgcc -ffreestanding -fno-builtin
// otherwise include only greenc.h

#include "greenc.h"

/*
 * For external use:
 *
 *    defines prefix-less shims for all greenc_* and greenc_*
*/

#define strlen greenc_strlen
#define concat greenc_concat
#define tree   greenc_tree
#define free   greenc_free
#define mfree  greenc_mfree
#define release greenc_release

#define string_startswith greenc_string_startswith
#define string_endswith greenc_string_endswith

// ...
