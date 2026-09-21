using System.Data;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace KMTGuard.AdminDesktop.Models;

public sealed class ClientlessHuntPolicy
{
    public bool Enabled { get; init; }
    public bool AttackNormal { get; init; } = true;
    public bool AttackUnique { get; init; } = true;
    public bool UniquePriority { get; init; } = true;
    public bool UseSkills { get; init; } = true;
    public bool UseBasicAttack { get; init; } = true;
    public byte HpPotionPercent { get; init; } = 60;
    public byte MpPotionPercent { get; init; } = 40;
    public short StuckSeconds { get; init; } = 15;
    public short TargetTimeoutSeconds { get; init; } = 30;
}

public sealed class ClientlessHuntingWorkspace
{
    public ClientlessHuntPolicy Policy { get; init; } = new();
    public DataTable Areas { get; init; } = new();
    public DataTable Accounts { get; init; } = new();
}

public sealed class ClientlessHunterAllocationResult
{
    public int RequestedOnlinePerCity { get; init; }
    public int RequestedHuntersPerCity { get; init; }
    public IReadOnlyList<ClientlessHunterCityAllocation> Cities { get; init; } =
        Array.Empty<ClientlessHunterCityAllocation>();
}

public sealed class ClientlessHunterCityAllocation
{
    public string City { get; init; } = string.Empty;
    public int TotalAccounts { get; init; }
    public int ReadyAvailableAccounts { get; init; }
    public int ReadyEnabledAccounts { get; init; }
    public int ActiveHunters { get; init; }
    public int ParkedReadyAccounts { get; init; }
    public int OfflineReadyAccounts { get; init; }
    public int UnavailableAccounts { get; init; }
}

public sealed class ClientlessCityHunterPlan : INotifyPropertyChanged
{
    private bool _isSelected;
    private string _desiredOnlineText = "0";
    private string _desiredHuntersText = "0";

    public string City { get; init; } = string.Empty;
    public int TotalAccounts { get; init; }
    public int ReadyAccounts { get; init; }
    public int CurrentEnabled { get; init; }
    public int CurrentHunters { get; init; }
    public int CurrentParked { get; init; }
    public int OnlineAccounts { get; init; }

    public bool IsSelected
    {
        get => _isSelected;
        set => SetField(ref _isSelected, value);
    }

    public string DesiredOnlineText
    {
        get => _desiredOnlineText;
        set => SetField(ref _desiredOnlineText, value);
    }

    public string DesiredHuntersText
    {
        get => _desiredHuntersText;
        set => SetField(ref _desiredHuntersText, value);
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
            return;

        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
