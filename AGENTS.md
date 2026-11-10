# GreenC contributor guidance

Read and follow [PRINCIPLES.md](PRINCIPLES.md), especially its type-selection
rules. They take precedence over legacy uses of built-in numeric types.

## Review every change

Before responding to each implementation request, review the complete resulting
diff as a maintainer who has not read the chat or original specification.
Check correctness, resource lifetimes, failure paths, naming, scope, build
instructions, and consistency with nearby code. Fix actionable findings within
the requested scope before delivery. For requests without code changes, apply
the same review to the proposed guidance or artifact; do not manufacture edits.
Report validation accurately and mention any material unresolved findings.

Code should explain its purpose through names, structure, and useful comments.
Do not depend on the prompt to explain an ambiguous helper or a surprising
behavior. Keep changes focused and readable; avoid speculative abstractions.

## Follow this repository

- Inspect neighboring files and existing call sites before choosing a pattern.
  Established local conventions take precedence over generic style preferences.
  When patterns differ, follow the closest relevant subsystem and explain any
  necessary departure. Do not copy an existing correctness defect.
- C functions and variables generally use `snake_case`. Runtime/compiler APIs
  use `greenc_`; narrower modules use meaningful domain prefixes such as `sys_`
  and `chan_`. Choose a prefix that communicates responsibility.
- Prefer concrete names such as `ui_window_proc`, `ui_window_create_fullscreen`,
  and `ui_window_poll_events` over `example_window_proc` or generic helpers such
  as `handle`, `process`, or `utils`. Name the behavior a caller needs to know.
- Match nearby C layout: four-space indentation, same-line opening braces,
  `const char*` pointer spelling, and `#pragma once` for shared headers.
- Separate logical groups with blank lines. After an `if` block or a completed
  group of guard clauses, leave a blank line before the next operation. Usually
  put a blank line after `}` when more work follows; keep connected `else`
  branches and enclosing closing braces together. Closely related checks or
  operations may form compact groups of around five lines when that reads
  naturally. Expand multi-statement inline blocks so control flow is easy to
  scan. Use meaningful grouping rather than adding blank lines mechanically.
- Always write `if` / `else` bodies as multiline braced blocks, including
  single-statement guards. Keep C parentheses, but use the explicit block shape
  familiar from Go: `if (condition) {` followed by an indented body and `}`.
  Never write `if (condition) return ...;` or a complete `if` on one line.
- Leave a blank line before `return`, `continue`, `break`, or `goto` when it
  follows other work in the same block. Do not add an empty line immediately
  after an opening brace when the jump is the block's only statement.
- Keep ownership and cleanup explicit. GreenC source uses the documented
  `///move`, `///read`, and `///stack` contracts where applicable; native C
  examples must not claim GreenC checking they do not receive.
- Preserve the documented `_create` / `_init` distinction: creation returns an
  owned resource, while initialization prepares a value. Pair each acquired
  resource with its appropriate release operation on success and failure paths.

## Examples

- Examples should be small, complete programs a reader can build and adapt.
  Use descriptive directory names within the existing platform layout:
  `examples/win/cpui` for CPU interfaces and `examples/win/gpui` for GPU interfaces.
- Keep example `main.c` files independent of platform APIs: use only `ui_`
  calls for windowing, input, timing, allocation, presentation, and cleanup.
  Native handles, Win32 types/constants, and OpenGL calls belong in backend
  implementation files, not application code or the public API.
- Declare the portable interface in `examples/win/ui_window.h`. Use opaque
  `ui_window` and `ui_gpu` handles; expose CPU pixel data through a documented
  portable framebuffer. Keep Windows details in `*_win32.c` and the private
  `ui_window_win32.h`. Use internal linkage for backend-only helpers such as
  `ui_window_proc`. Keep the CPU animation in the example, where its intent is
  visible. Do not replace the entire example with a single opaque demo runner.
- Keep GPU geometry, colors, background, and animation choices in the program
  too. Backend drawing primitives accept caller-supplied data and preserve
  coordinate proportions; they must not hard-code demo content. Check geometric
  claims such as equal triangle sides independently of successful compilation.
- Design the API so another backend can implement it without changing the
  examples. Implement only the requested platforms; do not imply Linux support
  until a Linux backend exists. Document ownership, borrowed lifetimes, and the
  current single-window event-loop assumption.
- Document build/run commands, controls, platform requirements, rendering
  backend, and meaningful limitations. Distinguish borderless fullscreen from
  exclusive display modes and CPU rendering from accelerated presentation.
- Keep input handling and resource lifetimes easy to follow. Handle setup and
  rendering failures, permit normal close/escape behavior, and release renderer
  resources before destroying the window.
- Pace active rendering with `ui_window_wait_frame` and the user-changeable
  program-local `target_fps` setting (initially 240), passed to window creation.
  `UI_DEFAULT_TARGET_FPS` is the library fallback, not the application's
  customization point. Account for rendering time and
  fractional frame intervals; do not use a fixed millisecond sleep as a frame
  limiter. Keep animation speed independent of the chosen render rate. Treat
  application FPS and monitor refresh/vsync as separate concerns.
- Maintain the library/application ownership boundary described in
  [PRINCIPLES.md](PRINCIPLES.md): shared API and backend files provide mechanisms
  and defaults; each program owns its settings and behavior. Do not require
  callers to edit shared headers or rebuild backends with application-specific
  preprocessor definitions to customize a program.
- Build changed native examples with their Makefiles (`-Wall -Wextra -Werror`).
  For window behavior changes, smoke-test startup, fullscreen dimensions, and
  shutdown when a Windows desktop is available. State what was actually tested;
  compilation alone does not verify rendering. Use focused checks rather than
  tests that merely repeat the implementation.
- Generated `.exe` and local `.bat` files stay ignored and untracked. Do not
  commit build artifacts. Do not include unrelated working-tree edits.
