using System.Security.Cryptography;

namespace KMTGuard.SessionManager;

public sealed class GatewayCredential : IDisposable
{
    private readonly object _sync = new();
    private char[] _password = Array.Empty<char>();

    public bool HasValue
    {
        get
        {
            lock (_sync)
                return _password.Length != 0;
        }
    }

    public void Replace(string password)
    {
        ArgumentNullException.ThrowIfNull(password);
        var replacement = password.ToCharArray();
        lock (_sync)
        {
            ClearCore();
            _password = replacement;
        }
    }

    public string Reveal()
    {
        lock (_sync)
        {
            if (_password.Length == 0)
                throw new InvalidOperationException("Gateway credential is no longer available.");
            return new string(_password);
        }
    }

    public void Clear()
    {
        lock (_sync)
            ClearCore();
    }

    public void Dispose() => Clear();

    private void ClearCore()
    {
        if (_password.Length != 0)
            CryptographicOperations.ZeroMemory(System.Runtime.InteropServices.MemoryMarshal.AsBytes(_password.AsSpan()));
        _password = Array.Empty<char>();
    }
}
