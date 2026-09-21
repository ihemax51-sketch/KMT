$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$gameServerRoot = Join-Path $repositoryRoot 'gameserver\source\SilkroadOnline\SR_VIETNAM_SERVICE\v188\Server\SR_GameServer'
$logCustoms = Get-Content -LiteralPath (Join-Path $gameServerRoot 'GameServer\GameServer\src\GSLog\LogCustoms.cpp') -Raw
$serverFramework = Get-Content -LiteralPath (Join-Path $repositoryRoot 'gameserver\source\JMX_ServerFramework\src\ServerFramework\ServerFramework.h') -Raw
$initialization = Get-Content -LiteralPath (Join-Path $gameServerRoot 'OutPut\src\Util.cpp') -Raw

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

Assert-True ($logCustoms -match 'SR_GameServer is initialized successfully') 'Native GameServer readiness trigger is missing.'
Assert-True ($logCustoms -match 'GameServer startup completed successfully') 'Native GameServer readiness stamp is missing.'
Assert-True ($logCustoms -match 'InterlockedCompareExchange\(&s_readyStampWritten, 1, 0\)') 'Native GameServer readiness stamp is not one-shot.'
Assert-True ($initialization -match 'CLogCustoms::Setup\(KMTGUARD_VERSION_STRING\)') 'The native GameServer stamp is not bound to the product version.'
Assert-True ($serverFramework -match 'if \(pLogMsg == NULL\)') 'Native log allocation is not guarded.'
Assert-True ($serverFramework -match 'TryWriteString') 'Native log packet overflow protection is missing.'

$originalLogIndex = $logCustoms.IndexOf('void* result = s_pfnServerApp_OnLogMsg')
$stampIndex = $logCustoms.IndexOf('WriteNativeReadyStamp();')
Assert-True ($originalLogIndex -ge 0 -and $stampIndex -gt $originalLogIndex) 'KMTGuard readiness is emitted before the original startup completion message.'

Write-Output 'GameServer native readiness stamp checks: PASS'
