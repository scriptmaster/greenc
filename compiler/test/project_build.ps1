# Integration checks for native project discovery, paths and failure propagation.
$ErrorActionPreference = 'Stop'
$repository = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$compiler = Join-Path $repository 'greenc.exe'
$fixture = Join-Path $repository ('bin/project build checks/' + [guid]::NewGuid().ToString('N'))
$source = Join-Path $fixture 'src'
$application = Join-Path $source 'app'
$entry = Join-Path $source 'main.c'
$name = Split-Path $fixture -Leaf
$executable = Join-Path $fixture "bin/$name.exe"
$log = Join-Path $fixture 'build.log'

New-Item -ItemType Directory -Force $application | Out-Null
@'
#include "answer.h"
int main(void) { return answer() == 42 ? 0 : 1; }
'@ | Set-Content $entry
'int answer(void);' | Set-Content (Join-Path $application 'answer.h')
'int answer(void) { return 42; }' | Set-Content (Join-Path $application 'answer.c')
foreach ($directory in @('first', 'second')) {
    New-Item -ItemType Directory -Force (Join-Path $application $directory) | Out-Null
}

'#define VALUE 1' | Set-Content (Join-Path $application 'first/local.h')
'#define VALUE 2' | Set-Content (Join-Path $application 'second/local.h')
@'
#include "local.h"
_Static_assert(VALUE == 1, "first source must use its own header");
'@ | Set-Content (Join-Path $application 'first/local.c')
@'
#include "local.h"
_Static_assert(VALUE == 2, "second source must use its own header");
'@ | Set-Content (Join-Path $application 'second/local.c')
New-Item -ItemType Directory -Force (Join-Path $source 'test') | Out-Null
'this intentionally invalid test must not be compiled' | Set-Content (Join-Path $source 'test/skipped.c')
$validEntry = Get-Content -Raw $entry

