namespace KMTGuard.AdminDesktop.Models;

public sealed class TelegramNotificationSettings
{
    public bool Enabled { get; set; }
    public string BotToken { get; set; } = string.Empty;
    public bool HasStoredBotToken { get; set; }
    public string ChannelId { get; set; } = string.Empty;
    public bool UniqueSpawnEnabled { get; set; } = true;
    public bool UniqueKillEnabled { get; set; } = true;
    public bool ShowKillerName { get; set; } = true;
    public bool EventReminderEnabled { get; set; } = true;
    public string ReminderMinutes { get; set; } = "15,5";
    public bool EventStartedEnabled { get; set; } = true;
    public bool EventFinishedEnabled { get; set; } = true;
    public bool ServerOnlineEnabled { get; set; }
    public bool FortressWarEnabled { get; set; } = true;
    public DateTime UpdatedAtUtc { get; set; }
}
