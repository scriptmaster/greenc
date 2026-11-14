# greencsharp native compiler plugin

GreenC dynamically loads this C17 plugin for `.cs`, `.csproj` and `.sln` inputs.
It parses and checks the supported C# subset, emits native `.c` and `.h` files,
and invokes the configured GCC toolchain. It does not use .NET, Roslyn, MSBuild,
or `.gc` intermediates.

## Build and use

Run root `build.bat --no-pause` on Windows or `./build.sh` on Linux/macOS.
The bootstrap builds and checks the compiler and plugin before publishing them.
The plugin is `compiler/libs/greencsharp/bin/greencsharp.dll` on Windows,
`greencsharp.so` on Linux, or `greencsharp.dylib` on macOS. Those platform-specific
plugins are distinct from the static archives produced for C# library projects.
Windows compilation/execution is tested; native Unix execution remains unverified.

```powershell
.\greenc.exe examples/greencsharp/hello/Hello.cs
.\greenc.exe build examples/greencsharp/projects/App/App.csproj
.\greenc.exe examples/greencsharp/projects/NativeDemo.sln
.\greenc.exe examples/greencsharp/projects/App/App.csproj -c Debug
.\greenc.exe examples/greencsharp/projects/App/App.csproj --emit-c
.\greenc.exe examples/greencsharp/projects/App/App.csproj --rebuild
.\greenc.exe examples/greencsharp/projects/App/App.csproj --clean
```

`build` is optional. Options can precede or follow the input. `-c` or
`--configuration` selects `Release` (default, `-Os`) or `Debug` (`-O0 -g`).
`--emit-c` generates the root project's units without compiling/linking them;
its dependencies are built so their contracts are available. `-o PATH` copies
the selected project's output to an additional path. With `--emit-c`, `-o` requires
one source unit; emitted C still needs the generated include directory and
referenced public headers. Solution builds and clean do not accept `-o`.

`GREENC_CSHARP_CC`, then `CC`, then `gcc` selects the compiler executable.
Use executable paths, not shell command strings. `AR` overrides the archiver;
otherwise GreenC asks the compiler using `-print-prog-name=ar`. The compiler's
`-dumpmachine` defines the target. Cross compilation requires a complete matching
GCC/ar toolchain. LTO is not enabled. Logged commands make the exact native compile/archive/link inputs inspectable. No Makefile is read.

## Project artifacts and contracts

Each project builds separately in dependency order. Library projects produce
lib<AssemblyName>.a; executable projects produce <AssemblyName>.exe on Windows,
or <AssemblyName> on Unix. Solutions retain every project's own output, including
multiple executable/test projects. AssemblyName defaults to the input stem.

Successful builds retain only .c, .h, .o, .a and native executable files. Paths
are relative to each project's directory:

```text
obj/<target>/<configuration>/<AssemblyName>/
    generated/<source-folders>/gcs_unit_<identity>_<source>.cs.c
    objects/<source-folders>/gcs_unit_<identity>_<source>.cs.o
    objects/... .o.dependencies.h, .o.cache.h
    include/<AssemblyName>.h
    include/<AssemblyName>.internal.h
    include/greenc_runtime.h
    artifacts.h
bin/<target>/<configuration>/
    lib<AssemblyName>.a or <AssemblyName>[.exe]
    <artifact>.cache.h
    <AssemblyName>.h
    <AssemblyName>.semantic.h
    greenc_runtime.h
```

One C translation unit/object is retained per source. Source folders are preserved;
hashed basenames prevent archive member collisions. External source items use flat
hashed names. Method symbols derive from assembly/type/member identities. Public
headers expose public methods of public classes; private headers contain local
declarations. Versioned semantic, dependency, cache and artifact records are
hex-encoded inside valid C header comments (greenc-data-v1). Encoding prevents
source/path text from terminating a C comment. These headers carry build data,
not runtime functions. They replace .contract/.json/.d/.stamp/.list sidecars.
Typed AST/dependency JSON remains available through in-memory API analyze mode,
not written as build outputs.

Consumers parse semantic headers and include public headers without merging
referenced implementation bodies. Target, configuration and toolchain identity
are checked. Public/internal/private access is checked semantically; native symbol
visibility is not a security boundary. This is GreenC's native contract format,
not CLR assembly metadata or reflection.

Compiler, ar and linker commands receive argv directly, without retained response
files. GCC's temporary dependency listing is converted into a header comment and
removed. Libraries use ar rcs with a fresh temporary archive and exactly the
current objects, then atomically replace the published archive. Removed/renamed
sources cannot leave stale members. Executables link own objects followed by
transitive archives in consumer-before-provider order, deduplicating diamonds.
Inline scalar runtime helpers carry no duplicated global state. Programs do not
load the compiler plugin at runtime.

Content fingerprints cover generated code, GCC header dependencies, project and
semantic data, toolchain and flags. No-op builds skip native tools. Private body
edits rebuild the owning units/archive and relink affected executables while
preserving consumers with unchanged contracts. Public contract changes invalidate
consumer objects. Parsing/checking still run each time. Rebuild bypasses caches;
clean removes recorded project/dependency artifacts, including obsolete source
objects. Directory scaffolding and published shared runtime headers remain.
Exported -o copies are caller-owned. Serialize builds in a project tree. Failures
preserve the last published native executable/archive; check the exit status.

