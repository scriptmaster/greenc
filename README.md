# GreenC

greenc is valid, modern, safe c language with `-nostdlib -fno-builtin -ffreestanding` flags and a built-in safe lib.

All functions are available in `libgreenc.h`

Macro based C compiler with built-in borrow checker

```
.g extension for the version without macros and .cs like syntax.
.gc for migrating/mixing exiting C code along with the new greenc features.
.gy extension for a python like syntax. Thank goodness we missed a = "py" like syntax.
```

## Smallest Binaries

Produces tiny binaries (< 30 KB) with Clean C code, that is readable and maintainable.

Compiles to C (no IR, LLVM), uses gcc or clang or cl.exe
