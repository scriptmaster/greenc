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


Two worded types are heap allocated, otherwise stack allocated.

string list files = greenc_list_files(cwd);

types:
dictionary
queue
ringbuffer

# Greenc Roadmap (Emit-C Backend)

## 🎯 Vision

Greenc is:
- A **memory-safe systems language**
- With **borrow semantics (Rust-like)**
- Compiling to **C (Emit-C backend)**
- With a **minimal libc replacement runtime**
- And a **rich native capability layer (AI, HTTP, GUI)**

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
- [ ] filtering (`.c`, `.h`, `.gc`)
- [ ] file classification
- [ ] build graph struct

## CLI
- [ ] `greenc build .`
- [ ] `-o output`
- [ ] argument parsing

---

# 🧠 Phase 2 — Language Core

## Lexer
- [ ] tokens:
  - identifiers
  - numbers
  - strings
  - keywords (`fn`, `let`, `return`)
  - operators/symbols

## Parser (minimal)
- [ ] AST nodes:
  - function
  - variable
  - return
- [ ] block parsing
- [ ] expression parsing (basic)

---

# 🧠 Phase 3 — Semantic Layer

## Type System
- [ ] primitive types (`int`, `string`)
- [ ] variable declarations
- [ ] function signatures

## Borrow Checker (core differentiator)
- [ ] ownership rules
- [ ] borrow vs move
- [ ] lifetime tracking (minimal)
- [ ] compile-time errors

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
- [ ] strings
- [ ] file I/O
- [ ] path utilities

## Extensions
- [ ] JSON (parse + serialize)
- [ ] env variables
- [ ] time

---

# 🚀 Phase 7 — Runtime Capabilities (Your MVP Features)

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

# 🤖 Phase 8 — AI Integration (IMPORTANT)

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
- [ ] build caching
- [ ] package system (optional)

---

# 🧭 Development Strategy

## Strict Order (critical)

1. FS + build graph ✔
2. Sorting + filtering
3. Lexer
4. Parser
5. Borrow checker
6. Emit-C backend
7. Runtime libraries

## Parallel Tracks (optional later)

- HTTP server
- AI integration
- GUI

---

# 🧠 Core Philosophy

- No libc dependency (initially)
- Deterministic builds
- Explicit memory model
- Clean separation:
  - compiler
  - runtime
  - libraries

---

# 🎯 MVP Definition

Greenc MVP =

- compile `.gc` → C → binary
- borrow-checked functions
- minimal stdlib
- file-based modules

Optional MVP+:
- HTTP server
- AI model loading

---

# 🚀 Future

- direct backend (no C)
- JIT / REPL
- cross-compilation toolchain
- package ecosystem
