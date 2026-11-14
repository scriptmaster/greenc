# Scale regression: 24 projects, deep/diamond references, six independently linked entry points.
$ErrorActionPreference = 'Stop'
$repository = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$compiler = Join-Path $repository 'greenc.exe'
$sample = Join-Path $repository 'examples/greencsharp/large_solution'
$target = (& gcc -dumpmachine).Trim()
$profile = "$target/Release"
$log = Join-Path $repository 'bin/large-solution-test.log'
& $compiler (Join-Path $sample 'LargeDemo.sln') --rebuild *> $log
if ($LASTEXITCODE -ne 0) { throw (Get-Content -Raw $log) }
$projects = @(Get-ChildItem $sample -Recurse -Filter '*.csproj')
if ($projects.Count -ne 24) { throw 'Scale fixture must contain 24 projects' }
$expected = [ordered]@{ Server='171'; Client='78'; Worker='192'; Tool='21'; CoreTests='Core tests passed'; IntegrationTests='Integration tests passed' }
foreach ($name in $expected.Keys) {
    $actual = @(& (Join-Path $sample "$name/bin/$profile/$name.exe")) -join '|'
    if ($LASTEXITCODE -ne 0 -or $actual -ne $expected[$name]) { throw "$name execution failed: $actual" }
}
$artifacts = @(Get-ChildItem $sample -Recurse -File | Where-Object { $_.FullName -match '[\\/](obj|bin)[\\/]' })
$unexpected = @($artifacts | Where-Object { $_.Extension -notin @('.c','.h','.o','.a','.exe') })
if ($unexpected) { throw "Unexpected generated artifacts: $($unexpected.FullName -join ', ')" }
foreach ($extension in @('.c','.o','.a','.exe')) {
    $expectedCount = @{ '.c'=42; '.o'=42; '.a'=18; '.exe'=6 }[$extension]
    if (@($artifacts | Where-Object Extension -eq $extension).Count -ne $expectedCount) { throw "Incorrect $extension artifact count" }
}
# Every generated header, including semantic/cache/dependency records, must be legal C.
$headerCheck = Join-Path $repository 'bin/large-headers.c'
$headers = @($artifacts | Where-Object Extension -eq '.h')
Set-Content $headerCheck ($headers | ForEach-Object { '#include "' + $_.FullName.Replace('\','/') + '"' })
$headerIncludes = @()
foreach ($project in $projects) { $headerIncludes += @('-I', (Join-Path $project.DirectoryName "bin/$profile")) }
& gcc -std=c17 -Wall -Wextra -Werror @headerIncludes -fsyntax-only $headerCheck
if ($LASTEXITCODE -ne 0) { throw 'Generated headers are not valid C' }
# Compile all generated C independently, then link only the selected application object and archives.
$independent = Join-Path $repository 'bin/large-independent'
New-Item -ItemType Directory -Force $independent | Out-Null
$objects = @{}
foreach ($project in $projects) {
    $name = $project.BaseName
    $generated = Join-Path $project.DirectoryName "obj/$profile/$name/generated"
    $include = Join-Path $project.DirectoryName "obj/$profile/$name/include"
    $includes = @('-I', $include)
    foreach ($dependency in $projects) { $includes += @('-I', (Join-Path $dependency.DirectoryName "bin/$profile")) }
    $objects[$name] = @()
    foreach ($source in Get-ChildItem $generated -Recurse -Filter '*.c') {
        $object = Join-Path $independent ($name + '_' + $source.BaseName + '.o')
        & gcc -std=c17 -Wall -Wextra -Werror -Os @includes -c $source.FullName -o $object
        if ($LASTEXITCODE -ne 0) { throw "Independent C compile failed: $($source.FullName)" }
        $objects[$name] += $object
    }
    if ($name -like 'Library*') {
        & ar rcs (Join-Path $independent "lib$name.a") @($objects[$name])
        if ($LASTEXITCODE -ne 0) { throw 'Independent archive failed' }
    }
}
foreach ($name in $expected.Keys) {
    $archives = @(18..1 | ForEach-Object { Join-Path $independent ('libLibrary' + $_.ToString('00') + '.a') })
    $output = Join-Path $independent "$name.exe"
    & gcc @($objects[$name]) @archives -o $output
    if ($LASTEXITCODE -ne 0) { throw "Independent link failed: $name" }
    $actual = @(& $output) -join '|'
    if ($LASTEXITCODE -ne 0 -or $actual -ne $expected[$name]) { throw "Independent executable failed: $name" }
}
& $compiler (Join-Path $sample 'LargeDemo.sln') *> $log
if ($LASTEXITCODE -ne 0 -or (Get-Content -Raw $log) -match '"-c"|"rcs"|"-o"') { throw 'No-op solution build invoked native tools' }
Write-Output 'Large solution passed: 24 projects, 18 libraries, four applications, two test executables, 42 C/object units, valid headers, native-only output extensions, independent C rebuild/link/run and incremental reuse.'
