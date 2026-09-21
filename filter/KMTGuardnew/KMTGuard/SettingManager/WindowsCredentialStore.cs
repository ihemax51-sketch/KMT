using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

namespace KMTGuard.SettingManager;

internal static class WindowsCredentialStore
{
    private const uint CredTypeGeneric = 1;

    public static string ReadPassword(string target, string expectedUsername)
    {
        if (!OperatingSystem.IsWindows())
            throw new PlatformNotSupportedException("KMTGuard SQL credentials require Windows Credential Manager.");
        if (string.IsNullOrWhiteSpace(target))
            throw new InvalidDataException("CredentialTarget is required.");

        if (!CredRead(target, CredTypeGeneric, 0, out var credentialPointer))
            throw new Win32Exception(Marshal.GetLastWin32Error(),
                $"Windows credential '{target}' was not found.");

        try
        {
            var credential = Marshal.PtrToStructure<Credential>(credentialPointer);
            var storedUsername = Marshal.PtrToStringUni(credential.UserName) ?? string.Empty;
            if (!string.IsNullOrWhiteSpace(storedUsername) &&
                !string.Equals(storedUsername, expectedUsername, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException(
                    $"CredentialTarget '{target}' belongs to a different SQL username.");

            if (credential.CredentialBlob == IntPtr.Zero || credential.CredentialBlobSize == 0)
                throw new InvalidDataException($"CredentialTarget '{target}' has an empty password.");

            var bytes = new byte[credential.CredentialBlobSize];
            Marshal.Copy(credential.CredentialBlob, bytes, 0, bytes.Length);
            var password = Encoding.Unicode.GetString(bytes).TrimEnd('\0');
            CryptographicOperations.ZeroMemory(bytes);
            if (string.IsNullOrEmpty(password))
                throw new InvalidDataException($"CredentialTarget '{target}' has an empty password.");
            return password;
        }
        finally
        {
            CredFree(credentialPointer);
        }
    }

    public static byte[] ReadBase64Secret(string target, string expectedUsername, int requiredBytes)
    {
        var encoded = ReadPassword(target, expectedUsername);
        byte[] secret;
        try
        {
            secret = Convert.FromBase64String(encoded);
        }
        catch (FormatException exception)
        {
            throw new InvalidDataException(
                $"CredentialTarget '{target}' does not contain a Base64 secret.", exception);
        }

        if (secret.Length != requiredBytes)
        {
            CryptographicOperations.ZeroMemory(secret);
            throw new InvalidDataException(
                $"CredentialTarget '{target}' must contain exactly {requiredBytes} random bytes.");
        }

        return secret;
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
