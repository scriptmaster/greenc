# BCL and package support: deferred implementation scope

Status: planned, not implemented. This document records the follow-up requested
on 14 November 2026. It is a development backlog, not an automatic scheduled job.

## Current delivery

GreenC's native C# frontend builds declarative .sln/.csproj dependency graphs into
C/header units, objects, static archives and native executable outputs. The
24-project scale example contains 18 libraries, four applications and two native
assertion executables. IsTestProject is accepted as a literal boolean marker;
it does not activate a test SDK, test discovery or a NuGet runner. Builds do not
execute project tests automatically.

Current built-ins are scalar arithmetic/boolean helpers, borrowed string literal
and equality operations, Console.Write/WriteLine and command-line argument
access. This is not a BCL implementation. PackageReference, FrameworkReference
and managed binary assembly references remain rejected with diagnostics.
Instance classes/interfaces, constructors, inheritance, generics, exceptions,
async and reflection also remain unsupported. A large graph passing the static
subset tests does not establish arbitrary C# or arbitrary .sln compatibility.

## Follow-up milestones, in dependency order

1. Define the native managed-value ABI and ownership/runtime contract. Implement
   instance layout/opaque references, constructors, interface dispatch, strings,
   arrays, lifetime handling, checked bounds and error propagation. Specify
   accessibility, initialization and exception behavior before exposing BCL APIs.
   Acceptance: constructor-injected Orders interface example, alternate injected
   implementation, cross-project access failures and failure-cleanup tests.
2. Add a versioned first BCL surface under a dedicated library boundary, with
   declarations and native implementations separate from application policy.
   Initial candidates: String/Char and encoding, numeric conversion/Math,
   Array, collections after generics, Path/File/Directory/Stream, time and text
   building. Publish an explicit supported API matrix and behavior/encoding/error
   limitations; link required runtime definitions once. Test Unicode, bounds,
   overflows, allocation failure and resource cleanup on supported platforms.
3. Implement deterministic package graph resolution: literal PackageReference,
   package identity and versions/ranges, transitive dependencies, target/framework
   and runtime asset selection, local cache and reproducible lock data. Parse
   .nuspec and package archives natively; validate hashes, archive paths, size
   limits and offline behavior. Do not execute package scripts, MSBuild targets,
   analyzers or source generators implicitly. Inputs may contain package metadata;
   generated compiler/build outputs remain .c/.h/.o/.a/native executables, with
   any compiler-owned lock/cache records stored as documented .h comments.
4. Support a clearly identified native/source package class first. Translate
   compatible C# source through GreenC; consume target-matching native archives
   through versioned headers/semantic contracts. Existing NuGet managed DLLs
   need an additional ECMA-335 metadata/CIL importer or an explicit reimplementation;
   downloading a package alone does not make it compilable without .NET/Roslyn.
   Unsupported assets must fail with package, version, target and API diagnostics.
5. Select and implement common package capabilities against the published ABI:
   JSON serialization, logging abstractions, configuration/options and dependency
   injection first; then HTTP, concurrent collections and async only after their
   runtime prerequisites. Review licenses and compatibility for each selected
   package/API; do not claim blanket compatibility with Newtonsoft.Json,
   System.Text.Json, Microsoft.Extensions.*, xUnit or NUnit.
6. Add test discovery/runner integration after the relevant reflection/attributes
   or explicit registration model is defined. Validate a 20+ project solution
   with applications, native unit/integration tests, BCL code and versioned package
   dependencies; test offline rebuilds, conflicting dependencies, contract changes,
   target mismatch, source removal, incremental reuse and supported OS execution.

## Completion criteria

No dependency on dotnet or Roslyn. No silent skipping of unavailable APIs or
package assets. All supported source is translated into inspectable C/header
output and uses the normal native compile/archive/link pipeline. A published
compatibility matrix, ownership/error documentation, representative package
examples and actual platform validation accompany each milestone. Native .a/.o
files remain target/ABI specific; these outputs do not become CLR assemblies.

The initial package/API list above is a proposed starting scope. Full BCL,
ASP.NET, WPF/WinForms, reflection emit, dynamic assembly loading and arbitrary
managed NuGet compatibility are outside that initial milestone set.

## Primary references

- [.NET runtime libraries overview](https://learn.microsoft.com/dotnet/standard/runtime-libraries-overview)
  describes the breadth of BCL/runtime-library APIs, beyond Console/scalars.
- [NuGet .nuspec reference](https://learn.microsoft.com/nuget/reference/nuspec)
  defines dependency groups and package metadata that the resolver must handle.
- [NuGet target frameworks](https://learn.microsoft.com/nuget/reference/target-frameworks)
  documents framework-specific dependency and asset selection.
