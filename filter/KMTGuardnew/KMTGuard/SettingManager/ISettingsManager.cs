

namespace KMTGuard.SettingManager;

public interface ISettingsManager : IDisposable
{
    ISettings Settings { get; }
}