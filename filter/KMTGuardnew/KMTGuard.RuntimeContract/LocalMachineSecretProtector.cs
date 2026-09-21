using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

namespace KMTGuard.RuntimeContract;

public static class LocalMachineSecretProtector
{
    private const int CryptProtectLocalMachine = 0x4;
    private static readonly byte[] Entropy = Encoding.UTF8.GetBytes("KMTGuard.Telegram.v1");

    public static string Protect(string plaintext)
    {
        if (string.IsNullOrEmpty(plaintext))
            return string.Empty;

        EnsureWindows();
        var plaintextBytes = Encoding.UTF8.GetBytes(plaintext);
        return Convert.ToBase64String(ProtectBytes(plaintextBytes));
    }

    public static string Unprotect(string protectedValue)
    {
        if (string.IsNullOrEmpty(protectedValue))
            return string.Empty;

        EnsureWindows();
        try
        {
            return Encoding.UTF8.GetString(UnprotectBytes(Convert.FromBase64String(protectedValue)));
        }
        catch (FormatException ex)
        {
            throw new InvalidDataException("The protected Telegram token is not valid Base64 data.", ex);
        }
    }

    private static byte[] ProtectBytes(byte[] input)
    {
        using var inputBlob = DataBlobHandle.FromBytes(input);
        using var entropyBlob = DataBlobHandle.FromBytes(Entropy);
        if (!CryptProtectData(
                ref inputBlob.Blob,
                null,
                ref entropyBlob.Blob,
                IntPtr.Zero,
                IntPtr.Zero,
                CryptProtectLocalMachine,
                out var outputBlob))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Windows could not protect the Telegram token.");
        }

        return CopyAndFree(outputBlob);
    }

    private static byte[] UnprotectBytes(byte[] input)
    {
        using var inputBlob = DataBlobHandle.FromBytes(input);
        using var entropyBlob = DataBlobHandle.FromBytes(Entropy);
        if (!CryptUnprotectData(
                ref inputBlob.Blob,
                IntPtr.Zero,
                ref entropyBlob.Blob,
                IntPtr.Zero,
                IntPtr.Zero,
                0,
                out var outputBlob))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Windows could not decrypt the Telegram token on this server.");
        }

        return CopyAndFree(outputBlob);
    }

    private static byte[] CopyAndFree(DataBlob blob)
    {
        try
        {
            if (blob.Size <= 0 || blob.Data == IntPtr.Zero)
                return Array.Empty<byte>();

            var bytes = new byte[blob.Size];
            Marshal.Copy(blob.Data, bytes, 0, bytes.Length);
            return bytes;
        }
        finally
        {
            if (blob.Data != IntPtr.Zero)
                LocalFree(blob.Data);
        }
    }

    private static void EnsureWindows()
    {
        if (!OperatingSystem.IsWindows())
            throw new PlatformNotSupportedException("Telegram token protection requires Windows.");
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct DataBlob
    {
        public int Size;
        public IntPtr Data;
    }

    private sealed class DataBlobHandle : IDisposable
    {
        private DataBlobHandle(DataBlob blob)
        {
            Blob = blob;
        }

        public DataBlob Blob;

        public static DataBlobHandle FromBytes(byte[] bytes)
        {
            var data = Marshal.AllocHGlobal(Math.Max(bytes.Length, 1));
            if (bytes.Length > 0)
                Marshal.Copy(bytes, 0, data, bytes.Length);

            return new DataBlobHandle(new DataBlob { Size = bytes.Length, Data = data });
        }

        public void Dispose()
        {
            if (Blob.Data == IntPtr.Zero)
                return;

            Marshal.FreeHGlobal(Blob.Data);
            Blob.Data = IntPtr.Zero;
            Blob.Size = 0;
        }
    }

    [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CryptProtectData(
        ref DataBlob dataIn,
        string? description,
        ref DataBlob optionalEntropy,
        IntPtr reserved,
        IntPtr promptStruct,
        int flags,
        out DataBlob dataOut);

    [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CryptUnprotectData(
        ref DataBlob dataIn,
        IntPtr description,
        ref DataBlob optionalEntropy,
        IntPtr reserved,
        IntPtr promptStruct,
        int flags,
        out DataBlob dataOut);

    [DllImport("kernel32.dll")]
    private static extern IntPtr LocalFree(IntPtr memory);
}
