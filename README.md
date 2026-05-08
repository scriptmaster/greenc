# GreenC

GreenC = C with enforced safety discipline

GreenC is a modern, memory-safe extension of C designed to work with:

```
-nostdlib -fno-builtin -ffreestanding
```

All functions are available in `libgreenc.h`

Macro based C compiler with built-in borrow checker

It compiles to clean C (Emit-C backend) and produces small deterministic binaries., for use in servers to desktops to micro-controllers.

---

## Core Philosophy

C, explicit ownership, contracts, compile-time safety

No hidden behavior. No implicit ownership. Everything is explicit.

---


## Smallest Binaries

Produces tiny binaries (< 30 KB) with Clean C code, that is readable and maintainable.

Compiles to C (no IR, LLVM), uses gcc or clang or cl.exe

## Ownership Model

```c
///move   → owns heap (must free exactly once)
///read   → alias (must not free)
///stack  → value (no heap)
```

Missing marker → compile error

---

## Pointer Rules

- All `*` pointers are tracked
- No RC/GC runtime
- Every pointer must be:
  - freed at last usage OR
  - freed at end of scope

---

## Naming Conventions

```
_create     → heap (///move)
_init       → stack (///stack)
_to_string  → heap (///move)
_str        → lightweight / borrowed
_new        → allowed ONLY for heap (///move)
```

---

## Example

```c
char* buf = buffer_create(); ///move
greenc_free(buf);

string_view s = string_view_init("hello"); ///stack
char* heap = string_view_to_string(s); ///move
greenc_free(heap);
```

---

## Chaining Rules

❌ Invalid:
```c
char* x = f(g(h(i)));
```

✔ Valid:
```c
char* a = h(i); ///move
char* b = g(a); ///move
greenc_free(a);

char* x = f(b); ///move
greenc_free(b);
```

---

## Escape Rules

Only annotated functions may return owned values:

```c
///make_buffer:
char* make_buffer() /* move: greenc_free */
{
    char* buf = alloc(); ///move
    return buf;
}
```

---

## string_view (Slices)

```c
typedef struct {
    const char* data;
    int len;
} string_view;
```

Rules:
- always ///read
- never freed
- cannot outlive source
- cannot be ///move

---

## Invalid Slice

```c
string_view bad() {
    char* tmp = read(); ///move
    string_view s = { tmp, 5 };
    greenc_free(tmp);
    return s; // ERROR
}
```

---

## Type Ownership

```c
///string_list:
struct string_list {
    char** items; ///move
}

/* move: greenc_string_list_free */
```

---

## Memory Report

Each function produces:

```
Assignments: N
Moves: N
Reads: N
Frees: N

Bugs:
Use after free
Double free
Heap escapes
```

---

## Status

### Runtime Core (DONE)
- syscalls
- allocator
- strings
- no libc

### Filesystem (DONE)
- file read
- directory listing
- traversal
- tree

---

## Next

- parser
- borrow checker
- emit-C backend

---

## Roadmap

1. Lexer
2. Parser
3. Checker
4. Codegen
5. Stdlib
6. Networking / AI / GUI

---

# 🧱 Phase 0 — Foundation (DONE / IN PROGRESS)

## Runtime Core (Freestanding)
- [x] Custom entry (`_start`)
- [x] syscalls (Windows + Linux)
- [x] `sys_write`, `sys_exit`
- [x] allocator (`greenc_malloc`, `free`)
- [x] strings (`strlen`, concat)
- [x] no libc dependency

## Filesystem Layer
- [x] `getcwd`
- [x] file read
- [x] directory listing
- [x] recursive traversal
- [x] tree printing
- [x] file collection (dynamic list)

---

# 🧱 Phase 1 — Deterministic Build Layer (NEXT)

## File System Graph
- [ ] sorting (deterministic builds)
- [ ] filtering (`.c`, `.h`)
- [ ] file classification
- [ ] build graph struct

## CLI
- [ ] `greenc build .`
- [ ] `-o output`
- [ ] argument parsing

---

# 🧠 Phase 2 — Language Core

## Lexer
- [x] identifiers
- [x] numbers
- [x] strings
- [x] operators/symbols
- [x] marker comments (`///move`, contracts)

## Parser (minimal)
- [ ] AST nodes:
  - function
  - variable
  - return
