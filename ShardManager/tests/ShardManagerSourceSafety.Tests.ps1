$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projectRoot = Join-Path $repositoryRoot 'ShardManager\vSRO-ShardManager'

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

$project = Get-Content -LiteralPath (Join-Path $projectRoot 'vSRO-ShardManager.vcxproj') -Raw
$dllMain = Get-Content -LiteralPath (Join-Path $projectRoot 'DllMain.cpp') -Raw
$safeMessages = Get-Content -LiteralPath (Join-Path $projectRoot 'Network\MainProcessSafe.cpp') -Raw
$messageBuffer = Get-Content -LiteralPath (Join-Path $projectRoot 'Network\MsgStreamBuffer.cpp') -Raw
$sqlConnection = Get-Content -LiteralPath (Join-Path $projectRoot 'Database\SQLConnection.cpp') -Raw
$sqlCommand = Get-Content -LiteralPath (Join-Path $projectRoot 'Database\SQLCommand.cpp') -Raw
$asyncCommands = Get-Content -LiteralPath (Join-Path $projectRoot 'AsyncGSCommands.cpp') -Raw
$safeSettings = Get-Content -LiteralPath (Join-Path $projectRoot 'SettingManagers\SettingsSafe.cpp') -Raw
$localSettingsTemplate = Join-Path $projectRoot 'Outpus\KMTGuard-Addon.ini'
$deliverySettingsTemplate = 'D:\KMTGuard-build\ServerAddons\ShardManager\KMTGuard-Addon.ini'
$settingsTemplatePath = if (Test-Path -LiteralPath $localSettingsTemplate -PathType Leaf) {
    $localSettingsTemplate
} else {
    $deliverySettingsTemplate
}
$settingsTemplate = Get-Content -LiteralPath $settingsTemplatePath -Raw
$moduleSettings = Get-Content -LiteralPath (Join-Path $projectRoot 'SettingManagers\ModuleSettingsManager.cpp') -Raw

Assert-True ($project -notmatch 'ClCompile Include="Network\\MainProcess\.cpp"') 'Legacy message routing is still compiled.'
Assert-True ($project -notmatch 'ClCompile Include="SettingManagers\\Settings\.cpp"') 'Legacy settings loader is still compiled.'
Assert-True ($project -notmatch 'ClCompile Include="Utils\\BSObj\.cpp"') 'Legacy console formatter is still compiled.'
Assert-True ($project -match 'Console\\ShardManagerConsole\.cpp') 'Professional console implementation is missing from the build.'
Assert-True ($project -match 'Runtime\\ShardManagerRuntimeSafety\.cpp') 'Runtime compatibility guard is missing from the build.'
Assert-True ($project -notmatch 'DetachedThread\.h') 'Command workers are still compiled with detached fake thread handles.'
Assert-True ($project -notmatch 'ForcedIncludeFiles[^<]*DetachedThread') 'The command worker still overrides native thread ownership.'

Assert-True ($dllMain -match 'TerminateProcess\(GetCurrentProcess\(\), ERROR_DLL_INIT_FAILED\)') 'Fail-closed process termination is missing.'
Assert-True ($dllMain -match 'ShardManagerRuntimeSafety::RollbackHooks\(\)') 'Initialization rollback is missing.'

foreach ($opcode in @('READ_LOCK_INFO', 'READ_UNLOCK_INFO', 'READ_TIMED_ITEM', 'READ_TIMED_ITEM_REMOVE', 'READ_LINKED_CHAT')) {
    Assert-True ($safeMessages.Contains("case $opcode")) "Safe routing is missing for $opcode."
}
Assert-True ($safeMessages -match 'TryReadStringA\(receiverName, 64\)') 'Receiver-name length guard is missing.'
Assert-True ($safeMessages -match 'TryReadStringA\(text, 1024\)') 'Chat-text length guard is missing.'
Assert-True ($safeMessages -match 'SR_ShardManager is initialized successfully') 'Native ShardManager readiness trigger is missing.'
Assert-True ($safeMessages -match 'ShardManager startup completed successfully') 'Native ShardManager readiness stamp is missing.'
Assert-True ($safeMessages -match 'InterlockedCompareExchange\(&s_readyStampWritten, 1, 0\)') 'Native ShardManager readiness stamp is not one-shot.'
$nativeOriginalIndex = $safeMessages.IndexOf('const int result = s_originalHandleMessage')
$nativeStampIndex = $safeMessages.LastIndexOf('WriteShardManagerReadyStamp(')
Assert-True ($nativeOriginalIndex -ge 0 -and $nativeStampIndex -gt $nativeOriginalIndex) 'KMTGuard readiness is emitted before the original ShardManager completion message.'
Assert-True ($messageBuffer -match 'count <= static_cast<size_t>\(writePosition - readPosition\)') 'Packet read-boundary guard is missing.'

