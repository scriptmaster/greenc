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
- Prefer concrete names such as `ui_window_proc`, `ui_window_create`,
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
- Always brace `if` / `else` bodies. Default to multiline; a trivial braced
  condition with exactly one simple action may fit on one readable line.
  Follow the detailed braces and whitespace rules below.
- Leave a blank line before `return`, `continue`, `break`, or `goto` when it
  follows other work in the same block. Do not add an empty line immediately
  after an opening brace when the jump is the block's only statement.
- Keep ownership and cleanup explicit. GreenC source uses the documented
  `///move`, `///read`, and `///stack` contracts where applicable; native C
  examples must not claim GreenC checking they do not receive.
- Preserve the documented `_create` / `_init` distinction: creation returns an
  owned resource, while initialization prepares a value. Pair each acquired
  resource with its appropriate release operation on success and failure paths.

## Stable library boundary

The UI architecture lives in `compiler/libs/ui/`; filesystem mechanisms live in
`compiler/libs/sys/fs/`. Example programs live under `examples/win/`.
Treat public contracts and backend mechanisms as frozen for ordinary application
requests. Change them only when the user explicitly requests library/architecture
work (including a named backend defect). A request to change an editor, theme,
FPS, shortcut, or window preference alone does not authorize library changes.
If the API cannot express a requested program feature, explain the missing
capability before broadening the architecture. Do not silently add abstractions,
rename APIs, or refactor backends as part of example work.

Library owns: opaque resources, validated configuration, OS translation, event
transport, timing/deadlines, canvas primitives, renderer selection/failure
recovery, allocation/cleanup, and generic atomic file replacement. Keep file I/O
in the filesystem module, not the UI renderer.

Program owns: chosen FPS and runtime changes, window mode/dimensions/decorations,
renderer preference, title, theme, layout, geometry, document model, editing,
file paths, save/quit policy, input bindings, animation, and status messages.
Pass choices as values through public APIs. No application names, preferred FPS,
editor layouts, or demo geometry belong in backend files. Zero FPS means
unpaced; the library must not pick a preferred application frame rate.

For explicitly authorized architecture work, update its contracts and ownership
documentation, migrate all callers, and validate CPU-only, GPU/automatic fallback,
window modes, input, resize, runtime FPS changes and failure cleanup before
considering the boundary stable again. See the library's own AGENTS.md.

## Examples

- Examples should be small, complete programs a reader can build and adapt.
  Use descriptive directory names within the existing platform layout:
  `examples/win/cpui` for CPU interfaces and `examples/win/gpui` for GPU interfaces.
- Keep example `main.c` files independent of platform APIs: use only `ui_`
  calls for windowing, input, timing, allocation, presentation, and cleanup.
  Native handles, Win32 types/constants, and OpenGL calls belong in backend
  implementation files, not application code or the public API.
- Declare the portable interface in `compiler/libs/ui/ui_window.h`. Use opaque
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
  program-owned `target_fps` setting, supplied in `ui_window_config` or changed
  with `ui_window_set_target_fps`. Zero means unpaced; the library has no
  preferred FPS. Account for rendering time and
  fractional frame intervals; do not use a fixed millisecond sleep as a frame
  limiter. Keep animation speed independent of the chosen render rate. Treat
  application FPS and monitor refresh/vsync as separate concerns.
- Maintain the library/application ownership boundary described in
  [PRINCIPLES.md](PRINCIPLES.md): shared API and backend files provide mechanisms;
  each program owns its settings and behavior. Do not require
  callers to edit shared headers or rebuild backends with application-specific
  preprocessor definitions to customize a program.
- Build changed native examples with their Makefiles (`-Wall -Wextra -Werror`).
  For window behavior changes, smoke-test startup, fullscreen dimensions, and
  shutdown when a Windows desktop is available. State what was actually tested;
  compilation alone does not verify rendering. Use focused checks rather than
  tests that merely repeat the implementation.
- Generated `.exe` and local `.bat` files stay ignored and untracked. Do not
  commit build artifacts. Do not include unrelated working-tree edits.

## Maintainability review after every request

Apply this standard to current work and future projects. Before the final reply
for every request, review the complete resulting change as a maintainer who has
not read the chat. For advice-only requests, review the proposed guidance; do not
manufacture code changes. Fix findings within the authorized scope, run checks
appropriate to the risk, and report actual validation and material limitations.