- [ ] block parsing
- [ ] expression parsing (basic)
- [ ] marker attachment to AST

---

# 🧠 Phase 3 — Semantic Layer

## Type System
- [ ] primitive types (`int`, `string_view`)
- [ ] variable declarations
- [ ] function signatures
- [ ] struct/type contracts

## Borrow Checker (core differentiator)
- [ ] ownership rules (`///move`, `///read`, `///stack`)
- [ ] pointer tracking (no ref counting)
- [ ] free enforcement (same scope / last use)
- [ ] escape rules
- [ ] chaining rules
- [ ] slice (`string_view`) lifetime rules
- [ ] compile-time diagnostics

# 🧪 Phase 3.5 — Runtime Verification (Sanitizers)

## AddressSanitizer (ASan)
- [ ] integration flag (`-fsanitize=address`)
- [ ] detect:
  - buffer overflow
  - use-after-free (runtime safety net)
  - heap corruption

## UndefinedBehaviorSanitizer (UBSan)
- [ ] integration flag (`-fsanitize=undefined`)
- [ ] detect:
  - null dereference
  - integer overflow
  - invalid casts
  - misaligned access

## Build Modes
- [ ] debug mode → ASan + UBSan enabled
- [ ] release mode → disabled

## Integration
- [ ] `greenc build --sanitize`
- [ ] pass flags to gcc/clang
---

# ⚙️ Phase 4 — Emit-C Backend

## Code Generation
- [ ] AST → C code
- [ ] function emission
- [ ] variable emission
- [ ] control flow

## Runtime Mapping
- [ ] `greenc_*` → C output
- [ ] memory-safe wrappers

## Build Integration
- [ ] call GCC / Clang
- [ ] produce binary

---

# 🧩 Phase 5 — Module System

- [ ] file-based modules
- [ ] import resolution
- [ ] dependency graph
- [ ] incremental build (optional)

---

# 🌐 Phase 6 — Standard Library (Minimal libc replacement)

## Core
- [ ] memory
- [ ] strings / `string_view`
- [ ] file I/O
- [ ] path utilities

## Containers
- [ ] dictionary
- [ ] queue
- [ ] ringbuffer

## Extensions
- [ ] JSON (parse + serialize)
- [ ] env variables
- [ ] time

---

# 🚀 Phase 7 — Runtime Capabilities (MVP Features)

## Networking
- [ ] HTTP server (Windows + Linux)
- [ ] routing
- [ ] static file serving

## Data
- [ ] JSON serialization/deserialization
- [ ] Redis client
- [ ] Postgres client

## Concurrency
- [ ] coroutines (libco integration)
- [ ] async task system

---

# 🤖 Phase 8 — AI Integration

- [ ] dynamic library loading (`.dll` / `.so`)
- [ ] llama.cpp integration
- [ ] model loading APIs
- [ ] inference wrapper
- [ ] memory-safe bindings

---

# 🖥️ Phase 9 — GUI + Graphics

## Native UI
- [ ] window creation
- [ ] input handling

## Rendering
- [ ] WebGL / WebGPU bindings
- [ ] raylib integration
- [ ] shader support

## Assets
- [ ] `.glb` loading
- [ ] texture + mesh pipeline

---

# 🌍 Phase 10 — Embedded Browser

- [ ] WebKit / alternative
- [ ] HTML UI integration
- [ ] bridge (JS ↔ Greenc)

---

# ⚡ Phase 11 — Polish

- [ ] error messages (compiler)
- [ ] diagnostics
- [ ] memory reports (per function)
- [ ] build caching
- [ ] package system (optional)

---

# 🧭 Development Strategy

## Strict Order (critical)

1. FS + build graph ✔
2. Sorting + filtering
3. Lexer ✔
4. Parser
5. Borrow checker
6. Emit-C backend
7. Runtime libraries

## Parallel Tracks (optional later)

- HTTP server
- AI integration
- GUI

---

# 🎯 MVP Definition

Greenc MVP =

- compile `.c` → checked → C → binary
- borrow-checked functions
- minimal stdlib
- deterministic build

Optional MVP+:
- HTTP server
- AI model loading

---

# Rejected:

[X] direct backend. Always readable C.
[X] cross-compilation toolchain (integration with zig or use platform/cicd image).
[X] package ecosystem. C already has a large ecosystem.
