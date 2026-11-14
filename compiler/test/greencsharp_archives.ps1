# Native archive isolation, dependency invalidation, and cleanup regression checks.
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$compiler = Join-Path $root 'greenc.exe'
$profile = ((& gcc -dumpmachine).Trim()) + '/Release'
$fixture = Join-Path $root ('bin/archive checks/' + [guid]::NewGuid().ToString('N'))
foreach ($directory in @('Base/first','Base/second','Middle','App')) { New-Item -ItemType Directory -Force (Join-Path $fixture $directory) | Out-Null }
function Write-File([string]$path, [string]$value) { Set-Content (Join-Path $fixture $path) $value }
function Build([string[]]$options = @()) {
    & $compiler (Join-Path $fixture 'App/App.csproj') @options *> (Join-Path $fixture 'build.log')
    $log = Get-Content -Raw (Join-Path $fixture 'build.log')
    if ($LASTEXITCODE -ne 0) { throw $log }
    return $log
}
function Run-App([string]$expected) {
    $actual = @(& (Join-Path $fixture "App/bin/$profile/App.exe")) -join '|'
    if ($LASTEXITCODE -ne 0 -or $actual -ne $expected) { throw "Expected $expected, received $actual" }
}
Write-File 'Base/Base.csproj' '<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><AssemblyName>NativeBase</AssemblyName></PropertyGroup></Project>'
Write-File 'Base/first/Value.cs' 'namespace Demo; public static class Value { private static int Detail() => 40; public static int Get() => Detail(); internal static int Hidden() => 9; }'
Write-File 'Base/second/Value.cs' 'namespace Demo; public static class Unused { public static int Get() => 99; }'
Write-File 'Middle/Middle.csproj' '<Project Sdk="Microsoft.NET.Sdk"><ItemGroup><ProjectReference Include="../Base/Base.csproj" /></ItemGroup></Project>'
Write-File 'Middle/Bridge.cs' 'namespace Demo; public static class Bridge { public static int Get() => Value.Get() + 2; }'
Write-File 'App/App.csproj' '<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType></PropertyGroup><ItemGroup><ProjectReference Include="../Middle/Middle.csproj" /><ProjectReference Include="../Base/Base.csproj" /></ItemGroup></Project>'
Write-File 'App/Program.cs' 'System.Console.WriteLine(Demo.Bridge.Get());'
$null = Build
Run-App '42'
$archive = Join-Path $fixture "Base/bin/$profile/libNativeBase.a"
$members = @(& ar t $archive)
if ($LASTEXITCODE -ne 0 -or $members.Count -ne 2 -or $members[0] -eq $members[1]) { throw 'Archive member collision/count failure' }
$symbols = & nm -s $archive
if ($LASTEXITCODE -ne 0 -or ($symbols -join '|') -notmatch 'Archive index') { throw 'Archive has no symbol index' }
$consumer = Get-ChildItem (Join-Path $fixture "App/obj/$profile/App/objects") -Recurse -Filter '*.o' | Select-Object -First 1
$consumerTime = $consumer.LastWriteTimeUtc
$header = Join-Path $fixture "Base/bin/$profile/NativeBase.h"
$contract = Join-Path $fixture "Base/bin/$profile/NativeBase.semantic.h"
if ((Get-Content -Raw $header) -match 'Detail|Hidden') { throw 'Private/internal declarations leaked into public header' }
$unchanged = Build
if ($unchanged -match '"-c"|"rcs"|"-o"') { throw 'No-op build invoked native tools' }
Write-File 'Base/first/Value.cs' 'namespace Demo; public static class Value { private static int Detail() => 41; public static int Get() => Detail(); internal static int Hidden() => 9; }'
$changed = Build
Run-App '43'
if ((Get-Item $consumer.FullName).LastWriteTimeUtc -ne $consumerTime) { throw 'Private body change recompiled consumer' }
if ($changed -notmatch '"rcs"' -or $changed -notmatch '"-o"') { throw 'Private edit did not refresh archive and relink' }
$contractBefore = Get-Content -Raw $contract
Write-File 'Base/first/Value.cs' 'namespace Demo; public static class Value { private static int Detail() => 41; public static int Get() => Detail(); public static int Added() => 1; internal static int Hidden() => 9; }'
$null = Build
if ((Get-Content -Raw $contract) -eq $contractBefore) { throw 'Public contract change was not published' }
if ((Get-Item $consumer.FullName).LastWriteTimeUtc -eq $consumerTime) { throw 'Contract change did not invalidate consumer' }
Remove-Item -LiteralPath (Join-Path $fixture 'Base/second/Value.cs')
$null = Build
if (@(& ar t $archive).Count -ne 1) { throw 'Removed source remained in archive' }
Write-File 'App/Program.cs' 'System.Console.WriteLine(Demo.Value.Hidden());'
& $compiler (Join-Path $fixture 'App/App.csproj') *> (Join-Path $fixture 'access.log')
if ($LASTEXITCODE -eq 0) { throw 'Internal method accessible across project boundary' }
Run-App '43'
Write-File 'App/Program.cs' 'System.Console.WriteLine(Demo.Bridge.Get());'
$null = Build @('--rebuild')
Run-App '43'
$null = Build @('-c','Debug')
$manifest = Join-Path $fixture "App/obj/$profile/App/artifacts.h"
$sentinel = Join-Path $fixture 'untouched.txt'
Set-Content $sentinel 'retain'
$originalManifest = Get-Content -Raw $manifest
$hex = ($originalManifest -replace '(?s)^/\* greenc-data-v1\s*|\s*\*/\s*$', '') -replace '\s', ''
$decoded = [Text.Encoding]::UTF8.GetString([Convert]::FromHexString($hex))
$injected = $decoded + $sentinel.Replace('\','/') + "`n"
Set-Content $manifest ("/* greenc-data-v1`n" + [Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($injected)).ToLowerInvariant() + "`n*/`n") -NoNewline
& $compiler (Join-Path $fixture 'App/App.csproj') --clean *> (Join-Path $fixture 'clean.log')
if ($LASTEXITCODE -eq 0 -or !(Test-Path $sentinel)) { throw 'Unsafe clean was not rejected' }
Set-Content $manifest $originalManifest -NoNewline
$null = Build @('--clean')
if (Test-Path $archive) { throw 'Clean left archive behind' }
if (Get-ChildItem (Join-Path $fixture "Base/obj/$profile/NativeBase/objects") -Recurse -Filter '*.o') { throw 'Clean left current or removed-source object behind' }
$null = Build
Run-App '43'
Write-Output 'Native archives passed: collision-safe members/index, transitive links, incremental private edits, contract invalidation, removed sources, accessibility, failure preservation, rebuild and guarded clean.'