foreach ($arguments in @(
    @('build', $fixture.Replace('\', '/')),
    @($fixture.Replace('\project build checks\', '\project build checks/')),
    @($entry),
    @('build', $entry.Replace('\', '/'))
)) {
    & $compiler @arguments *> $log
    if ($LASTEXITCODE -ne 0) { throw "Project build failed: $arguments`n$(Get-Content -Raw $log)" }

    & $executable
    if ($LASTEXITCODE -ne 0) { throw 'Discovered nested source returned an incorrect result' }
}

foreach ($invalidEntry in @(
    'int main(void) { return undefined_symbol(); }',
    'int missing(void); int main(void) { return missing(); }',
    ('// greenc: prepare greenc_missing_native_tool_93874' + "`n" + $validEntry),
    ('// greenc: link -lgreenc_missing_library_93874' + "`n" + $validEntry)
)) {
    $invalidEntry | Set-Content $entry
    & $compiler build $fixture *> $log
    if ($LASTEXITCODE -eq 0) { throw "Invalid project unexpectedly built: $invalidEntry" }
}

@'
int main(void) {
    char* owner = acquire(); ///move
    return 0;
}
'@ | Set-Content $entry
& $compiler build $fixture *> $log
if ($LASTEXITCODE -eq 0 -or (Get-Content -Raw $log) -notmatch 'ownership:') {
    throw 'Project build did not reject an outstanding owned resource'
}

$validEntry | Set-Content $entry
& $compiler build (Join-Path $fixture 'absent') *> $log
if ($LASTEXITCODE -eq 0) { throw 'Missing project unexpectedly built' }

$standalone = Join-Path $fixture 'standalone'
New-Item -ItemType Directory -Force $standalone | Out-Null
'int main(void) { return 0; }' | Set-Content (Join-Path $standalone 'main.c')
& $compiler $standalone *> $log
if ($LASTEXITCODE -ne 0) { throw "Standalone main.c failed: $(Get-Content -Raw $log)" }

& (Join-Path $standalone 'bin/standalone.exe')
if ($LASTEXITCODE -ne 0) { throw 'Standalone main.c produced an incorrect executable' }

$libraryName = 'discovery #$ ' + [guid]::NewGuid().ToString('N')
$libraryRoot = Join-Path $repository 'compiler/libs'
$libraryFixture = Join-Path $libraryRoot $libraryName
New-Item -ItemType Directory $libraryFixture | Out-Null
try {
    @'
#pragma once
#include "beta.h"
int discovered_answer(void);
'@ | Set-Content (Join-Path $libraryFixture 'alpha.h')
    @'
#pragma once
#include "alpha.h"
int discovered_base(void);
'@ | Set-Content (Join-Path $libraryFixture 'beta.h')
    @'
#include "alpha.h"
int discovered_answer(void) { return discovered_base() + 2; }
'@ | Set-Content (Join-Path $libraryFixture 'alpha_win32.c')
    @'
#include "beta.h"
#include <windows.h>
int discovered_base(void) { return GetSystemMetrics(SM_CXSCREEN) >= 0 ? 40 : 0; }
'@ | Set-Content (Join-Path $libraryFixture 'beta implementation.c')
    "source beta implementation.c`nlink -luser32" | Set-Content (Join-Path $libraryFixture 'beta.sources')
    'not valid C; inactive includes must not select this backend' | Set-Content (Join-Path $libraryFixture 'unused_win32.c')
    '#pragma once' | Set-Content (Join-Path $libraryFixture 'unused.h')
    @"
#define LIBRARY_HEADER "$libraryName/alpha.h"
#include LIBRARY_HEADER
#include LIBRARY_HEADER
#if 0
#include "$libraryName/unused.h"
#endif
int main(void) { return discovered_answer() == 42 ? 0 : 1; }
"@ | Set-Content (Join-Path $standalone 'main.c')
    & $compiler build $standalone *> $log
    if ($LASTEXITCODE -ne 0) { throw "Include discovery failed: $(Get-Content -Raw $log)" }

    & (Join-Path $standalone 'bin/standalone.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Discovered library implementations returned an incorrect result' }

    $link = Get-Content -Raw (Join-Path $standalone 'build/greenc/link.rsp')
    foreach ($implementation in @('alpha_win32.c', 'beta implementation.c')) {
        if ([regex]::Matches($link, [regex]::Escape($implementation)).Count -ne 1) {
            throw "Implementation missing or linked more than once: $implementation"
        }
    }

    if ($link -match 'unused_win32.c') { throw 'Inactive include selected an implementation' }

    'source missing.c' | Set-Content (Join-Path $libraryFixture 'beta.sources')
    & $compiler build $standalone *> $log
    if ($LASTEXITCODE -eq 0) { throw 'A missing library implementation unexpectedly built' }

    $outsideSource = [IO.Path]::GetRelativePath($libraryFixture, (Join-Path $standalone 'main.c')).Replace('\', '/')
    "source $outsideSource" | Set-Content (Join-Path $libraryFixture 'beta.sources')
    & $compiler build $standalone *> $log
    if ($LASTEXITCODE -eq 0) { throw 'A source outside the bundled library boundary unexpectedly built' }
} finally {
    $resolvedLibrary = [IO.Path]::GetFullPath($libraryFixture)
    $expectedParent = [IO.Path]::GetFullPath($libraryRoot)
    if ([IO.Path]::GetDirectoryName($resolvedLibrary) -ne $expectedParent -or
        [IO.Path]::GetFileName($resolvedLibrary) -ne $libraryName) {
        throw 'Refusing cleanup outside the generated library fixture'
    }

    Remove-Item -LiteralPath $resolvedLibrary -Recurse -Force
}

Write-Output 'Project builds passed: entry forms, standalone main.c, nested sources, local headers, escaped paths, ownership, failure propagation, active macro includes, transitive libraries, cycles, deduplication and library link metadata.'
