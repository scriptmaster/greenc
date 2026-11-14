# Windows UI examples

Build with MinGW-w64 GCC and Make from the repository root:

~~~sh
make -C examples/win/cpui
make -C examples/win/gpui
make -C examples/win/editor
~~~

Run one example at a time:

~~~sh
./examples/win/cpui/cpui.exe
./examples/win/gpui/gpui.exe
./examples/win/editor/editor.exe
./examples/win/editor/editor.exe --cpu --fps=144 --windowed draft.txt
~~~

CPUI generates an animated 640x360 framebuffer on the CPU and presents with GDI.
GPUI draws a rotating equilateral triangle directly through hardware OpenGL.
This low-level GPU demonstration reports unsupported hardware rather than
silently changing its algorithm. Both are borderless fullscreen, have
program-owned 240 FPS settings, and close on Escape or native close. Neither
changes display modes. Animation uses elapsed time.

The editor starts maximized with title bar and minimize/maximize/close buttons.
Its common renderer uses GPU presentation when available and automatically
falls back to CPU on setup/presentation failure. Both paths rasterize ASCII
text/rectangles on the CPU. The active backend appears in the status bar.

Use --cpu for explicit CPU, --windowed for a restored decorated window,
--borderless for a window without a title bar, and --fullscreen to demonstrate
fullscreen. --fps=N accepts 0..1000, where zero is unpaced. An optional file path
defaults to notes.txt in the working directory. Missing files start empty and
are created only on Save.

## Editor controls and scope

Ctrl+End and Ctrl+Home jump to document boundaries. Shift extends selection to those boundaries. The gutter highlights the caret line.

Ctrl+B toggles the right file tree, initially rooted at the opened file’s directory. Scroll with the mouse wheel and click folders to expand or collapse them. Hidden entries are muted.

Ctrl+grave toggles the bottom command panel. Drag its upper edge to resize, type a command and press Enter. Click the document to resume editing. Set the shell executable in editor_settings.c or pass --shell=PATH. The panel preserves the shell between toggles and supports line-oriented commands with redirected input and output.

- Type ASCII text; Enter inserts a newline and Tab inserts four spaces.
- Arrows and Home/End move the caret; Backspace/Delete remove characters.
- Hold Shift with arrows or Home/End to select. Click and drag to select with the mouse.
- Ctrl+A selects all. Ctrl+X, Ctrl+C and Ctrl+V cut, copy and paste plain text.
- Ctrl+S saves through fully written/flushed sibling-temp replacement.
- Ctrl+Q or native close quits, confirming before discarding unsaved changes.
- F6 cycles application settings through 60, 144 and 240 FPS at runtime.
- Resize, minimize/maximize and restore behave as desktop windows. The view
  scrolls to keep the keyboard caret visible.

This is a small text-editor example: ASCII documents limited by available process memory, with a native file chooser and no undo. CRLF/CR/LF normalize to LF;
tabs expand to spaces. Saving uses ASCII with LF. Paths use Windows ANSI APIs
and MAX_PATH limits. This is not a full IDE.

## Where to customize

editor_settings.c owns window options, initial FPS, F6 rates, font, theme,
layout and caret blink. Library/header edits are unnecessary.
editor_document.c owns text/caret operations and document I/O policy;
editor_input.c maps keys to model operations; editor_view.c owns layout,
scrolling and drawing; main.c orchestrates creation, input, rendering and cleanup.

Architecture moved out of examples into compiler/libs/ui/. Its
[README](../../compiler/libs/ui/README.md) documents contracts, ownership,
failure behavior and tests. Atomic file replacement lives in compiler/libs/sys/fs/.
Root/library AGENTS.md freeze this boundary for ordinary program changes.

These examples are native C, built directly with GCC rather than the partial
GreenC parser/borrow checker. Only Windows has a UI backend. Pacing requires
Windows 10 1803+ and does not guarantee monitor refresh/vsync.

The editor uses antialiased text by default. Open a file from the tree to add or activate its tab. Click a tab or use the left/right arrows to switch documents. Up to 24 documents can stay open, including unsaved buffers. The native File, Edit and View menus share the editor keyboard commands.