Assert-True ($sqlConnection -match 'SQLConnection::~SQLConnection\(\)[\s\S]*?Close\(\)') 'ODBC destructor cleanup is missing.'
Assert-True ($sqlConnection -match '_countof\(retConString\)') 'ODBC output buffer length is not bounded correctly.'
Assert-True ($asyncCommands -match 'Command_ClaimGameServer') 'Durable GameServer command claiming is missing.'
Assert-True ($asyncCommands -match 'TryConsumeBroadcast\(\)') 'Command broadcast guard is missing.'
Assert-True ($asyncCommands -match 'MAX_COMMANDS_PER_SECOND') 'The command bridge has no bounded backlog-drain rate.'
Assert-True ($asyncCommands -match 'ReconnectDatabaseLink') 'The command bridge cannot recover a failed database session.'
Assert-True ($asyncCommands -match 'CommandBridgeMonitorThread') 'The command bridge health monitor is missing.'
Assert-True ($asyncCommands -match 'CommandWorkerThreadEntry') 'The command bridge worker has no structured-exception boundary.'
Assert-True ($asyncCommands -match 'RestartCommandWorker') 'The command bridge monitor cannot restart an unexpectedly stopped worker.'
Assert-True ($asyncCommands -notmatch 'signal\s*\(\s*SIGINT') 'The command bridge must not install a process-wide SIGINT handler.'
Assert-True ($asyncCommands -notmatch 'while\s*\([^\)]*ExecuteClaimCompletion') 'A completion failure can still block the command queue forever.'
Assert-True ($sqlCommand -match 'SQL_FETCH_RESULT SQLCommand::FetchDataResult') 'ODBC fetch errors cannot be distinguished from an empty queue.'
Assert-True ($safeSettings -match 'AsyncGSCommands::Shutdown\(\)') 'The command bridge is not stopped during ShardManager rollback.'
Assert-True ($moduleSettings -match 'System_GameServerSettings') 'ShardManager does not read the unified GameServer setting catalog.'
Assert-True ($moduleSettings -match "N'GUILD_POINTS' THEN N'FixNegativeGuildPoint'") 'Guild-point protection is not bound to GUILD_POINTS.'
Assert-True ($moduleSettings -match "N'UNION_LIMIT' THEN N'UnionLimit'") 'Union enforcement is not bound to UNION_LIMIT.'
Assert-True ($moduleSettings -match "SettingName NOT IN \(N'UnionLimit', N'FixNegativeGuildPoint'\)") 'Retired duplicate ShardManager settings are still active.'
Assert-True ($safeSettings -match 'unionLimitLoaded' -and $safeSettings -match 'Required UNION_LIMIT setting is missing') 'UNION_LIMIT is not required during ShardManager startup.'
Assert-True ($safeSettings -match 'guildPointProtectionLoaded' -and $safeSettings -match 'Required GUILD_POINTS setting is missing or invalid') 'GUILD_POINTS is not required during ShardManager startup.'
Assert-True ($safeSettings -match 'TryParseBoolean\(setting\.value, enabled\)') 'GUILD_POINTS values are not strictly validated.'

Assert-True ($settingsTemplate -match 'Password=CHANGE_ME') 'The distributed settings template does not require explicit credentials.'
$passwordLines = @($settingsTemplate -split "`r?`n" | Where-Object { $_ -match '^(?i)Password=' })
Assert-True ($passwordLines.Count -eq 1 -and $passwordLines[0] -eq 'Password=CHANGE_ME') 'A non-placeholder password remains in the settings template.'

$deliveryDll = 'D:\KMTGuard-build\ServerAddons\ShardManager\KMTGuard_ShardManager.dll'
$localBuildDll = Join-Path $projectRoot 'Outpus\KMTGuard_ShardManager.dll'
$dll = if (Test-Path -LiteralPath $localBuildDll -PathType Leaf) {
    $localBuildDll
} else {
    $deliveryDll
}
Assert-True (Test-Path -LiteralPath $dll -PathType Leaf) 'ShardManager DLL was not built.'
$version = (Get-Item -LiteralPath $dll).VersionInfo.FileVersion
$productVersion = (Get-Content -LiteralPath (Join-Path $repositoryRoot 'VERSION.txt') -Raw).Trim()
$expectedFileVersion = "$productVersion.0"
Assert-True ($version -eq $expectedFileVersion) "Unexpected ShardManager version: $version (expected $expectedFileVersion)"

Write-Output 'ShardManager source safety checks: PASS'
