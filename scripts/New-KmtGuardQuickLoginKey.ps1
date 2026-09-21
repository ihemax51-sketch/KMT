[CmdletBinding()]
param(
    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string]$Target = 'KMTGuard/QuickLoginMasterKey'
)

$ErrorActionPreference = 'Stop'
$key = [byte[]]::new(32)
[Security.Cryptography.RandomNumberGenerator]::Fill($key)
try {
    $encoded = [Convert]::ToBase64String($key)
    $secure = ConvertTo-SecureString $encoded -AsPlainText -Force
    & (Join-Path $PSScriptRoot 'Set-KmtGuardSqlCredential.ps1') `
        -Target $Target -Username 'KMTGuard' -Password $secure
}
finally {
    [Security.Cryptography.CryptographicOperations]::ZeroMemory($key)
    $encoded = $null
    $secure = $null
}
