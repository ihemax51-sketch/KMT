namespace KMTGuard.SettingManager;

public class Settings : ISettings
{
    public string Address { get; set; } = string.Empty;
    public int? Port { get; set; }
    public string Username { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
    public string CredentialTarget { get; set; } = "KMTGuard/Sql";
    public string QuickLoginCredentialTarget { get; set; } = "KMTGuard/QuickLoginMasterKey";
    public string QuickLoginMasterKey { get; set; } = string.Empty;
    public string ProxyDb { get; set; } = string.Empty;
    public int MaximumPool { get; set; }
    public int MinimumPool { get; set; }
    public int ConnectionLifetime { get; set; }
    public string ServerIP { get; set; } = string.Empty;
    public string Language { get; set; } = "English";
    public int UpstreamConnectTimeoutSeconds { get; set; } = 10;
    public int HandshakeTimeoutSeconds { get; set; } = 15;
    public int LoginTimeoutSeconds { get; set; } = 60;
    public int UnauthenticatedIdleTimeoutSeconds { get; set; } = 90;
    public int AuthenticatedHeartbeatTimeoutSeconds { get; set; } = 180;
    public int GatewaySessionCap { get; set; } = 10000;
    public int AgentSessionCap { get; set; } = 10000;
    public int DownloadSessionCap { get; set; } = 2000;
    public ISettings Init()
    {
        Address = "127.0.0.1";
        Port = 1433;
        Username = "sa";
        Password = string.Empty;
        CredentialTarget = "KMTGuard/Sql";
        QuickLoginCredentialTarget = "KMTGuard/QuickLoginMasterKey";
        QuickLoginMasterKey = string.Empty;
        ProxyDb = "KMTGuard";
        MaximumPool = 256;
        MinimumPool = 10;
        ConnectionLifetime = 0;
        ServerIP = "192.168.1.1";
        Language = "English";
        UpstreamConnectTimeoutSeconds = 10;
        HandshakeTimeoutSeconds = 15;
        LoginTimeoutSeconds = 60;
        UnauthenticatedIdleTimeoutSeconds = 90;
        AuthenticatedHeartbeatTimeoutSeconds = 180;
        GatewaySessionCap = 10000;
        AgentSessionCap = 10000;
        DownloadSessionCap = 2000;
        return this;
    }

    public void Dispose()
    {
        Address = string.Empty;
        Port = null;
        Username = string.Empty;
        Password = string.Empty;
        CredentialTarget = string.Empty;
        QuickLoginCredentialTarget = string.Empty;
        QuickLoginMasterKey = string.Empty;
        ProxyDb = string.Empty;
        Language = string.Empty;
    }
}
