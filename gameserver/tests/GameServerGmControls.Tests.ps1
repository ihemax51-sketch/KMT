$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$gameServerRoot = Join-Path $repositoryRoot 'gameserver\source\SilkroadOnline\SR_VIETNAM_SERVICE\v188\Server\SR_GameServer'
$settingsHeader = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\SettingMgr\NewSettings.h') -Raw
$settingsSource = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\SettingMgr\NewSettings.cpp') -Raw
$sqlSource = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\SqlConnection\sqlCon.cpp') -Raw
$playerSource = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\Objects\GObjPC.cpp') -Raw
$patchSource = Get-Content -LiteralPath (Join-Path $gameServerRoot 'OutPut\src\StaticPatches.cpp') -Raw
$safetySource = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\KMTGuardCustom\GameServerRuntimeSafety.cpp') -Raw
$desktopSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'KMTGuard.AdminDesktop\MainWindow.xaml.cs') -Raw
$migration = Get-Content -LiteralPath (Join-Path $repositoryRoot 'database\migrations\20260829_gm_runtime_controls.sql') -Raw

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

foreach ($setting in @('ShowGmUniqueKillNotice', 'ForceGmVisibleOnSpawn')) {
    Assert-True ($settingsHeader -match "bool\s+$setting\s*;") "$setting is missing from GameCfgStruct."
    Assert-True ($settingsSource -match "m_Settings->$setting\s*=\s*false\s*;") "$setting does not have a safe False default."
    Assert-True ($sqlSource -match "IsCoreBooleanSetting[\s\S]*?$setting") "$setting is not validated as a boolean."
    Assert-True ($sqlSource -match "strcmp\(\(char\*\)szSettingName, `"$setting`"\)[\s\S]*?m_Settings->$setting") "$setting is not loaded from the database."
    Assert-True ($desktopSource -match "D\(`"$setting`"[\s\S]*?SettingKind.Boolean") "$setting is missing from the GameServer dashboard."
    Assert-True ($migration -match "\('$setting',\s*'False'\)") "$setting is not added to SQL with a False default."
}

Assert-True ($patchSource -match '#define\s+GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET\s+0x004C1D23') 'The verified GM Unique notice branch is not used.'
Assert-True ($patchSource -match 'kGmUniqueKillNoticeBranchOriginal[\s\S]*?0x0F,\s*0x84,\s*0xB7,\s*0x00,\s*0x00,\s*0x00') 'The original GM Unique notice instruction signature is incomplete.'
Assert-True ($patchSource -match 'ShowGmUniqueKillNotice[\s\S]*?MatchesBytes[\s\S]*?GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET') 'The GM Unique notice patch is not setting-controlled or validated.'
Assert-True ($patchSource -match 'WriteRaw\([\s\S]*?GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET[\s\S]*?Commit\(\)') 'The GM Unique notice patch is not transactional.'
Assert-True ($patchSource -match 'RestoreBytes\([\s\S]*?GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET[\s\S]*?kGmUniqueKillNoticeBranchOriginal') 'The GM Unique notice rollback is missing.'
Assert-True ($safetySource -match '0x004C1D23[\s\S]*?SIGNATURE_GM_UNIQUE_KILL_NOTICE') 'The GM Unique notice branch is missing from host compatibility checks.'

$spawnStart = $playerSource.IndexOf('unsigned int CGObjPC::CharacterSpawn_VfTable391()')
$spawnEnd = $playerSource.IndexOf('CRgnTerrain *CGObjPC::GetRgnTerrain()', $spawnStart)
Assert-True ($spawnStart -ge 0 -and $spawnEnd -gt $spawnStart) 'The player spawn hook boundaries are missing.'
$spawnMethod = $playerSource.Substring($spawnStart, $spawnEnd - $spawnStart)
$clearPosition = $spawnMethod.IndexOf('m_btBodyState = BODYMODE_NORMAL')
$nativeSpawnPosition = $spawnMethod.IndexOf('0x004df9e0')
Assert-True ($spawnMethod -match 'ForceGmVisibleOnSpawn') 'The player spawn hook is not controlled by ForceGmVisibleOnSpawn.'
Assert-True ($spawnMethod -match 'm_pLifeState\s*!=\s*NULL') 'The player spawn hook does not validate the life-state pointer.'
Assert-True ($spawnMethod -match 'm_btBodyState\s*==\s*BODYMODE_GM_INVISIBLE') 'The player spawn hook is not limited to GM Invisible state.'
Assert-True ($clearPosition -ge 0 -and $clearPosition -lt $nativeSpawnPosition) 'GM Invisible state must be cleared before native spawn serialization.'

Write-Output 'GameServer GM controls contract tests: PASS'
