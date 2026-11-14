# Native UI library

This is the reusable UI architecture. Programs live in examples/win/. Read the
root and local AGENTS.md before changing this directory. Ordinary program
customization uses the API; architecture changes require an explicit request.

## Responsibilities

| Library mechanism | Application choice |
| --- | --- |
| Native styles and monitor/work-area translation | Window mode, dimensions and decorations |
| Deadline pacing and runtime reset | Initial/current FPS and when it changes |
| Input transport and native close events | Bindings, edits and unsaved-work confirmation |
| Resource ownership and failure recovery | Automatic GPU/CPU or explicit CPU preference |
| Canvas and measured monospaced metrics | Font, theme, content and layout |
| Opaque platform resources | Document model, file paths, save/quit policy |

Atomic file replacement belongs in ../sys/fs/, not the UI renderer. The program
decides when and where to save.

## Window contract

Pass a complete ui_window_config to ui_window_create. Its title/configuration
are borrowed during creation only; the native window copies the title.

- Mode is WINDOWED, MAXIMIZED or FULLSCREEN. Maximized decorated windows use
  the work area; fullscreen covers the primary monitor without exclusive
  display-mode changes. Fullscreen ignores size/decorations.
- Width/height are positive initial client dimensions for non-fullscreen modes.
  Restore from maximized returns to this initial size.
- Title bar, resizability and minimize/maximize/close capabilities are explicit.
  Minimize/maximize require a title bar; maximize requires resizability. Windows
  may show an unavailable caption button disabled alongside its enabled
  counterpart. Close=false disables the system close action. Custom chrome
  is outside this API.
- target_fps=0 means unpaced; 1..1000 controls application deadlines. There is
  no preferred FPS in the library. ui_window_set_target_fps changes pacing
  without recreation, resets deadlines/fractional carry, and leaves settings
  unchanged on invalid input.

All calls run on the creating thread. The loop supports one top-level window.
Poll events, drain ui_window_next_input, render, then wait. Native close becomes
input.close_requested; the program may veto it. Only ui_window_request_close
ends the loop. Escape has no built-in action. The queue holds 256 events and
drops newest input on overflow; close requests are separately coalesced.
Input supports ASCII text, navigation, A-Z, F1-F12 and Control state.
Shift state and primary-button press, drag and release events are available in client coordinates.
Clipboard operations transfer ASCII text through the Windows system clipboard.

## Rendering contract

ui_renderer_create(window, &config) takes explicit backend preference and a
monospaced font face/cell dimensions. Query actual cell metrics for layout.
Font substitution is platform-dependent; invalid/proportional metrics fail
creation. No editor font/theme is selected by the library.

AUTO attempts hardware OpenGL, falling back to GDI when setup fails. A later
GPU presentation failure discards its surface and presents the same canvas
with GDI in that frame. Fallback is sticky for that renderer's lifetime.
CPU skips OpenGL.

The common canvas rasterizes opaque rectangles and ASCII text on the CPU.
OpenGL uploads the canvas to a texture and presents it; this is accelerated
presentation, not GPU text/layout rasterization. GPU uses a disposable disabled
child window so its immutable pixel format never invalidates the GDI parent.
The low-level ui_gpu_* triangle API reports failures directly.

Call begin, draw and present. Begin resizes to the client area and clears.
Coordinates are clipped client pixels; colors are opaque 0x00RRGGBB. Zero-sized
clients are no-ops. Drawing errors propagate at presentation. Text is borrowed
for each call. Standalone framebuffers own VirtualAlloc memory; renderer DIB
pixels must never be passed to ui_framebuffer_destroy.

Destroy renderers/GPU contexts before their borrowed window. Destroy accepts
NULL; other calls require live handles and valid output pointers. Do not mix
direct GPU and common-canvas renderers on the same window.

## Validation

From the repository root with MinGW-w64 GCC and Make:

~~~sh
make -C compiler/libs/ui test
make -C examples/win/editor all test
make -C examples/win/cpui
make -C examples/win/gpui
~~~

Integration checks open brief test windows and verify styles/client sizes,
fullscreen bounds, maximized state, input/vetoable close, dynamic pacing,
resize, CPU/hardware pixels, deterministic GPU setup/presentation failures,
and atomic replacement including locked-destination failure. GNU linker wrappers
inject failures only in tests; production has no test-only switches.

Windows 10 1803+ is required for high-resolution waitable timers. OpenGL 1.1
entry points and BGRA texture uploads are attempted; unsupported uploads fall
back to CPU. Only Windows has a backend. Application FPS, refresh and vsync
remain separate concerns.

A middle-button press produces UI_MOUSE_MIDDLE in client coordinates. A right-button release produces UI_MOUSE_CONTEXT in client coordinates. ui_dialog.h provides an owned-window popup menu, directory picker, native file properties, Explorer folder opening and an informational message box. Dialog calls borrow their window and paths for the duration of the call. A dismissed popup returns zero.

Applications can query client coordinates with ui_window_pointer and choose a native arrow or resize cursor with ui_window_set_cursor. During mouse capture the position can extend outside the client area. The window borrows system-managed cursor handles.

Page Up and Page Down are delivered as UI_KEY_PAGE_UP and UI_KEY_PAGE_DOWN with the current modifiers.
