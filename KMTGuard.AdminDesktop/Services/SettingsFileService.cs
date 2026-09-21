using System.IO;
using KMTGuard.AdminDesktop.Models;
using Newtonsoft.Json;

namespace KMTGuard.AdminDesktop.Services;

public sealed class SettingsFileService
{
    public string GetRuntimeSettingsPath()
    {
        return Path.Combine(AppContext.BaseDirectory, "Settings.json");
    }

    public string? FindDefaultSettingsPath()
    {
        var runtimePath = GetRuntimeSettingsPath();
        if (File.Exists(runtimePath))
            return runtimePath;

        var baseDir = AppContext.BaseDirectory;
        var candidates = new[]
        {
            Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\..\KMTGuard\bin\Release\net8.0\win-x64\Settings.json")),
            Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\..\KMTGuard\bin\Debug\net8.0\win-x64\Settings.json")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, @"KMTGuard\bin\Release\net8.0\win-x64\Settings.json")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, @"KMTGuard\bin\Debug\net8.0\win-x64\Settings.json")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, @"filter\KMTGuardnew\KMTGuard\bin\Release\net8.0\win-x64\Settings.json")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, @"filter\KMTGuardnew\KMTGuard\bin\Debug\net8.0\win-x64\Settings.json"))
        };

        var existingPath = candidates.FirstOrDefault(File.Exists);
        if (existingPath is null)
            return runtimePath;

        Directory.CreateDirectory(Path.GetDirectoryName(runtimePath) ?? AppContext.BaseDirectory);
        File.Copy(existingPath, runtimePath, overwrite: false);
        return runtimePath;
    }

    public AdminSettings Load(string path)
    {
        if (!File.Exists(path))
            throw new FileNotFoundException("Settings.json was not found.", path);

        var settings = JsonConvert.DeserializeObject<AdminSettings>(File.ReadAllText(path))
            ?? throw new InvalidDataException("Settings.json is empty or invalid.");

        if (string.IsNullOrEmpty(settings.Password) &&
            WindowsCredentialStore.TryReadPassword(
                settings.CredentialTarget, settings.Username, out var credentialPassword))
            settings.Password = credentialPassword;

        return settings;
    }

    public void Save(string path, AdminSettings settings)
    {
        var json = JsonConvert.SerializeObject(settings, Formatting.Indented);
        Directory.CreateDirectory(Path.GetDirectoryName(path) ?? AppContext.BaseDirectory);
        File.WriteAllText(path, json);
    }
}
