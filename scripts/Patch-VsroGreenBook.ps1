[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$GlobalManagerPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$targetText = 'update TB_USER set AccPlayTime'
$replacementText = 'update TB_USER set OnlineTimee'
$encoding = [System.Text.Encoding]::ASCII
$target = $encoding.GetBytes($targetText)
$replacement = $encoding.GetBytes($replacementText)

if ($target.Length -ne $replacement.Length) {
    throw 'The binary replacement must have exactly the same byte length as the original text.'
}

function Find-BytePatternOffsets {
    param(
        [Parameter(Mandatory = $true)]
        [byte[]]$Data,
        [Parameter(Mandatory = $true)]
        [byte[]]$Pattern
    )

    $offsets = [System.Collections.Generic.List[int]]::new()
    for ($i = 0; $i -le $Data.Length - $Pattern.Length; $i++) {
        $matches = $true
        for ($j = 0; $j -lt $Pattern.Length; $j++) {
            if ($Data[$i + $j] -ne $Pattern[$j]) {
                $matches = $false
                break
            }
        }

        if ($matches) {
            $offsets.Add($i)
        }
    }

    return $offsets.ToArray()
}

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

$resolvedPath = (Resolve-Path -LiteralPath $GlobalManagerPath).Path
if ([System.IO.Path]::GetFileName($resolvedPath) -ine 'GlobalManager.exe') {
    throw "Expected GlobalManager.exe, but received: $resolvedPath"
}

$bytes = [System.IO.File]::ReadAllBytes($resolvedPath)
$originalOffsets = @(Find-BytePatternOffsets -Data $bytes -Pattern $target)
$patchedOffsets = @(Find-BytePatternOffsets -Data $bytes -Pattern $replacement)

if ($originalOffsets.Count -eq 0 -and $patchedOffsets.Count -eq 1) {
    [PSCustomObject]@{
        Status = 'AlreadyPatched'
        Path = $resolvedPath
        Offset = ('0x{0:X}' -f $patchedOffsets[0])
        Sha256 = Get-Sha256 -Path $resolvedPath
        BackupPath = $null
    }
    exit 0
}

if ($originalOffsets.Count -ne 1 -or $patchedOffsets.Count -ne 0) {
    throw "Refusing to patch. Expected one original pattern and no replacement pattern; found $($originalOffsets.Count) original and $($patchedOffsets.Count) replacement patterns."
}

$originalLength = $bytes.Length
$originalHash = Get-Sha256 -Path $resolvedPath
$backupPath = '{0}.greenbook-backup-{1}' -f $resolvedPath, (Get-Date -Format 'yyyyMMdd-HHmmss')
[System.IO.File]::Copy($resolvedPath, $backupPath, $false)

$offset = $originalOffsets[0]
[System.Array]::Copy($replacement, 0, $bytes, $offset, $replacement.Length)

try {
    [System.IO.File]::WriteAllBytes($resolvedPath, $bytes)

    $verifiedBytes = [System.IO.File]::ReadAllBytes($resolvedPath)
    $verifiedOriginal = @(Find-BytePatternOffsets -Data $verifiedBytes -Pattern $target)
    $verifiedReplacement = @(Find-BytePatternOffsets -Data $verifiedBytes -Pattern $replacement)

    if ($verifiedBytes.Length -ne $originalLength -or
        $verifiedOriginal.Count -ne 0 -or
        $verifiedReplacement.Count -ne 1 -or
        $verifiedReplacement[0] -ne $offset) {
        throw 'Post-write verification failed.'
    }
}
catch {
    [System.IO.File]::Copy($backupPath, $resolvedPath, $true)
    throw
}

[PSCustomObject]@{
    Status = 'Patched'
    Path = $resolvedPath
    Offset = ('0x{0:X}' -f $offset)
    OriginalSha256 = $originalHash
    PatchedSha256 = Get-Sha256 -Path $resolvedPath
    BackupPath = $backupPath
    Length = $originalLength
}
