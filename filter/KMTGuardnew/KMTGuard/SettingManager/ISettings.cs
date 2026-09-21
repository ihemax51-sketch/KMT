namespace KMTGuard.SettingManager;

public interface ISettings : IDisposable
{
    string Address { get; set; }
    int? Port { get; set; }
    string Username { get; set; }
    string Password { get; set; }
    string CredentialTarget { get; set; }
    string QuickLoginCredentialTarget { get; set; }
    string QuickLoginMasterKey { get; set; }
    string ProxyDb { get; set; }
    int MaximumPool { get; set; }
    int MinimumPool { get; set; }
    int ConnectionLifetime { get; set; }
    string ServerIP { get; set; }
    string Language { get; set; }
    int UpstreamConnectTimeoutSeconds { get; set; }
    int HandshakeTimeoutSeconds { get; set; }
    int LoginTimeoutSeconds { get; set; }
    int UnauthenticatedIdleTimeoutSeconds { get; set; }
    int AuthenticatedHeartbeatTimeoutSeconds { get; set; }
    int GatewaySessionCap { get; set; }
    int AgentSessionCap { get; set; }
    int DownloadSessionCap { get; set; }
    ISettings Init();
}
