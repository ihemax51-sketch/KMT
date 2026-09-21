param(
    [Parameter(Mandatory = $true)]
    [string]$LauncherPath
)

$ErrorActionPreference = 'Stop'

$expectedSha256 = 'E17853E3FB61D649414A66F8047124D8133ADE70C863077D116ABD6457BA4461'
$branchOffset = 0x358B3
$expectedBytes = [byte[]](0x75, 0x2C)
$patchedBytes = [byte[]](0xEB, 0x2C)

$resolvedLauncher = (Resolve-Path -LiteralPath $LauncherPath).Path
$actualSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolvedLauncher).Hash
if ($actualSha256 -ne $expectedSha256) {
    throw "Unsupported silkroad.exe SHA-256: $actualSha256"
}

$bytes = [System.IO.File]::ReadAllBytes($resolvedLauncher)
if ($bytes.Length -le ($branchOffset + 1)) {
    throw 'The launcher is smaller than the verified patch offset.'
}

if ($bytes[$branchOffset] -ne $expectedBytes[0] -or
    $bytes[$branchOffset + 1] -ne $expectedBytes[1]) {
    throw ('Unexpected branch bytes at 0x{0:X}: {1:X2} {2:X2}' -f
        $branchOffset,
        $bytes[$branchOffset],
        $bytes[$branchOffset + 1])
}

$backupPath = Join-Path (Split-Path -Parent $resolvedLauncher) 'silkroad.original.exe'
if (-not (Test-Path -LiteralPath $backupPath)) {
    [System.IO.File]::Copy($resolvedLauncher, $backupPath, $false)
}

$bytes[$branchOffset] = $patchedBytes[0]
$bytes[$branchOffset + 1] = $patchedBytes[1]
[System.IO.File]::WriteAllBytes($resolvedLauncher, $bytes)

$patchedSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolvedLauncher).Hash
Write-Output "Patched launcher: $resolvedLauncher"
Write-Output "Original backup: $backupPath"
Write-Output "Patched SHA-256: $patchedSha256"
