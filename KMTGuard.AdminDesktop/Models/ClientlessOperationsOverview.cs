using System.Data;

namespace KMTGuard.AdminDesktop.Models;

public sealed class ClientlessOperationsOverview
{
    public int TotalAccounts { get; init; }
    public int EnabledAccounts { get; init; }
    public int ReadyAccounts { get; init; }
    public int OnlineAccounts { get; init; }
    public int NeedsAttentionAccounts { get; init; }
    public DataTable Cities { get; init; } = new();
}
