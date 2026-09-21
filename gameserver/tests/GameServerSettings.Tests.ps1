$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sourceRoot = Join-Path $repositoryRoot 'gameserver\source\SilkroadOnline\SR_VIETNAM_SERVICE\v188\Server\SR_GameServer'
$sqlSource = Get-Content -LiteralPath (Join-Path $sourceRoot 'GameServer\GameServer\src\SqlConnection\sqlCon.cpp') -Raw
$settingsSource = Get-Content -LiteralPath (Join-Path $sourceRoot 'GameServer\GameServer\src\SettingMgr\NewSettings.cpp') -Raw
$clientSettingsSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'JTClientLibrary\source\libs\ClientLib\src\PSTitle.cpp') -Raw
$clientSkillBoardSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'JTClientLibrary\source\libs\ClientLib\src\IFSkillBoard.cpp') -Raw
$clientSkillAutomationSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'JTClientLibrary\source\libs\ClientLib\src\SkillAutomationController.cpp') -Raw
$filterSettingsSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'filter\KMTGuardnew\KMTGuard\Database\Models\__Settings.cs') -Raw
$filterPacketSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'filter\KMTGuardnew\KMTGuard\Server\GatewayServer\PacketHandler\SERVER_DLL_SETTINGS_RESPONSE.cs') -Raw
$desktopSource = Get-Content -LiteralPath (Join-Path $repositoryRoot 'KMTGuard.AdminDesktop\MainWindow.xaml.cs') -Raw
$desktopLayout = Get-Content -LiteralPath (Join-Path $repositoryRoot 'KMTGuard.AdminDesktop\MainWindow.xaml') -Raw
$sqlMigration = Get-Content -LiteralPath (Join-Path $repositoryRoot 'database\migrations\20260810_gameserver_patch_dashboard.sql') -Raw

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

Assert-True ($sqlSource -match 'm_Settings->PENALTY_DROP_LEVEL_MIN') 'PK drop penalty is not applied from its canonical setting.'
Assert-True ($sqlSource -notmatch 'm_Settings->MIN_PK_LEVEL_FOR_DROP_ITEM') 'Retired PK drop setting remains active.'
Assert-True ($sqlSource -match 'm_Settings->MinItemLevelForAstralToTakeEffect') 'Astral activation setting is not applied.'
Assert-True ($sqlSource -match 'm_Settings->ItemLevelForAstralRecovery') 'Astral recovery setting is not applied.'
Assert-True ($sqlSource -notmatch 'm_Settings->STONE_ASTRAL_VALUE') 'Retired combined Astral setting remains active.'
$settingsStart = $sqlSource.IndexOf('bool CSqlCon::ApplyRuntimeSettings()')
$settingsEnd = $sqlSource.IndexOf('CRegionRestrictionDBSet* CSqlCon::GetRegionRestrictionDbSet()', $settingsStart)
Assert-True ($settingsStart -ge 0 -and $settingsEnd -gt $settingsStart) 'Runtime settings function boundaries are missing.'
$settingsMethod = $sqlSource.Substring($settingsStart, $settingsEnd - $settingsStart)
Assert-True ([regex]::Matches($settingsMethod, '#pragma\s+push_macro\("BS_INFO"\)').Count -eq 1) 'Runtime setting detail suppression is not opened exactly once.'
Assert-True ([regex]::Matches($settingsMethod, '#pragma\s+pop_macro\("BS_INFO"\)').Count -eq 1) 'Runtime setting detail suppression is not closed exactly once.'
Assert-True ($settingsMethod.IndexOf('#pragma pop_macro("BS_INFO")') -lt $settingsMethod.IndexOf('Runtime configuration rolled back')) 'Runtime setting failures are hidden from the operator console.'
$initializeStart = $sqlSource.IndexOf('bool CSqlCon::Initialize()')
$initializeEnd = $sqlSource.IndexOf('bool CSqlCon::LoadInternalPacketSharedSecret()', $initializeStart)
Assert-True ($initializeStart -ge 0 -and $initializeEnd -gt $initializeStart) 'GameServer initialization function boundaries are missing.'
$initializeMethod = $sqlSource.Substring($initializeStart, $initializeEnd - $initializeStart)
Assert-True ([regex]::Matches($initializeMethod, 'if \(!ApplyRuntimeSettings\(\)\)').Count -eq 1) 'Runtime settings are not applied exactly once during startup.'
Assert-True ($initializeMethod.IndexOf('CNewSettings::Validate(validationError)') -lt $initializeMethod.IndexOf('if (!ApplyRuntimeSettings())')) 'Runtime settings are applied before validation.'
Assert-True ($initializeMethod.IndexOf('if (!ApplyRuntimeSettings())') -lt $initializeMethod.IndexOf('if (!LoadLockedItems())')) 'Runtime settings are applied after dependent caches load.'
Assert-True ($sqlSource -match 'm_Settings->CH_MAX_MASTERY_LEVEL') 'Chinese mastery setting is not wired to GameServer runtime settings.'
Assert-True ($sqlSource -match 'm_Settings->EU_MAX_MASTERY_LEVEL') 'European mastery setting is not wired to GameServer runtime settings.'
foreach ($address in @('0x006A5197', '0x006A51A2', '0x006AA498', '0x006AA4A3')) {
    Assert-True ($clientSettingsSource -match [regex]::Escape($address)) "European mastery Client patch is missing $address."
}
Assert-True ($clientSettingsSource -match 'BuildMasteryTotalSelection') 'Client European total still depends on the native twice-character-level selection.'
Assert-True ($clientSettingsSource -match 'Unsupported native layout') 'Client mastery patch does not fail closed for an unsupported executable layout.'
Assert-True ($sqlSource -match 'ReadMemoryValue<double>\(0x00B46130') 'GameServer reads the European mastery cap using the wrong width.'
Assert-True ($sqlSource -match 'WriteMemoryValue<double>\(0x00B46130') 'GameServer writes the European mastery cap using the wrong width.'
Assert-True ($sqlSource -match 'europeanLevelBypass') 'GameServer still enforces total European mastery as twice the character level.'
Assert-True ($sqlSource -match 'GameServerRuntimeSafety::MatchesBytes\(0x0059C58B') 'GameServer does not validate the European mastery instructions before patching.'
Assert-True ($clientSettingsSource -notmatch 'PatchMe\(0x006AA4C3 \+ 1, m_Settings->') 'Client mastery patch still truncates the Chinese value to one byte.'
Assert-True ($clientSettingsSource -notmatch 'PatchMe\(0x006A5197 \+ 1, m_Settings->') 'Client mastery patch still truncates the European value to one byte.'
Assert-True ([regex]::Matches($clientSkillBoardSource, 'KmtApplyConfiguredMasteryLimits\(\)').Count -ge 2) 'Client mastery limits are not reapplied when the gameplay skill interface is created and selected.'
Assert-True ($clientSkillAutomationSource -match 'ChineseMasteryLimit' -and $clientSkillAutomationSource -match 'EuropeanMasteryLimit') 'Client auto mastery does not use the configured race-specific limits.'
Assert-True ($filterSettingsSource -match 'ChineseMasteryLimit' -and $filterSettingsSource -match 'EuropeanMasteryLimit') 'Filter does not load both effective GameServer mastery limits.'
Assert-True ($filterPacketSource -match 'WriteInt32\(_serverSettings\.ChineseMasteryLimit\)' -and $filterPacketSource -match 'WriteInt32\(_serverSettings\.EuropeanMasteryLimit\)') 'Filter does not append both mastery limits to the DLL settings packet.'
Assert-True ($filterPacketSource -match '(?s)dc\.WriteInt32\(Math\.Max\(\s*_serverSettings\.MasteryLimit,\s*Math\.Max\(\s*_serverSettings\.ChineseMasteryLimit,\s*_serverSettings\.EuropeanMasteryLimit') 'The legacy Client mastery field can still leave the European limit at 240.'
Assert-True ($sqlSource -notmatch 'questRaiseEventDiagnostic|0x005805C8') 'The withdrawn Quest RaiseEvent runtime patch is still present.'
Assert-True ($settingsSource -match 'KMT_REQUIRE_RANGE\(PENALTY_DROP_LEVEL_MIN, 1, 255\)') 'PK drop range validation is missing.'
Assert-True ($settingsSource -match 'KMT_REQUIRE_RANGE\(MinItemLevelForAstralToTakeEffect, 0, 255\)') 'Astral activation range validation is missing.'
Assert-True ($settingsSource -match 'KMT_REQUIRE_RANGE\(ItemLevelForAstralRecovery, 0, 255\)') 'Astral recovery range validation is missing.'

