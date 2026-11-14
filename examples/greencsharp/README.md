# Native C# examples

Build the compiler/plugin using root `build.bat --no-pause` on Windows, or
`./build.sh` on Linux/macOS. No .NET SDK, Roslyn or Makefile is required.
See the [supported language and artifact contracts](../../compiler/libs/greencsharp/README.md).

From the repository root, for a Windows x64 MinGW toolchain:

```powershell
.\greenc.exe examples/greencsharp/hello/Hello.cs
.\examples\greencsharp\hello\bin\x86_64-w64-mingw32\Release\Hello.exe
```

Output: `Hello from native C# through GreenC!`, `42`, `True`, on separate lines.

The project example demonstrates namespaces, static methods, recursion and a
separately compiled library consumed through a generated declaration contract:

```powershell
.\greenc.exe build examples/greencsharp/projects/App/App.csproj
.\examples\greencsharp\projects\App\bin\x86_64-w64-mingw32\Release\App.exe
.\greenc.exe examples/greencsharp/projects/NativeDemo.sln
```

Both build the same App executable and Arithmetic archive. App prints:

```text
Project references resolved by greencsharp
21
120
```

Build/inspect the library independently:

```powershell
.\greenc.exe examples/greencsharp/projects/Arithmetic/Arithmetic.csproj
ar t examples/greencsharp/projects/Arithmetic/bin/x86_64-w64-mingw32/Release/libArithmetic.a
```

The archive is native and target-specific. App links it; dependency source
implementations are not merged into App's generated C. Public C headers and
versioned semantic contracts accompany it. These are not managed .NET assemblies.

```powershell
.\greenc.exe examples/greencsharp/projects/App/App.csproj --emit-c
.\greenc.exe examples/greencsharp/projects/App/App.csproj -c Debug
.\greenc.exe examples/greencsharp/projects/App/App.csproj --rebuild
.\greenc.exe examples/greencsharp/projects/App/App.csproj --clean
```

Inspect `.c` files under `App/obj/<target>/<configuration>/App/generated/`,
headers under `include/`, and versioned semantic/build metadata in valid `.h` comments.
There are no `.gc` intermediates. Build logs show
GCC compilation, `ar rcs` and dependency-ordered executable linking.

Use your compiler's `gcc -dumpmachine` target in paths when it differs from
`x86_64-w64-mingw32`. On Unix invoke `./greenc`, omit `.exe`, and retain `.a`
for project libraries. Native Unix execution is not verified here. Interfaces,
instance classes and dependency injection are not yet supported; these examples
exercise the working static-method subset.

The [large solution](large_solution/README.md) builds 24 projects with six executable outputs, including two native test programs.
