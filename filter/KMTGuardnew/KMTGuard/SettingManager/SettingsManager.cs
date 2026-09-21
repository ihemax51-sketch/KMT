#region

using System.IO;
using System.ComponentModel;
using System.Security.Cryptography;
using System.Text;
using KMTGuard.SettingManager;
using Newtonsoft.Json;
using Serilog;
using Formatting = Newtonsoft.Json.Formatting;

#endregion

namespace KMTGuard.SettingManager;

public class SettingsManager : ISettingsManager
{
    public static string SettingsPath =>
        Environment.GetEnvironmentVariable("KMTGUARD_SETTINGS_PATH")
        ?? Path.Combine(AppContext.BaseDirectory, "Settings.json");

    public SettingsManager()
    {
        if (!File.Exists(SettingsPath))
        {
            Settings = new Settings().Init();
            Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath) ?? AppContext.BaseDirectory);
            File.WriteAllText(SettingsPath,
                JsonConvert.SerializeObject(Settings, Formatting.Indented));
            throw new InvalidOperationException(
                $"Settings template created at '{SettingsPath}'. Configure the SQL username and password before starting KMTGuard.");
        }

        var json = File.ReadAllText(SettingsPath);
        Settings = JsonConvert.DeserializeObject<Settings>(json)
            ?? throw new InvalidDataException("Settings.json is empty or invalid.");
    }

    public ISettings Settings { get; private set; }

    public byte[] GetOrCreateQuickLoginMasterKey()
    {
        if (TryDecodeQuickLoginKey(Settings.QuickLoginMasterKey, out var configuredKey))
            return configuredKey;

        var mutexHash = Convert.ToHexString(SHA256.HashData(
            Encoding.UTF8.GetBytes(Path.GetFullPath(SettingsPath))))[..24];
        using var settingsMutex = new Mutex(false, $"Local\\KMTGuard-Settings-{mutexHash}");
        if (!settingsMutex.WaitOne(TimeSpan.FromSeconds(10)))
            throw new IOException("Timed out while preparing the Quick Login key in Settings.json.");

        try
        {
            // Another Gateway may have populated the file while this process
            // waited for the settings lock, so always reload it first.
            var current = JsonConvert.DeserializeObject<Settings>(File.ReadAllText(SettingsPath))
                ?? throw new InvalidDataException("Settings.json is empty or invalid.");
            if (TryDecodeQuickLoginKey(current.QuickLoginMasterKey, out configuredKey))
            {
                Settings = current;
                return configuredKey;
            }

            byte[] key;
            try
            {
                // Import an existing credential once, then keep normal customer
                // setup self-contained in Settings.json.
                key = WindowsCredentialStore.ReadBase64Secret(
                    current.QuickLoginCredentialTarget, "KMTGuard", 32);
            }
            catch (Win32Exception)
            {
                key = RandomNumberGenerator.GetBytes(32);
            }

            current.QuickLoginMasterKey = Convert.ToBase64String(key);
            PersistSettings(current);
            Settings = current;
            return key;
        }
        finally
        {
            settingsMutex.ReleaseMutex();
        }
    }

    private static bool TryDecodeQuickLoginKey(string? encoded, out byte[] key)
    {
        key = Array.Empty<byte>();
        if (string.IsNullOrWhiteSpace(encoded))
            return false;

        try
        {
            key = Convert.FromBase64String(encoded);
        }
        catch (FormatException exception)
        {
            throw new InvalidDataException(
                "QuickLoginMasterKey in Settings.json must be valid Base64.", exception);
        }

        if (key.Length == 32)
            return true;

        CryptographicOperations.ZeroMemory(key);
        key = Array.Empty<byte>();
        throw new InvalidDataException(
            "QuickLoginMasterKey in Settings.json must contain exactly 32 random bytes.");
    }

    private static void PersistSettings(Settings settings)
    {
        var directory = Path.GetDirectoryName(SettingsPath) ?? AppContext.BaseDirectory;
        Directory.CreateDirectory(directory);
        var temporaryPath = Path.Combine(
            directory,
            $".{Path.GetFileName(SettingsPath)}.{Environment.ProcessId}.tmp");
        File.WriteAllText(temporaryPath, JsonConvert.SerializeObject(settings, Formatting.Indented));
        File.Move(temporaryPath, SettingsPath, true);
    }

    public void Dispose()
    {
        Settings.Dispose();
    }
}
