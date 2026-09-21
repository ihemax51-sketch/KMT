$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sourceRoot = Join-Path $repositoryRoot 'gameserver\source\SilkroadOnline\SR_VIETNAM_SERVICE\v188\Server\SR_GameServer'
$tradeSource = Get-Content -LiteralPath (Join-Path $sourceRoot 'GameServer\GameServer\src\KMTGuardCustom\TradeGoldControl.cpp') -Raw
$settingsHeader = Get-Content -LiteralPath (Join-Path $sourceRoot 'GameServer\GameServer\src\SettingMgr\NewSettings.h') -Raw
$settingsSource = Get-Content -LiteralPath (Join-Path $sourceRoot 'GameServer\GameServer\src\SettingMgr\NewSettings.cpp') -Raw
$sqlSource = Get-Content -LiteralPath (Join-Path $sourceRoot 'GameServer\GameServer\src\SqlConnection\sqlCon.cpp') -Raw
$initSource = Get-Content -LiteralPath (Join-Path $sourceRoot 'OutPut\src\Util.cpp') -Raw
$desktopSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'KMTGuard.AdminDesktop\MainWindow.xaml.cs') -Raw
$migration = Get-Content -LiteralPath (Join-Path $repositoryRoot 'database\migrations\20260909_disable_original_trade_gold.sql') -Raw

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

Assert-True ($settingsHeader -match 'bool DisableOriginalTradeGold;') 'The global GameServer setting field is missing.'
Assert-True ($settingsSource -match 'DisableOriginalTradeGold = false;') 'DisableOriginalTradeGold does not default to OFF.'
Assert-True ($sqlSource -match 'strcmp\(name, "DisableOriginalTradeGold"\) == 0') 'DisableOriginalTradeGold is not validated as a boolean.'
Assert-True ($sqlSource -match 'm_Settings->DisableOriginalTradeGold\s*=') 'DisableOriginalTradeGold is not loaded from System_GameServerSettings.'

Assert-True ($tradeSource -match 'kTradeProfitScaleCallAddress = 0x004C8DBC') 'The trade profit call-site address is incorrect.'
Assert-True ($tradeSource -match '0xE8, 0xCF, 0xD5, 0xFB, 0xFF') 'The original scale-helper CALL signature is missing.'
Assert-True ($tradeSource -match 'kScaleHelperEpilogueAddress = 0x004863B0') 'The original helper epilogue is not validated.'
Assert-True ($tradeSource -match '0x59, 0xC2, 0x08, 0x00') 'The original ret 8 stack-cleanup signature is missing.'
Assert-True ($tradeSource -match 'mov eax, dword ptr \[esp \+ 4\]') 'The hook does not return the original low DWORD.'
Assert-True ($tradeSource -match 'mov edx, dword ptr \[esp \+ 8\]') 'The hook does not return the original high DWORD.'
Assert-True ($tradeSource -match 'ret 8') 'The hook does not preserve the original callee stack cleanup.'
Assert-True ($tradeSource -match 'if \(!CNewSettings::m_Settings->DisableOriginalTradeGold\)') 'OFF mode is not fail-open to the original calculation.'
Assert-True ($tradeSource -match 'GameServerMemoryPatchTransaction transaction;') 'The trade call replacement is not transactional.'
Assert-True ($tradeSource -match 'RestoreBytes\(') 'The trade call replacement is not reversible.'
Assert-True ($tradeSource -notmatch '0x006194E0|0x00619674|0x0061968E|0x7034|UpdateGold') 'The hook touches a prohibited sell-handler, opcode, or UpdateGold path.'

Assert-True ($initSource -match '!TradeGoldControl::Initialize\(\)') 'TradeGoldControl is not initialized during GameServer startup.'
Assert-True ($initSource -match 'TradeGoldControl::Shutdown\(\)') 'TradeGoldControl is not included in initialization rollback.'
Assert-True ($desktopSource -match 'T\("DisableOriginalTradeGold", "Disable Original Trade Gold"') 'The dashboard toggle is missing.'
Assert-True ($desktopSource -match 'new\(name, "GameServer.Trade"') 'The toggle is not assigned to the Trade System section.'
Assert-True ($desktopSource -match 'Changes require GameServer restart') 'The dashboard restart warning is missing.'
Assert-True ($migration -match "VALUES \('DisableOriginalTradeGold', '0'\)") 'The database setting does not use logical BIT default 0.'

Write-Output 'GameServer original trade gold control tests: PASS'
