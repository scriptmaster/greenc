# GreenC Editor for Windows

From this directory, run `make` with the MinGW GCC toolchain and launch `editor.exe [file]`. Run `make test` for the editor checks. The entry point is in `src/main.c`. Application contracts are in `src/app/*.h` and the implementation directories group startup, entities, views, input, workflows and utilities.

The build bootstraps GreenC and emits each application source under `build/`. Hosted modules parse statement and scope structure and lower typed local declarations such as `let u32 next`. MinGW checks C types and expressions and links the generated application with the Windows backends. The editor tests and scroll benchmark use the same generated application sources.

The default window is maximized with antialiased Consolas text. Use `--windowed`, `--fullscreen` or `--borderless` to choose another mode, and `--fps=N` to set startup pacing. The editor starts with CPU rendering at 30 FPS. One click on the FPS control selects 60 FPS. Use `--gpu` to request accelerated presentation with CPU fallback.

## Documents, navigation and tabs

The File menu provides New File, Open File, Save, Save As and Quit. Use Ctrl+O to open, Ctrl+S to save, Ctrl+Shift+S for Save As and Ctrl+Q to quit. Drag or Shift-click to select text. Shift+arrow extends the selection, Ctrl+A selects all, and Ctrl+X/C/V cuts, copies or pastes. Ctrl+Home/End jumps to the start/end of the document. Clicking blank space below the last line puts the caret at the end; Shift-click extends the selection there. Key input resets the thin blinking caret to visible.

Document, Files and Terminal panels share a scrollable tab strip with arrow controls, a plus button and close buttons. The Files panel and terminal have draggable dividers with resize cursors. Line numbers are muted except for the current line and byte warnings.

Ctrl+T and Ctrl+W add and close tabs in the focused panel. Files opens a folder picker and Terminal opens the configured shell. Ctrl+N creates an untitled document. Middle-click the document to start vertical autoscroll, move above or below the starting point to control speed, and middle-click again to stop.

Click the disclosure beside a line number to fold a block. Click the hidden-line count to expand it. The far-left gutter toggles a breakpoint. The first breakpoint opens Debugger settings if no executable has been chosen. Settings opens as an editor tab and can be closed with its close icon or Ctrl+W. Theme edits apply immediately and persist in local application data. Scroll the theme field list with the wheel. Breakpoint and stopped-line backgrounds are independently themeable.

Choose a GDB executable in Editor / Debugger. Build the target with debug symbols (for hosted GreenC use the emitted C with GCC `-g -O0`). F5 asks for the target executable, installs the marked lines from open saved source files, and runs it. At a stop, F5 continues, F10 steps over, F11 steps into and Shift+F5 stops the session. Rebuild after editing source and restart to load changed breakpoints. Run `make debug-test` for a live GDB regression using `test/debug_target.gc`.

Right-click a tab for New, Close, Save or Save All. Document tabs save their buffers. Terminal tabs export their visible screens to text. Files tabs export their root paths. A leading dot indicates unsaved document edits or activity in an unfocused shell or folder. Folder notifications and console screen changes are checked every three seconds.

The footer shows the document path on the left. Long paths scroll back and forth within the available space. Click the FPS value at the right edge or press F6 to cycle the configured frame rates.

## Terminal