$loaderSettings = [regex]::Matches($sqlSource, 'strcmp\(\(char\*\)szSettingName, "([^"]+)"\)') |
    ForEach-Object { $_.Groups[1].Value } |
    Sort-Object -Unique
$methodStart = $desktopSource.IndexOf('private static IReadOnlyDictionary<string, SettingDefinition> BuildGameServerSettingDefinitions()')
$methodEnd = $desktopSource.IndexOf('    private enum SettingKind', $methodStart)
Assert-True ($methodStart -ge 0 -and $methodEnd -gt $methodStart) 'Gameserver Patch dashboard catalog is missing.'
$dashboardMethod = $desktopSource.Substring($methodStart, $methodEnd - $methodStart)
$dashboardSettings = [regex]::Matches($dashboardMethod, '[DT]\("([^"]+)"') |
    ForEach-Object { $_.Groups[1].Value } |
    Sort-Object -Unique
$difference = Compare-Object $loaderSettings $dashboardSettings
Assert-True ($null -eq $difference) 'The Gameserver Patch dashboard does not cover the full GameServer loader catalog.'
Assert-True ($desktopSource -match 'Restart every GameServer after saving') 'The required GameServer restart warning is missing.'
Assert-True ($desktopLayout -match '(?s)<ListBoxItem Tag="Settings">.*?</ListBoxItem>\s*<ListBoxItem Tag="GameServerPatch">') 'Gameserver Patch is not a standalone navigation page directly below Filter Settings.'
Assert-True ($desktopSource -match 'page is "Settings" or "GameServerPatch"') 'The standalone Gameserver Patch page is not routed through the settings editor.'
Assert-True ($desktopSource -match '\.Where\(setting => setting\.Store == _settingsStoreView\)') 'Filter and GameServer settings are not isolated by their owning store.'
Assert-True ($desktopSource -match 'new SectionDefinition\("GameServer\.Trade", "Trade System"') 'The GameServer Trade System section is missing.'
Assert-True ($desktopSource -match 'SettingsSectionsPanel\.Visibility = Visibility\.Visible') 'The GameServer settings section selector is not visible.'

foreach ($movedSetting in @('DisableDurability','DisableGreenBook','EnablePartyMonsterSpawn','PartyMonsterMinimumMembers','PartyMonsterSpawnRate')) {
    Assert-True ($sqlMigration -match "DELETE dbo.System_Settings[\s\S]*?$movedSetting") "$movedSetting is not removed from System_Settings."
}

Write-Output "GameServer settings contract tests: PASS ($($loaderSettings.Count) dashboard settings)"
