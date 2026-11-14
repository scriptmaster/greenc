param([Parameter(Mandatory)][ValidateSet("Install", "Remove")] [string]$Action)
$ErrorActionPreference = "Stop"
$publisher = "CN=GreenC Editor, OID.2.25.311729368913984317654407730594956997722=1"

try {
    $bin = (Resolve-Path (Join-Path $PSScriptRoot "../bin")).Path
    $existing = Get-AppxPackage -Name GreenC.Editor.Explorer | Where-Object Publisher -eq $publisher
    if ($Action -eq "Remove") {
        foreach ($package in $existing) {
            Remove-AppxPackage -Package $package.PackageFullName
        }
    } else {
        foreach ($name in "GreenCExplorer.msix", "GreenCExplorer.dll", "editor.exe") {
            if (!(Test-Path -LiteralPath (Join-Path $bin $name) -PathType Leaf)) { throw "Missing $name in $bin" }
        }
        Add-AppxPackage -Path (Join-Path $bin "GreenCExplorer.msix") -ExternalLocation $bin -AllowUnsigned
        $installed = Get-AppxPackage -Name GreenC.Editor.Explorer | Where-Object Publisher -eq $publisher
        if (!$installed -or $installed.Status -ne "Ok") { throw "Windows did not report a healthy Explorer package." }
    }
    exit 0
} catch {
    $log = Join-Path $env:LOCALAPPDATA "GreenC-editor-explorer-install.log"
    $_ | Out-String | Set-Content -LiteralPath $log
    exit 1
}
