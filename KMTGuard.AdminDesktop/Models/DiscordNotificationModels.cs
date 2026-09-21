using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace KMTGuard.AdminDesktop.Models;

public sealed class DiscordNotificationSettings
{
    public bool Enabled { get; set; }
    public string BotToken { get; set; } = string.Empty;
    public bool HasStoredBotToken { get; set; }
    public DateTime UpdatedAtUtc { get; set; }
}

public sealed class DiscordNotificationChannel : INotifyPropertyChanged
{
    private int _channelRecordId;
    private string _channelName = string.Empty;
    private string _originalChannelName = string.Empty;
    private string _discordChannelId = string.Empty;
    private bool _enabled = true;
    private int _sortOrder;
    private string _statusMessage = "Ready.";

    public int ChannelRecordID
    {
        get => _channelRecordId;
        set => SetField(ref _channelRecordId, value);
    }

    public string ChannelName
    {
        get => _channelName;
        set => SetField(ref _channelName, value);
    }

    public string OriginalChannelName
    {
        get => _originalChannelName;
        set => SetField(ref _originalChannelName, value);
    }

    public string DiscordChannelID
    {
        get => _discordChannelId;
        set => SetField(ref _discordChannelId, value);
    }

    public bool Enabled
    {
        get => _enabled;
        set => SetField(ref _enabled, value);
    }

    public int SortOrder
    {
        get => _sortOrder;
        set => SetField(ref _sortOrder, value);
    }

    public string StatusMessage
    {
        get => _statusMessage;
        set => SetField(ref _statusMessage, value);
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

public sealed class DiscordDeliveryActivity
{
    public long NotificationID { get; set; }
    public string ChannelName { get; set; } = string.Empty;
    public string MessagePreview { get; set; } = string.Empty;
    public string DeliveryStatus { get; set; } = string.Empty;
    public int Attempts { get; set; }
    public DateTime CreatedAtUtc { get; set; }
    public DateTime? SentAtUtc { get; set; }
    public string LastError { get; set; } = string.Empty;
}
