using System.IO;
using KMTGuard.AdminDesktop.Models;
using Newtonsoft.Json;

namespace KMTGuard.AdminDesktop.Services;

public sealed class SetupStateService
{
    private const int CurrentSetupVersion = 1;

    private static string StatePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "KMTGuard",
        "AdminDesktop",
        "setup-state.json");

    public bool IsComplete(string settingsPath)
    {
        if (string.IsNullOrWhiteSpace(settingsPath) || !File.Exists(settingsPath) || !File.Exists(StatePath))
            return false;

        try
        {
            var state = JsonConvert.DeserializeObject<SetupState>(File.ReadAllText(StatePath));
            var settings = JsonConvert.DeserializeObject<AdminSettings>(File.ReadAllText(settingsPath));
            return state is { Completed: true, Version: >= CurrentSetupVersion } &&
                   settings is not null &&
                   !string.IsNullOrWhiteSpace(settings.Address) &&
                   !string.IsNullOrWhiteSpace(settings.ProxyDb) &&
                   !string.IsNullOrWhiteSpace(settings.ServerIP);
        }
        catch
        {
            return false;
        }
    }

    public void MarkComplete()
    {
        var directory = Path.GetDirectoryName(StatePath)
            ?? throw new InvalidOperationException("The setup-state folder could not be resolved.");
        Directory.CreateDirectory(directory);

        var state = new SetupState
        {
            Version = CurrentSetupVersion,
            Completed = true,
            CompletedAtUtc = DateTimeOffset.UtcNow
        };
        File.WriteAllText(StatePath, JsonConvert.SerializeObject(state, Formatting.Indented));
    }

    private sealed class SetupState
    {
        public int Version { get; set; }
        public bool Completed { get; set; }
        public DateTimeOffset CompletedAtUtc { get; set; }
    }
}
