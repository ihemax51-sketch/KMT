param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "Build-Component.ps1") `
    -Component All `
    -Configuration $Configuration
exit $LASTEXITCODE