The [24-project example](../../../examples/greencsharp/large_solution/README.md)
contains 18 libraries, four applications and two native assertion test programs.
Its regression independently recompiles all generated C and relinks/runs every
entry point. IsTestProject accepts a literal boolean marker; tests still require
OutputType=Exe and are run explicitly. It does not activate a .NET test SDK.

GNU documents r/c/s in its [ar specification](https://sourceware.org/binutils/docs/binutils/ar-cmdline.html).
Static archives remain target/ABI specific. The
[.NET manifest](https://learn.microsoft.com/en-us/dotnet/standard/assembly/manifest)
model is distinct from this native project-contract mechanism.

## Implemented C# subset

- ASCII identifiers, UTF-8 input with optional BOM, line/block comments.
- File-level `using` directives before namespaces, block/file-scoped namespaces,
  classes containing static methods, public/internal/private method access.
- `int`, `bool`, borrowed immutable `string`, initialized locals and `var`;
  forward/static calls, recursion, block/expression method bodies.
- Arithmetic, comparisons, string-content equality, boolean negation,
  short-circuit `&&`/`||`, assignment, int compound assignment and `++`/`--`.
  Runtime addition/subtraction/multiplication wrap to 32 bits; constant overflow
  and division by zero are diagnosed. Runtime division errors terminate nonzero.
- Blocks, if/else, while, for, break, continue, typed returns. Non-void methods
  require a return on every supported path. Ordered temporaries preserve C#
  argument and operand evaluation order.
- One static void/int Main, optionally `string[] args`, or one top-level file.
  Argument arrays support Length, checked indexing and passing to methods.
- Console.Write(value), WriteLine([value]) for int/bool/string. Literal escapes
  include newline, carriage return, tab, backslash and quote. Native argument
  strings use the host narrow-character encoding, not CLR UTF-16.

Instance objects, fields, constructors, interfaces/dispatch, inheritance,
properties, overloads, partial classes, generics, null, delegates/lambdas,
exceptions, async/await, LINQ, attributes, preprocessor directives, string
allocation/concatenation and the wider .NET class library are unsupported and
rejected. In particular, the email's Orders/interface-injection acceptance
example is **not implemented**. Static archives and contracts do not establish
that object semantics work. There is no CLR garbage collector or exception model.

## Project and solution input

A bounded native XML reader accepts `Project Sdk="Microsoft.NET.Sdk"`, or literal
compile-item projects without an SDK. Properties: OutputType, AssemblyName,
TargetFramework, EnableDefaultCompileItems, ImplicitUsings, Nullable and IsTestProject.
TargetFramework/Nullable are metadata only; no framework or SDK is loaded.
SDK projects discover `.cs` recursively, excluding hidden/linked directories,
bin, obj and build. Alternatively disable default items and list literal
`Compile Include` paths. Literal `ProjectReference Include` paths are normalized,
deduplicated and cycle checked. References must target Library projects.

Conditions, expansion, explicit-item globs/removal/metadata, imports, targets,
NuGet packages, binary assembly references, custom SDKs, multi-targeting and
unknown properties are rejected. No project commands execute; DTDs/external
entities are rejected. This is not full MSBuild evaluation. Windows filesystem
paths currently use ANSI APIs. The format-12 `.sln` reader accepts project and
solution-folder records with matched project/global sections. Configuration
sections do not override explicit CLI configuration. Build-order metadata does
not create assembly references; ProjectReference supplies those edges.

Limits: 128 projects, 1,024 sources per project compilation, 100,000 AST nodes,
128 expression/statement levels, 64 directory levels, 32 XML levels,
10,000 XML elements/project and 16 MiB per input/generated artifact.

## Plugin ownership and validation

`include/greencsharp.h` exports one size/version-checked API via
`greencsharp_get_api`. `create` owns the compilation, `result` borrows strings,
and `destroy` frees all allocations before the caller unloads the library.
Analyze mode returns C/AST/graph in memory without writing files; native modes
return the artifact path. Parsing, checking and build state remain in the plugin.
The core contains only argument handling, dynamic loading and the ABI adapter.

The loader checks beside the compiler, then its relative
`compiler/libs/greencsharp/bin/` directory; it does not search PATH/current
working directory for plugins. `GREENC_CSHARP_LIBRARY` explicitly overrides the
path. Only load trusted native code. Missing libraries, absent symbols or
incompatible ABI produce the requested missing-dependency error and nonzero
status. Ordinary C/GreenC commands do not load this plugin.

On Windows run `compiler/test/greencsharp.ps1`,
`compiler/test/greencsharp_archives.ps1` and
`compiler/test/greencsharp_solution.ps1` with PowerShell. The bootstrap also
runs `compiler/test/greencsharp_api.c` for repeated success/failure ownership
lifetimes and ABI checks. The source groups discovery/XML in graph/project/
solution/files/paths/scan modules, syntax in lexer/parse modules, semantics in
symbols/check modules, C lowering in emit modules, and artifact/tool/cache/link
mechanisms in build modules. Application policy stays outside these mechanisms.

BCL and NuGet work is deferred in [BCL_NUGET_SCOPE.md](BCL_NUGET_SCOPE.md).
