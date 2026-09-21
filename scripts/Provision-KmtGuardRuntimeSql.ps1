[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string]$Server,

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string]$FilterDatabase,

    [Parameter()]
    [string[]]$DataDatabases = @(),

    [Parameter()]
    [string]$AdminUsername,

    [Parameter()]
    [Security.SecureString]$AdminPassword,

    [Parameter()]
    [Security.SecureString]$RuntimePassword,

    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string]$CredentialTarget = 'KMTGuard/Sql'
)

$ErrorActionPreference = 'Stop'
if ($null -eq $RuntimePassword) {
    $RuntimePassword = Read-Host 'New KMTGuardRuntime SQL password' -AsSecureString
}
if ($AdminUsername -and $null -eq $AdminPassword) {
    $AdminPassword = Read-Host "SQL administrator password for $AdminUsername" -AsSecureString
}

$runtimeBstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($RuntimePassword)
$adminBstr = [IntPtr]::Zero
$temporarySql = $null
try {
    $runtimePlain = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($runtimeBstr)
    $escapedPassword = $runtimePlain.Replace("'", "''")
    $escapedDatabase = $FilterDatabase.Replace(']', ']]')
    $sql = @"
SET XACT_ABORT ON;
IF SUSER_ID(N'KMTGuardRuntime') IS NULL
    CREATE LOGIN [KMTGuardRuntime] WITH PASSWORD=N'$escapedPassword', CHECK_POLICY=ON, CHECK_EXPIRATION=OFF;
ELSE
    ALTER LOGIN [KMTGuardRuntime] WITH PASSWORD=N'$escapedPassword', CHECK_POLICY=ON, CHECK_EXPIRATION=OFF;
IF IS_SRVROLEMEMBER(N'sysadmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [sysadmin] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'securityadmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [securityadmin] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'serveradmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [serveradmin] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'setupadmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [setupadmin] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'processadmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [processadmin] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'diskadmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [diskadmin] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'dbcreator',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [dbcreator] DROP MEMBER [KMTGuardRuntime];
IF IS_SRVROLEMEMBER(N'bulkadmin',N'KMTGuardRuntime')=1 ALTER SERVER ROLE [bulkadmin] DROP MEMBER [KMTGuardRuntime];
USE [$escapedDatabase];
IF USER_ID(N'KMTGuardRuntime') IS NULL
    CREATE USER [KMTGuardRuntime] FOR LOGIN [KMTGuardRuntime];
IF IS_ROLEMEMBER(N'KMTGuardRuntimeRole',N'KMTGuardRuntime') <> 1
    ALTER ROLE [KMTGuardRuntimeRole] ADD MEMBER [KMTGuardRuntime];
"@

    foreach ($database in $DataDatabases) {
        if ($database -notmatch '^[A-Za-z0-9_]+$') {
            throw "Unsafe SQL database name: $database"
        }
        $escapedDataDatabase = $database.Replace(']', ']]')
        $sql += @"
USE [$escapedDataDatabase];
IF USER_ID(N'KMTGuardRuntime') IS NULL
    CREATE USER [KMTGuardRuntime] FOR LOGIN [KMTGuardRuntime];
IF DATABASE_PRINCIPAL_ID(N'KMTGuardRuntimeDataRole') IS NULL
    CREATE ROLE [KMTGuardRuntimeDataRole] AUTHORIZATION dbo;
GRANT CONNECT TO [KMTGuardRuntimeDataRole];
GRANT SELECT, INSERT, UPDATE, DELETE ON SCHEMA::dbo TO [KMTGuardRuntimeDataRole];
GRANT EXECUTE ON SCHEMA::dbo TO [KMTGuardRuntimeDataRole];
DENY ALTER, CONTROL, TAKE OWNERSHIP TO [KMTGuardRuntimeDataRole];
IF IS_ROLEMEMBER(N'KMTGuardRuntimeDataRole',N'KMTGuardRuntime') <> 1
    ALTER ROLE [KMTGuardRuntimeDataRole] ADD MEMBER [KMTGuardRuntime];
"@
    }

    $temporarySql = Join-Path ([IO.Path]::GetTempPath()) ("kmtguard-sql-" + [Guid]::NewGuid().ToString('N') + '.sql')
    [IO.File]::WriteAllText($temporarySql, $sql, [Text.UTF8Encoding]::new($false))

    $arguments = @('-b', '-l', '20', '-S', $Server, '-i', $temporarySql)
    if ($AdminUsername) {
        $adminBstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($AdminPassword)
        $env:SQLCMDPASSWORD = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($adminBstr)
        $arguments += @('-U', $AdminUsername)
    }
    else {
        $arguments += '-E'
    }

    & sqlcmd @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "SQL runtime login provisioning failed with exit code $LASTEXITCODE."
    }

    & (Join-Path $PSScriptRoot 'Set-KmtGuardSqlCredential.ps1') `
        -Target $CredentialTarget -Username 'KMTGuardRuntime' -Password $RuntimePassword
    Write-Host 'KMTGuardRuntime login, database user, role membership, and node credential were provisioned.'
}
finally {
    Remove-Item Env:SQLCMDPASSWORD -ErrorAction SilentlyContinue
    if ($temporarySql -and (Test-Path -LiteralPath $temporarySql)) {
        [IO.File]::WriteAllText($temporarySql, '')
        Remove-Item -LiteralPath $temporarySql -Force
    }
    $runtimePlain = $null
    $escapedPassword = $null
    if ($adminBstr -ne [IntPtr]::Zero) {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($adminBstr)
    }
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($runtimeBstr)
}