Press Ctrl+` to toggle the embedded Windows console panel. Type commands at the shell prompt. The plus opens another terminal and the adjacent selector offers Command Prompt, PowerShell and Bash. The default is cmd.exe. Use `--shell=PATH` to configure a different shell. If bash.exe is unavailable, the editor tries cmd.exe.

## Files panel

Ctrl+B toggles the panel. The first named document establishes a project root. Opening a descendant keeps that root.
Opening an external file activates its existing project or adds its containing folder as another Files tab.
Use the strip arrows to scroll folders, **+** to browse for a folder, and **X** to close its Files tab. Untitled documents keep the current project.

Expand folders with the disclosure arrows. Empty folders lose their arrow after expansion. Hidden entries and dot-prefixed names use muted text. Scroll the tree independently of the document. Right-click a folder to create a file or directory inside it. On a file, **New File Here** uses its containing folder. **New File in Panel Root** is also available from nested folders. Type a single name in the panel header. Enter applies it and Escape cancels. New files open in a tab immediately.

Right-click a file to duplicate, duplicate under a chosen name, rename, inspect its size, open Windows Properties or launch its containing folder in Explorer. F2 renames the selected file while the panel has focus. Duplicates copy the saved bytes on disk. Rename preserves edits in open tabs. Delete asks for confirmation and permanently removes the selected file. Save or close a modified tab before deleting its file. File creation, duplication and rename preserve existing destination files.

The editor currently accepts ASCII text documents that fit in available process memory, including normalized text and the row index, and the Windows file dialogs and paths use the native ANSI path limit.

## Large-file performance

The editor reads 64 KiB blocks into a growing flat text buffer. A flat index records visible line boundaries. Scrolling paints the viewport, and caret jumps use binary search on that index. The working set includes text, index entries and allocation headroom. Fold flags are allocated on first use. Opening, saving, editing a flat buffer and rebuilding its index can still take time proportional to file size.

To measure your machine, build `make bin/scroll_benchmark.exe` and run `bin/scroll_benchmark.exe path/to/file.txt` with a supported document containing more than 1,000 lines. The CPU benchmark reports file loading, first index and frame, 200 wheel-and-paint frames, and 10,000 cursor lookups without frame-rate pacing.

## File details and hex editing

The footer uses bytes below 1,000 and decimal KB, MB and GB with one decimal place above that. Symbolic links show their target's size. The footer gives the active file's saved file size and
relative modification time.

Hover over Modified to see the exact local date and 12-hour time. Unsaved new documents show their buffer size. Memory is the process private working set in 1,048,576-byte units, sampled alongside CPU usage once per second. The FPS control identifies the renderer; CPU mode also shows process CPU as a percentage of total logical processor capacity.

Files containing non-ASCII or binary bytes open with hexadecimal and printable-character columns. Non-printable bytes appear as middle dots. Up to 256 bytes open directly for editing. Larger files preview the first 256 bytes; choose **Open Hex Editor** for the full file. Scroll with the wheel or drag the scrollbar. Click either byte column to highlight the same byte on both sides, or use arrows and Ctrl+Home/End. Drag in either column, Shift-click, or hold Shift while navigating to select bytes. Ctrl+A selects the displayed range (up to 256 bytes in preview), and Ctrl+C copies uppercase hexadecimal pairs separated by spaces. Ctrl+Z undoes the last text edit or hex digit overwrite in the active tab. Ctrl+Y or Ctrl+Shift+Z redoes it. Each tab retains its own history until closed. A new edit clears redo in that tab. Type two hex digits to overwrite a byte and use Ctrl+S to save.

Page Up and Page Down move the caret and scroll by three quarters of the visible editor height (at least one row). Hold Shift to extend a text or byte selection.

File detection checks the first 256 bytes before continuing the ASCII load. Tabs, carriage returns and line feeds are accepted. A later unsupported byte opens the original bytes in the hex view, including in Markdown files.

Right-click the hex preview or full hex document and choose **Open in text editor**. Each unsupported byte occupies one editable cell marked with a middle dot. Hover for its current zero-based byte offset, hex value and decimal value. Saving preserves untouched bytes. A red line number identifies a line containing unsupported bytes. Delete or replace all marked bytes to reopen the file as ASCII. Undo restores the original byte and its warning.

## Settings and syntax highlighting

Settings > Editor > Keybindings lists active shortcuts and suggested additions. Under Themes, click the group selector to cycle through editor colors, buttons, links, debugger colors, general syntax and language-specific syntax. General rules color quoted strings green, numbers amber and function names teal. In C, headers and GreenC, directives such as #include and keywords are violet; line and block comments are dark grey, including across lines. Several other C-style extensions share comment and call highlighting. Python, shell and PowerShell have hash comments, and Markdown has headings. These are lexical rules rather than full language parsers. The theme controls include button radius, hover and press colors and opacity, plus link styles. Browse loads a theme file and Save As exports a copy.

Right-click the Settings tab and choose Open as JSON file to edit theme and debugger preferences. Ctrl+S validates and applies edits. Ctrl+Shift+S exports the JSON. Open as settings UI validates pending edits and returns to the controls in the same tab.

## Sessions and layout

On normal exit the editor saves open tab identities and active tabs in local application data. Named documents reload from disk; untitled tabs reopen empty after confirmed discard. Terminal tabs start fresh shells. Starting in a different working directory selects a restored document beneath it when possible, otherwise adds the folder and its default document. An explicit filename takes priority.

Wide text documents have a draggable horizontal scrollbar above the footer or terminal. Settings > Editor > Themes > Editor colors includes `column_guide`. The default is 100; choose zero to hide the guide. The same integer can be edited directly in Settings JSON.

## Windows Explorer

Open **Settings > Editor > General**. The **Classic menu for this user** checkbox controls the folder and folder-background actions under **Show more options**. Changing that checkbox does not request elevation.

On x64 Windows 11, click **Install Windows 11 menu (administrator)** and approve the Windows consent prompt when ready. The helper installs the bundled unsigned GreenC Explorer identity package for the account running the installer and registers machine-wide classic commands. Right-click a directory or the blank background inside a directory and choose **Open in GreenC Editor**. The matching Files tab is selected, or a new folder tab is added. A request sent to an existing editor leaves its window in the background. Use **Remove Windows 11 menu (administrator)** to remove the package registration and machine-wide classic commands. The per-user classic checkbox is independent.

The editor's regular startup, opening General, building the package, and receiving a folder never request elevation. The modern extension uses the DLL and editor in the external bin directory; keep them together. If Windows declines deployment, check the Settings status and the installation account's %LOCALAPPDATA%/GreenC-editor-explorer-install.log. The editor does not restart Explorer. If Explorer caches an old menu, sign out and back in when convenient.

## Build and architecture

The Makefile bootstraps GreenC, checks the hosted ownership contracts and emits intermediate C in build/. The native compiler links the editor and Windows backends into bin/editor.exe. The launcher in this directory points to that executable. The Windows build also creates the shell-extension DLL, embedded icon and sparse MSIX identity package. Building does not install them. Run make test for editor and compiler regressions and make debug-test for the GDB integration test.

The compiler enforces declared move, read and stack ownership contracts, including supported struct fields, function arguments and returns. The runtime memory counter describes this process on this machine; switching compilers alone does not guarantee a particular working set. Shared UI and filesystem libraries own platform mechanisms. The editor owns window preferences, shortcuts, document behavior, theme and layout. See the repository's PRINCIPLES.md and AGENTS.md for the boundary and source conventions.
