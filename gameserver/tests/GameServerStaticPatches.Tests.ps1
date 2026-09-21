$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$gameServerRoot = Join-Path $repositoryRoot 'gameserver\source\SilkroadOnline\SR_VIETNAM_SERVICE\v188\Server\SR_GameServer'
$staticPatches = Get-Content -LiteralPath (Join-Path $gameServerRoot 'OutPut\src\StaticPatches.cpp') -Raw
$runtimeSettings = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\SqlConnection\sqlCon.cpp') -Raw

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$combinedSource = $staticPatches + "`n" + $runtimeSettings
Assert-True ([regex]::Matches($combinedSource, '0x005805C8').Count -eq 1) 'Quest RaiseEvent block is owned by more than one runtime patch path.'
Assert-True ([regex]::Matches($combinedSource, '0x005805D3').Count -eq 1) 'Quest RaiseEvent call is owned by more than one runtime patch path.'
Assert-True ($staticPatches -match 'kQuestRaiseEventLogCallOriginal[\s\S]*?0xE8,\s*0x68,\s*0x60,\s*0x3B,\s*0x00') 'The original Quest RaiseEvent CALL signature is incomplete.'
Assert-True ($staticPatches -match 'kQuestRaiseEventLogCallSuppressed[\s\S]*?0x90,\s*0x90,\s*0x90,\s*0x90,\s*0x90') 'The Quest RaiseEvent patch must suppress exactly the five-byte CALL.'
Assert-True ($staticPatches -match 'MatchesBytes\([\s\S]*?QUEST_RAISE_EVENT_LOG_BLOCK_OFFSET[\s\S]*?kQuestRaiseEventLogBlockOriginal') 'The complete surrounding instruction block is not validated before patching.'
Assert-True ($staticPatches -match 'WriteRaw\([\s\S]*?QUEST_RAISE_EVENT_LOG_CALL_OFFSET[\s\S]*?kQuestRaiseEventLogCallSuppressed[\s\S]*?Commit\(\)') 'The diagnostic CALL is not applied transactionally.'
Assert-True ($staticPatches -match 'RestoreBytes\([\s\S]*?QUEST_RAISE_EVENT_LOG_CALL_OFFSET[\s\S]*?kQuestRaiseEventLogCallOriginal') 'The diagnostic CALL rollback is missing.'
Assert-True ($runtimeSettings -notmatch 'QUEST_RAISE_EVENT|questRaiseEvent|0x005805C8|0x005805D3') 'Runtime settings contain a duplicate Quest RaiseEvent patch.'

Write-Output 'GameServer static patch ownership checks: PASS'
