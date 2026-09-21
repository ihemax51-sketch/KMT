[CmdletBinding()]
param(
    [Parameter()]
    [ValidateNotNullOrEmpty()]
    [string]$Target = 'KMTGuard/Sql',

    [Parameter(Mandatory)]
    [ValidateNotNullOrEmpty()]
    [string]$Username,

    [Parameter()]
    [Security.SecureString]$Password
)

$ErrorActionPreference = 'Stop'
if (-not $IsWindows -and $PSVersionTable.PSVersion.Major -ge 6) {
    throw 'Windows Credential Manager is only available on Windows.'
}
if ($null -eq $Password) {
    $Password = Read-Host 'SQL password' -AsSecureString
}

if (-not ('KmtCredentialWriter' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

public static class KmtCredentialWriter
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct Credential
    {
        public UInt32 Flags;
        public UInt32 Type;
        public string TargetName;
        public string Comment;
        public System.Runtime.InteropServices.ComTypes.FILETIME LastWritten;
        public UInt32 CredentialBlobSize;
        public IntPtr CredentialBlob;
        public UInt32 Persist;
        public UInt32 AttributeCount;
        public IntPtr Attributes;
        public string TargetAlias;
        public string UserName;
    }

    [DllImport("advapi32.dll", EntryPoint = "CredWriteW", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CredWrite(ref Credential credential, UInt32 flags);

    public static void Write(string target, string username, string password)
    {
        byte[] blob = Encoding.Unicode.GetBytes(password);
        IntPtr pointer = Marshal.AllocHGlobal(blob.Length);
        try
        {
            Marshal.Copy(blob, 0, pointer, blob.Length);
            Credential credential = new Credential
            {
                Type = 1,
                TargetName = target,
                UserName = username,
                CredentialBlob = pointer,
                CredentialBlobSize = (UInt32)blob.Length,
                Persist = 2
            };
            if (!CredWrite(ref credential, 0))
                throw new Win32Exception(Marshal.GetLastWin32Error());
        }
        finally
        {
            Array.Clear(blob, 0, blob.Length);
            Marshal.Copy(blob, 0, pointer, blob.Length);
            Marshal.FreeHGlobal(pointer);
        }
    }
}
'@
}

$bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($Password)
try {
    $plainText = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    [KmtCredentialWriter]::Write($Target, $Username, $plainText)
    Write-Host "Stored Windows credential '$Target' for '$Username'."
}
finally {
    $plainText = $null
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
}
