# Bootstrapping the compiler

Double-click root `build.bat` on Windows. It finds GCC on PATH or in the default
MSYS2 UCRT64 installation and builds `greenc.exe` beside the script. For command
line use without the final pause, run `build.bat --no-pause`. `CC` may name a
different GCC executable. The compiler is ignored; the bootstrap script is tracked.

The bootstrap also builds and checks the optional native `greencsharp` language
plugin under `compiler/libs/greencsharp/bin/`. It uses C17 and the native toolchain,
without .NET or Roslyn. C# input is dynamically delegated to this plugin; ordinary
C/GreenC input does not load it. See the [plugin contract and supported language](libs/greencsharp/README.md)
and [runnable examples](../examples/greencsharp/README.md).

On Linux or macOS run root `./build.sh`. It selects the current OS, uses
`${CC:-cc}`, and produces root `greenc`. Unix file-manager double-click behavior
depends on its executable-script settings; the script has its executable bit
set in Git. On macOS a native C toolchain such as Command Line Tools is required.
Both scripts resolve paths relative to themselves, stage outputs under
`bin/bootstrap/`, run smoke checks and replace the root compiler only on success.
The Unix script additionally tests hosted file I/O, directory enumeration and
child-process results. Existing compilers survive compilation/test failures.

Unix bootstrap uses `GREENC_HOSTED`, a libc/POSIX runtime mode with normal
process startup, allocation, files, directory enumeration and `posix_spawnp`.
This supports building the compiler without Linux-specific raw syscalls on
Linux/macOS; it does not add Unix UI backends or port the Windows project
builder below. Native Linux/macOS execution has not been validated on this
Windows development machine.

# Building applications on Windows

Application builds require GCC
on PATH; Windows resources require windres and C++ companions require G++.
Keep the compiler in this repository tree so it can locate `compiler/libs`.

```powershell
.\greenc.exe build examples\win/editor
.\greenc.exe examples/win/editor
.\greenc.exe examples/win/editor/src/main.c
```

These commands use GreenC's project builder, without reading a Makefile.
`build` accepts exactly one directory or entry-point file. Without `build`,
a lone directory or file named `main.c` selects the same project builder.
Other file commands and explicit hosted-module/emit-C commands retain the
existing file compiler behavior; they do not discover application sources.

A directory must contain `main.c` or `src/main.c` (the former takes precedence).
The source root is the entry point's directory. All `.c` and `.gc` files beneath
it are checked as hosted modules and emitted in sorted path order. Every
source directory becomes an include search directory. Each lowered source is
compiled separately with its original directory searched first for quoted
includes, preserving local header resolution. There must be only one
application entry point. Hidden entries, reparse points and directories named
`bin`, `build`, `test` and `tests` are skipped. Application discovery collects
the source tree, rather than following function calls. Keep unrelated
programs and tests outside that source tree. At most 1,024 source files,
64 directory levels and one `.ownership` contract file are supported.

The project root is the source root's parent when the source root is named
`src`, otherwise the source root itself. Output goes to
`<project>/bin/<project-directory-name>.exe`. Generated hosted C, object files
and GCC compile/link response files go under `<project>/build/greenc/`. Native compilation uses
`-Os -std=c17 -Wall -Wextra -Werror`. Tool failures return a nonzero exit code.
Project-build flags and Makefile targets are not accepted by this CLI yet.

# Discovering bundled library implementations

The native preprocessor's dependency files identify active includes, including
macro includes and conditional branches. GreenC follows user headers beneath
`compiler/libs`, normalizes paths and processes each dependency once. This
handles transitive includes and cycles without compiling inactive includes.
System headers supplied by the toolchain do not select bundled sources.

For `module.h`, GreenC first checks `module.sources`. Without a manifest it
selects sibling `module_win32.c` when present, otherwise `module.c`. Headers
with no companion implementation are treated as header-only. A library can
describe a multi-file implementation and native link requirements:

```text
# module.sources, beside module.h
source module_win32.c
source module_support.c
link -luser32
```

Source paths are relative to the header's directory. Only `source`, `link`,
empty lines and `#` comments are accepted. Manifests are library-owned source
metadata, not executable packaging recipes. Native implementations are probed
for further includes; their sources are deduplicated and their link options
follow source/object inputs. The graph is limited to 2,048 dependencies.
Unconventional implementations need a manifest; prebuilt or external libraries
still require explicit link options. GreenC does not infer a linker library
from an arbitrary system header name.

The editor's UI, filesystem, console, shell and process-statistics backends are
discovered this way. Its resource, application subsystem and Explorer packaging
remain in its own entry-point annotations.

# Application build annotations

Put optional build annotations on separate lines in the entry point:

```c
// greenc: native ui/ui_window_win32.c
// greenc: link -mwindows -luser32 -lgdi32
// greenc: resource assets/application.rc
```

`native` optionally adds a backend source beneath `compiler/libs` that cannot
be reached through includes. Normally source discovery makes this unnecessary.
`link` appends native linker arguments to the generated response file.
`resource` compiles one resource script to `build/greenc/resource.o` using
windres. Use at most one resource annotation per application.

For companion artifacts, `// greenc: prepare <command>` runs a native tool
directly, without a shell, in the project root. Commands run in annotation
order after checking application sources and before linking. Quote arguments
containing spaces. Shell operators and environment expansion are not supported.
`// greenc: publish temporary-path|destination-path` replaces a prepared
artifact using Windows file replacement. These annotations express application
packaging policy, while GreenC generates compilation commands for the discovered
application sources. As with any executable build instructions, build trusted
projects only. A failed build can leave intermediates and previous binaries.

This project builder currently supports Windows hosted applications with ANSI
paths within Windows' native limits and shorter than 1,024 bytes. It does not add Linux project-build support or
infer external platform libraries or application packaging from arbitrary C headers.

Run the focused project-build regression checks from the repository root:

```powershell
pwsh -NoProfile -File compiler/test/project_build.ps1
```
