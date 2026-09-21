[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateScript({ Test-Path -LiteralPath $_ -PathType Leaf })]
    [string]$SettingsPath,

    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string]$ServiceIdentity = ([Security.Principal.WindowsIdentity]::GetCurrent().Name)
)

$ErrorActionPreference = 'Stop'
$resolved = (Resolve-Path -LiteralPath $SettingsPath).Path
$acl = [Security.AccessControl.FileSecurity]::new()
$acl.SetAccessRuleProtection($true, $false)
foreach ($identity in @('NT AUTHORITY\SYSTEM', 'BUILTIN\Administrators', $ServiceIdentity) | Select-Object -Unique) {
    $rule = [Security.AccessControl.FileSystemAccessRule]::new(
        $identity,
        [Security.AccessControl.FileSystemRights]::FullControl,
        [Security.AccessControl.AccessControlType]::Allow)
    [void]$acl.AddAccessRule($rule)
}
Set-Acl -LiteralPath $resolved -AclObject $acl
Write-Host "Restricted Settings.json ACL to SYSTEM, Administrators, and '$ServiceIdentity'."
