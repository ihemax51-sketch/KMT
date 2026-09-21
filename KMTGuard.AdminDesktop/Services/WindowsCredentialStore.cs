using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

namespace KMTGuard.AdminDesktop.Services;

internal static class WindowsCredentialStore
{
    private const uint CredTypeGeneric = 1;

    public static bool TryReadPassword(string target, string expectedUsername, out string password)
    {
        password = string.Empty;
        if (!OperatingSystem.IsWindows() || string.IsNullOrWhiteSpace(target))
            return false;

        if (!CredRead(target, CredTypeGeneric, 0, out var credentialPointer))
            return false;

        try
        {
            var credential = Marshal.PtrToStructure<Credential>(credentialPointer);
            var storedUsername = Marshal.PtrToStringUni(credential.UserName) ?? string.Empty;
            if (!string.IsNullOrWhiteSpace(storedUsername) &&
                !string.Equals(storedUsername, expectedUsername, StringComparison.OrdinalIgnoreCase))
                return false;

            if (credential.CredentialBlob == IntPtr.Zero || credential.CredentialBlobSize == 0)
                return false;

            var bytes = new byte[credential.CredentialBlobSize];
            try
            {
                Marshal.Copy(credential.CredentialBlob, bytes, 0, bytes.Length);
                password = Encoding.Unicode.GetString(bytes).TrimEnd('\0');
                return password.Length > 0;
            }
            finally
            {
                CryptographicOperations.ZeroMemory(bytes);
            }
        }
        finally
        {
            CredFree(credentialPointer);
        }
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct Credential
    {
        public uint Flags;
        public uint Type;
        public IntPtr TargetName;
        public IntPtr Comment;
        public System.Runtime.InteropServices.ComTypes.FILETIME LastWritten;
        public uint CredentialBlobSize;
        public IntPtr CredentialBlob;
        public uint Persist;
        public uint AttributeCount;
        public IntPtr Attributes;
        public IntPtr TargetAlias;
        public IntPtr UserName;
    }

    [DllImport("advapi32.dll", EntryPoint = "CredReadW", CharSet = CharSet.Unicode,
        SetLastError = true)]
    private static extern bool CredRead(
        string target, uint type, uint reservedFlag, out IntPtr credentialPointer);

    [DllImport("advapi32.dll")]
    private static extern void CredFree(IntPtr buffer);
}
