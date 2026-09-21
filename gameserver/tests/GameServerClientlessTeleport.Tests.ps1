$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sourcePath = Join-Path $repositoryRoot 'gameserver\source\SilkroadOnline\SR_VIETNAM_SERVICE\v188\Server\SR_GameServer\GameServer\GameServer\src\KMTGuardCustom\CGObjPCCustom.cpp'
$source = Get-Content -LiteralPath $sourcePath -Raw

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$handlerMatch = [regex]::Match(
    $source,
    'void\s+CGObjPC::HandleFilterTeleportRequest\s*\(CMsg\*\s*pMsg\)\s*\{(?<Body>[\s\S]*?)\r?\n\}',
    [System.Text.RegularExpressions.RegexOptions]::CultureInvariant)

Assert-True $handlerMatch.Success 'The authenticated Clientless teleport handler is missing.'
$handler = $handlerMatch.Groups['Body'].Value

Assert-True ($handler -match 'if\s*\(\s*!this->MoveTo\([\s\S]*?,\s*2\s*\)\s*\)') 'Clientless teleport must begin with the native mode-2 world-transfer handshake.'
Assert-True ($handler -match 'if\s*\(\s*!this->MoveTo\([\s\S]*?,\s*2\s*\)\s*\)\s*\{[\s\S]*?this->MoveTo\([\s\S]*?,\s*1\s*\)') 'Clientless teleport must retain mode 1 only as the established fallback.'
Assert-True ($handler -notmatch 'direct-position mode only') 'Clientless teleport must not force the incomplete direct-position path.'

Write-Output 'GameServer Clientless teleport contract checks: PASS'
