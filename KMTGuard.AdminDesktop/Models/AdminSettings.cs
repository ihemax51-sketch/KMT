using Microsoft.Data.SqlClient;

namespace KMTGuard.AdminDesktop.Models;

public sealed class AdminSettings
{
    public string Address { get; set; } = string.Empty;
    public int? Port { get; set; }
    public string Username { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
    public string CredentialTarget { get; set; } = "KMTGuard/Sql";
    public string QuickLoginCredentialTarget { get; set; } = "KMTGuard/QuickLoginMasterKey";
    public string QuickLoginMasterKey { get; set; } = string.Empty;
    public string ProxyDb { get; set; } = string.Empty;
    public int MaximumPool { get; set; } = 1500;
    public int MinimumPool { get; set; } = 50;
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

    public string BuildConnectionString()
    {
        var builder = new SqlConnectionStringBuilder
        {
            DataSource = Port is > 0 ? $"{Address},{Port.Value}" : Address,
            InitialCatalog = ProxyDb,
            UserID = Username,
            Password = Password,
            PersistSecurityInfo = true,
            MultipleActiveResultSets = true,
            Encrypt = false,
            Pooling = true,
            MaxPoolSize = MaximumPool <= 0 ? 1500 : MaximumPool,
            MinPoolSize = Math.Max(0, MinimumPool),
            ConnectTimeout = 5,
            ApplicationName = "KMTGuard Admin Desktop"
        };

        if (ConnectionLifetime > 0)
            builder.LoadBalanceTimeout = ConnectionLifetime;

        return builder.ConnectionString;
    }

    public AdminSettings Clone() => new()
    {
        Address = Address,
        Port = Port,
        Username = Username,
        Password = Password,
        CredentialTarget = CredentialTarget,
        QuickLoginCredentialTarget = QuickLoginCredentialTarget,
        QuickLoginMasterKey = QuickLoginMasterKey,
        ProxyDb = ProxyDb,
        MaximumPool = MaximumPool,
        MinimumPool = MinimumPool,
        ConnectionLifetime = ConnectionLifetime,
        ServerIP = ServerIP,
        Language = Language,
        UpstreamConnectTimeoutSeconds = UpstreamConnectTimeoutSeconds,
        HandshakeTimeoutSeconds = HandshakeTimeoutSeconds,
        LoginTimeoutSeconds = LoginTimeoutSeconds,
        UnauthenticatedIdleTimeoutSeconds = UnauthenticatedIdleTimeoutSeconds,
        AuthenticatedHeartbeatTimeoutSeconds = AuthenticatedHeartbeatTimeoutSeconds,
        GatewaySessionCap = GatewaySessionCap,
        AgentSessionCap = AgentSessionCap,
        DownloadSessionCap = DownloadSessionCap
    };
}
