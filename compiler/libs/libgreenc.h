#pragma once

// INCLUDE this file if:   -nostdlib -static-libgcc -ffreestanding -fno-builtin
// otherwise include only greenc.h

#include "greenc.h"

/*
 * For external use:
 *
 *    defines prefix-less shims for all gc_* and greenc_*
*/

#define strlen greenc_strlen
#define concat greenc_concat
#define tree   gc_tree
#define free   greenc_free

// ...