Keep stable architecture separate from application policy. Library mechanisms,
resource ownership, platform adaptation and failure recovery belong behind
small explicit contracts. Settings, defaults chosen for a program, layouts,
themes, document behavior and input bindings belong to the program. Treat an
established architecture as frozen unless the request explicitly asks to change
it or fix a named architectural/backend defect. Report missing API capabilities
before broadening application work into architecture work.

Code should read like clear English paragraphs: descriptive domain names,
small cohesive operations, one abstraction level per function, early guards,
and a short orchestration function whose calls explain the workflow.
Group by responsibility. Split a function/module when its responsibilities,
nesting or context switching make it difficult to read, not merely to satisfy
a line-count target. Keep ownership, failure handling and cleanup visible.

Use useful techniques familiar from maintainable .NET code where they fit the
language: explicit options/configuration objects, composition, separation of
model/presentation/platform code, narrow interfaces and dependency injection
through ordinary parameters. In C, use structs, functions and opaque handles;
do not imitate framework machinery, add service locators, or introduce patterns
without a concrete benefit. Prefer enough abstraction to express current
requirements clearly over speculative extension points.

Expose meaningful application choices through parameters/options and runtime
setters where change is needed: frame rate, window mode/dimensions/decorations,
renderer preference, font, theme and layout. Keep related settings together.
Do not turn every implementation constant into a setting. Do not require users
to edit library internals or compiler macros to customize an application.

### Braces and whitespace

- Always brace if/else bodies, using the Go-like explicit shape appropriate to
  the language (C keeps parentheses). Default to multiline blocks.
- A trivial if may occupy one line only when its braced body contains one
  simple statement and the entire condition/action is easy to read. Never pack
  multiple statements, nested decisions, or complex branches onto one line.
- After a completed if/else, loop, switch or other block, leave a blank line
  before the next independent operation. Keep connected else/catch/finally
  clauses attached. Do not add blank lines between consecutive closing braces.
- Leave a blank line before return, continue, break or goto when it follows
  other work in the same block. A guard whose only statement is the jump
  needs no extra blank line inside the block.
- Group related guards and related operations together. Aim for readable
  groups of roughly five short lines/items when natural; use a blank line
  between groups. This is a readability guide, not a mechanical quota.
- Separate completed logical groups, not every statement. Avoid blank lines
  immediately after opening braces, duplicate empty lines, and empty space
  that separates an explanation from the operation it explains.
- Match the project's indentation and formatting. Review the final source
  visually as well as checking its behavior.

The final review must assess correctness, architectural boundaries, sufficient
but restrained abstractions, readable orchestration, function/module cohesion,
naming, braces/spacing, configuration ownership, resource lifetimes, failure
paths, security, documentation, and meaningful tests. Do not claim perfection
or tests that were not run.

## GreenC application layout and file documentation

Keep the executable entry point in `src/main.c`. After its file documentation and includes, define only `main`. Delegate startup, the event loop and resource cleanup to application modules. Keep all application headers directly under `src/app/`. Group implementation files by responsibility:

- `app/core/` for startup, configuration and application-wide commands.
- `app/entities/` for owned models, state and invariants.
- `app/views/` for presentation and layout; `app/views/ui/` for reusable controls.
- `app/input/` for keyboard, mouse and command dispatch.
- `app/process/` for workflows, file operations and session lifetimes.
- `app/functions/` for focused, genuinely shared utility functions.

Use at most five function definitions per application source file. Aim for three to five cohesive operations where natural. A smaller module is appropriate for a complete, narrow responsibility. Keep headers focused, with related declarations together. Tests belong under `test/` and compiled outputs under `bin/`.

Start every application .c and .h file with a documentation block. Count definitions in a source and declarations in a header. For a types-only or aggregate header, record zero and describe its role. Give each function one line explaining its purpose, then list real related files under See also. Keep the inventory and references current whenever a function moves. Use the following shape:

```c
/*
 ** editor_start.c - 1 function.
 ** editor_start - Acquire application resources and run the editor.
 ** See also: editor_start.h, main.c.
 */
```

Apply these conventions to current and future GreenC applications. Preserve the stable library boundary and keep program policy in the application.

### Group declarations, guards, and actions

Use a blank line when a declaration or setup group gives way to a guard, loop, or independent action. Group adjacent guards that validate the same operation. Leave a blank line after that group before starting the next calculation. Apply the same separation inside nested blocks. Keep a single-purpose guard compact, keep else attached, and keep consecutive closing braces together. A return that follows work in the same block starts a new paragraph. Preserve short groups of closely related statements when splitting them would obscure their relationship.
