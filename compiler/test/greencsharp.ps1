# Native C# compiler integration checks. Requires the root bootstrap build and GCC.
$ErrorActionPreference = 'Stop'
$repository = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$compiler = Join-Path $repository 'greenc.exe'
$target = (& gcc -dumpmachine).Trim()
$profile = "$target/Release"
$fixture = Join-Path $repository ('bin/greencsharp checks/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $fixture | Out-Null

function Build-Source([string]$name, [string]$source, [string[]]$expected, [string[]]$programArguments = @()) {
    $directory = Join-Path $fixture $name
    New-Item -ItemType Directory -Force $directory | Out-Null
    $inputFile = Join-Path $directory 'Program.cs'
    $outputFile = Join-Path $directory 'bin/program.exe'
    $log = Join-Path $directory 'compiler.log'
    Set-Content $inputFile $source
    & $compiler build $inputFile -o $outputFile *> $log
    if ($LASTEXITCODE -ne 0) { throw "$name failed to build: $(Get-Content -Raw $log)" }

    $actual = @(& $outputFile @programArguments)
    if ($LASTEXITCODE -ne 0 -or ($actual -join "`n") -cne ($expected -join "`n")) {
        throw "$name produced [$($actual -join '|')] instead of [$($expected -join '|')]"
    }

    $generated = Get-ChildItem (Join-Path $directory "obj/$profile/Program/generated") -Recurse -Filter '*.c'
    if (!$generated -or !(Test-Path (Join-Path $directory "obj/$profile/Program/include/Program.h"))) { throw "$name did not produce C/header artifacts" }
}

function Reject-Source([string]$name, [string]$source) {
    $directory = Join-Path $fixture $name
    New-Item -ItemType Directory -Force $directory | Out-Null
    $inputFile = Join-Path $directory 'Program.cs'
    $log = Join-Path $directory 'compiler.log'
    Set-Content $inputFile $source
    & $compiler $inputFile *> $log
    if ($LASTEXITCODE -eq 0 -or (Get-Content -Raw $log) -notmatch 'greencsharp:') {
        throw "$name was not rejected with a compiler diagnostic"
    }
}

Build-Source 'arguments' @'
using System;
class Program {
    static void Main(string[] args) {
        Console.WriteLine(args.Length);
        Console.WriteLine(args[0]);
        return;
    }
}
'@ @('1', 'one value') @('one value')

Build-Source 'semantics' @'
using System;
static class Program {
    static int Mark(int value) { Console.Write(value); return value; }
    static int Pair(int left, int right) => left * 10 + right;
    static int Main() {
        Console.WriteLine(Pair(Mark(1), Mark(2)));
        bool skipped = false && Mark(9) == 9;
        bool selected = true || Mark(8) == 8;
        Console.WriteLine(skipped);
        Console.WriteLine(selected);
        int value = 1;
        Console.WriteLine(value + (value = 2));
        value += (value = 3);
        Console.WriteLine(value);
        int sequence = 0;
        Console.WriteLine(sequence++ + ++sequence);
        Console.WriteLine(sequence);
        int sum = 0;
        for (int index = 0; index < 5; index++) {
            if (index == 2) { continue; }
            sum += index;
        }
        while (sum < 10) { sum++; }
        Console.WriteLine(sum);
        int maximum = 2147483647;
        Console.WriteLine(maximum + 1);
        Console.WriteLine(0123);
        Console.WriteLine("same" == "same");
        return 0;
    }
}
'@ @('1212', 'False', 'True', '3', '5', '2', '2', '10', '-2147483648', '123', 'True')

Build-Source 'top level return' 'System.Console.WriteLine("before"); return;' @('before')
Build-Source 'minimum and remainder' 'int value = -2147483648; System.Console.WriteLine(value % -1);' @('0')
Build-Source 'string encoding' 'System.Console.WriteLine("??/n and 100% and \"quoted\"");' @('??/n and 100% and "quoted"')

foreach ($sample in @('hello/Hello.cs', 'projects/App/App.csproj', 'projects/NativeDemo.sln', 'projects/Arithmetic/Arithmetic.csproj')) {
    & $compiler (Join-Path $repository "examples/greencsharp/$sample") *> (Join-Path $fixture 'example.log')
    if ($LASTEXITCODE -ne 0) { throw "Example failed: $sample`n$(Get-Content -Raw (Join-Path $fixture 'example.log'))" }
}

$projectOutput = @(& (Join-Path $repository "examples/greencsharp/projects/App/bin/$profile/App.exe"))
if ($LASTEXITCODE -ne 0 -or ($projectOutput -join '|') -ne 'Project references resolved by greencsharp|21|120') {
    throw 'Referenced project executable returned incorrect results'
}

$solutionOutput = @(& (Join-Path $repository "examples/greencsharp/projects/App/bin/$profile/App.exe"))
if ($LASTEXITCODE -ne 0 -or ($solutionOutput -join '|') -ne ($projectOutput -join '|')) { throw 'Solution output mismatch' }

$explicit = Join-Path $fixture 'explicit project'
New-Item -ItemType Directory -Force (Join-Path $explicit 'src') | Out-Null
'System.Console.WriteLine("explicit");' | Set-Content (Join-Path $explicit 'src/Program & Test.cs')
'not valid C#' | Set-Content (Join-Path $explicit 'Excluded.cs')
@'
<Project Sdk="Microsoft.NET.Sdk">
  <!-- Explicit file list and XML entity decoding. -->
  <PropertyGroup><OutputType>Exe</OutputType><EnableDefaultCompileItems>false</EnableDefaultCompileItems></PropertyGroup>
  <ItemGroup><Compile Include="src/Program &amp; Test.cs" /></ItemGroup>
</Project>
'@ | Set-Content (Join-Path $explicit 'Explicit.csproj')
& $compiler (Join-Path $explicit 'Explicit.csproj') *> (Join-Path $fixture 'explicit.log')
if ($LASTEXITCODE -ne 0) { throw "Explicit project failed: $(Get-Content -Raw (Join-Path $fixture 'explicit.log'))" }

if ((@(& (Join-Path $explicit "bin/$profile/Explicit.exe")) -join '|') -ne 'explicit') { throw 'Explicit source selection failed' }

& $compiler -o (Join-Path $explicit 'alternate.c') --emit-c (Join-Path $explicit 'Explicit.csproj') *> (Join-Path $fixture 'emit.log')
if ($LASTEXITCODE -ne 0 -or !(Test-Path (Join-Path $explicit 'alternate.c'))) { throw 'Emit-C or options-before-input failed' }

Reject-Source 'type mismatch' 'int value = "wrong";'
Reject-Source 'unknown local' 'System.Console.WriteLine(missing);'
Reject-Source 'unsupported object' 'object value = new object();'
Reject-Source 'duplicate entry' 'class Program { static void Main() {} } class Other { static void Main() {} }'
Reject-Source 'reserved identifier' 'int class = 1;'
Reject-Source 'invalid return' 'class Program { static int Main() { return true; } }'
Reject-Source 'unterminated comment' '/* no closing delimiter'
Reject-Source 'unsupported escape' 'System.Console.WriteLine("\u0041");'
Reject-Source 'missing method' 'class Program { static void Main() { Missing(); } }'
Reject-Source 'overload' 'class Program { static void Main() {} static int F(int x) => x; static int F() => 0; }'
Reject-Source 'constant overflow' 'System.Console.WriteLine(2147483647 + 1);'
Reject-Source 'constant divide zero' 'System.Console.WriteLine(1 / 0);'
Reject-Source 'deep expression' ('System.Console.WriteLine(' + ((1..300 | ForEach-Object { '1' }) -join '+') + ');')
Reject-Source 'unknown namespace' 'using Does.Not.Exist; System.Console.WriteLine(1);'

foreach ($xml in @(
    '<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup Condition="true"><OutputType>Exe</OutputType></PropertyGroup></Project>',
    '<Project><ItemGroup><PackageReference Include="Something" /></ItemGroup></Project>',
    '<Project><Import Project="arbitrary.targets" /></Project>',
    '<!DOCTYPE Project [<!ENTITY external SYSTEM "file:///secret">]><Project />',
    '<Project><PropertyGroup></Project>',
    '<Project><ItemGroup><ProjectReference Include="missing.csproj" /></ItemGroup></Project>'
)) {
    Set-Content (Join-Path $fixture 'Rejected.csproj') $xml
    & $compiler (Join-Path $fixture 'Rejected.csproj') *> (Join-Path $fixture 'rejected.log')
    if ($LASTEXITCODE -eq 0) { throw "Unsupported project unexpectedly built: $xml" }
}

'<Project><ItemGroup><ProjectReference Include="CycleB.csproj" /></ItemGroup></Project>' | Set-Content (Join-Path $fixture 'CycleA.csproj')
'<Project><ItemGroup><ProjectReference Include="CycleA.csproj" /></ItemGroup></Project>' | Set-Content (Join-Path $fixture 'CycleB.csproj')
& $compiler (Join-Path $fixture 'CycleA.csproj') *> (Join-Path $fixture 'cycle.log')
if ($LASTEXITCODE -eq 0 -or (Get-Content -Raw (Join-Path $fixture 'cycle.log')) -notmatch 'cyclic') { throw 'Project cycle was not diagnosed' }

$malformedSolution = (Get-Content -Raw (Join-Path $repository 'examples/greencsharp/projects/NativeDemo.sln')).Replace('EndProject', 'WrongRecord')
Set-Content (Join-Path $fixture 'Malformed.sln') $malformedSolution
& $compiler (Join-Path $fixture 'Malformed.sln') *> (Join-Path $fixture 'solution.log')
if ($LASTEXITCODE -eq 0) { throw 'Malformed solution unexpectedly compiled' }

@'
#include "greencsharp/include/greencsharp.h"
GCS_EXPORT const greencsharp_api* greencsharp_get_api(u32 version) {
    (void)version;
    static const greencsharp_api api = {sizeof(api), 999, 0, 0, 0};
    return &api;
}
'@ | Set-Content (Join-Path $fixture 'incompatible.c')
& gcc -Os -std=c17 -DGREENC_CSHARP_BUILD "-I$(Join-Path $repository 'compiler/libs')" -shared (Join-Path $fixture 'incompatible.c') -o (Join-Path $fixture 'incompatible.dll')
if ($LASTEXITCODE -ne 0) { throw 'Cannot build incompatible ABI fixture' }

@'
#include "greencsharp/include/greencsharp.h"
static void* create(const greencsharp_request* request) { (void)request; static int context; return &context; }
static const greencsharp_result* result(void* context) {
    (void)context;
    static const greencsharp_result value = {0, 0, 1, (const char*)1, 0, 0, 0};
    return &value;
}
static void destroy(void* context) { (void)context; }
GCS_EXPORT const greencsharp_api* greencsharp_get_api(u32 version) {
    (void)version;
    static const greencsharp_api api = {sizeof(api), GREENC_CSHARP_ABI, create, result, destroy};
    return &api;
}
'@ | Set-Content (Join-Path $fixture 'malformed-result.c')
& gcc -Os -std=c17 -DGREENC_CSHARP_BUILD "-I$(Join-Path $repository 'compiler/libs')" -shared (Join-Path $fixture 'malformed-result.c') -o (Join-Path $fixture 'malformed-result.dll')
if ($LASTEXITCODE -ne 0) { throw 'Cannot build malformed result fixture' }

$previousLibrary = $env:GREENC_CSHARP_LIBRARY
try {
    $env:GREENC_CSHARP_LIBRARY = Join-Path $fixture 'missing-greencsharp.dll'
    & $compiler (Join-Path $repository 'examples/greencsharp/hello/Hello.cs') *> (Join-Path $fixture 'missing.log')
    if ($LASTEXITCODE -eq 0 -or (Get-Content -Raw (Join-Path $fixture 'missing.log')) -notmatch 'greenc cannot compile C#.*missing') {
        throw 'Missing plugin diagnostic was not returned'
    }

    $env:GREENC_CSHARP_LIBRARY = Join-Path $fixture 'incompatible.dll'
    & $compiler (Join-Path $repository 'examples/greencsharp/hello/Hello.cs') *> (Join-Path $fixture 'abi.log')
    if ($LASTEXITCODE -eq 0 -or (Get-Content -Raw (Join-Path $fixture 'abi.log')) -notmatch 'incompatible ABI') { throw 'Incompatible ABI was not rejected' }

    $env:GREENC_CSHARP_LIBRARY = Join-Path $fixture 'malformed-result.dll'
    & $compiler (Join-Path $repository 'examples/greencsharp/hello/Hello.cs') *> (Join-Path $fixture 'result.log')
    if ($LASTEXITCODE -ne 1 -or (Get-Content -Raw (Join-Path $fixture 'result.log')) -notmatch 'invalid or failed compilation result') {
        throw 'Malformed result size was not rejected safely'
    }

    'int main(void) { return 0; }' | Set-Content (Join-Path $fixture 'ordinary.c')
    & $compiler (Join-Path $fixture 'ordinary.c') --hosted-module -o (Join-Path $fixture 'ordinary.generated.c') *> (Join-Path $fixture 'ordinary.log')
    if ($LASTEXITCODE -ne 0) { throw 'Ordinary C compilation attempted to load greencsharp' }
} finally {
    $env:GREENC_CSHARP_LIBRARY = $previousLibrary
}

$previousPath = $env:PATH
try {
    $env:PATH = $fixture
    & $compiler (Join-Path $repository 'examples/greencsharp/hello/Hello.cs') -o (Join-Path $fixture 'cannot-link.exe') *> (Join-Path $fixture 'native-failure.log')
    if ($LASTEXITCODE -eq 0) { throw 'Native compiler launch failure was not propagated' }
} finally {
    $env:PATH = $previousPath
}

Write-Output 'greencsharp passed: source/project/solution builds, referenced libraries, native library output, C/headers, Main arguments, evaluation order, short-circuiting, loops, integer wrapping, source selection, diagnostics, project cycles, optional loading and native launch failure.'
