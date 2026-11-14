# Large native solution

`LargeDemo.sln` contains 24 projects: 18 static libraries, four applications and
two test executables. Libraries form an 18-level dependency chain with repeated
references to Library01. Executables share dependencies at different depths.
Solution records deliberately list consumers before dependencies.

Run from the repository root:

```powershell
.\greenc.exe examples/greencsharp/large_solution/LargeDemo.sln
pwsh -NoProfile -File compiler/test/greencsharp_solution.ps1
```

For target `x86_64-w64-mingw32`, Release executables are under each project's
`bin/x86_64-w64-mingw32/Release/`. Use your GCC `-dumpmachine` target if different.

| Project | Output |
| --- | --- |
| Server | 171 |
| Client | 78 |
| Worker | 192 |
| Tool | 21 |
| CoreTests | Core tests passed; exit 0 |
| IntegrationTests | Integration tests passed; exit 0 |

Each library produces its own `.a`. There are 42 generated C units/objects and
six executable entry points. Retained build outputs use only `.c`, `.h`, `.o`,
`.a` and `.exe` on Windows. Semantic, dependency and incremental records are
versioned comments in valid `.h` files. Native Unix executables omit `.exe`.

The regression builds/runs every executable, checks output types/counts and C
header syntax, recompiles all generated C independently, archives the library
objects and links/runs each entry point again. It also checks native no-op reuse.
The tests compile from C# through GreenC; no hand-maintained C copies are used.

IsTestProject is a literal marker. Tests are ordinary native assertion programs,
built as Exe projects and run explicitly; no test SDK/xUnit/NUnit adapter is
claimed. The example uses the supported static-method subset, not instance
objects, interface dispatch, BCL or NuGet. See the
[deferred BCL/package scope](../../../compiler/libs/greencsharp/BCL_NUGET_SCOPE.md).
