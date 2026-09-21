using Dapper;
using Microsoft.Data.SqlClient;
using Newtonsoft.Json;
using Serilog;
using SilkroadSecurityAPI;
using KMTGuard.SessionManager;

namespace KMTGuard.Features.WebViewer;

public sealed class WebViewerButtonConfig
{
    public string Name { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public string Url { get; set; } = string.Empty;
    public int FrameWidth { get; set; } = 900;
    public int FrameHeight { get; set; } = 620;
}

public sealed class WebViewerConfigFile
{
    public List<WebViewerButtonConfig> Buttons { get; set; } = new();
}

public static class WebViewerManager
{
    public const ushort ServerOpcode = 0xA4F0;
    private const int MaxButtons = 16;
    private static readonly object SyncRoot = new();
    private static List<WebViewerButtonConfig> _buttons = new();

    public static async Task SendConfigAsync(ISession session)
    {
        var buttons = GetButtons();
        var response = new Packet(ServerOpcode);
        response.WriteUInt16((ushort)buttons.Count);

        foreach (var button in buttons)
        {
            response.WriteAscii(button.Name);
            response.WriteAscii(button.Icon);
            response.WriteAscii(button.Url);
            response.WriteInt32(button.FrameWidth);
            response.WriteInt32(button.FrameHeight);
        }

        await session.SendToClient(response);
    }

    private static List<WebViewerButtonConfig> GetButtons()
    {
        lock (SyncRoot)
        {
            var dbButtons = TryLoadFromDatabase(out var databaseTableExists);
            if (databaseTableExists)
            {
                _buttons = dbButtons;
                return _buttons;
            }

            var path = GetConfigPath();
            EnsureDefaultConfig(path);

            try
            {
                var json = File.ReadAllText(path);
                var config = JsonConvert.DeserializeObject<WebViewerConfigFile>(json) ?? new WebViewerConfigFile();
                _buttons = config.Buttons
                    .Where(IsValid)
                    .Take(MaxButtons)
                    .Select(Normalize)
                    .ToList();
                Log.Verbose("WebViewer config loaded: {Count} button(s) from {Path}", _buttons.Count, path);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to load WebViewer config from {Path}", path);
                _buttons = new List<WebViewerButtonConfig>();
            }

            return _buttons;
        }
    }

    private static List<WebViewerButtonConfig> TryLoadFromDatabase(out bool tableExists)
    {
        tableExists = false;
        try
        {
            using var connection = new SqlConnection(Program.Connectionstring);
            tableExists = connection.ExecuteScalar<int>(
                "SELECT CASE WHEN OBJECT_ID('[dbo].[Web_Buttons]', 'U') IS NULL THEN 0 ELSE 1 END",
                commandTimeout: 10) == 1;

            if (!tableExists)
                return new List<WebViewerButtonConfig>();

            var rows = connection.Query<WebViewerButtonDbRow>(
                @"SELECT TOP (@Take)
                      Name,
                      IconPath,
                      Url,
                      FrameWidth,
                      FrameHeight
                  FROM [dbo].[Web_Buttons] WITH (NOLOCK)
                  WHERE IsEnabled = 1
                  ORDER BY DisplayOrder, ID;",
                new { Take = MaxButtons },
                commandTimeout: 10).ToList();

            var buttons = rows
                .Select(row => new WebViewerButtonConfig
                {
                    Name = row.Name ?? string.Empty,
                    Icon = row.IconPath ?? string.Empty,
                    Url = row.Url ?? string.Empty,
                    FrameWidth = row.FrameWidth,
                    FrameHeight = row.FrameHeight
                })
                .Where(IsValid)
                .Take(MaxButtons)
                .Select(Normalize)
                .ToList();

            Log.Verbose("WebViewer config loaded from database: {Count} button(s)", buttons.Count);

            return buttons;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to load WebViewer buttons from database. Falling back to webviewer.json.");
            return new List<WebViewerButtonConfig>();
        }
    }

    private static string GetConfigPath()
    {
        return Path.Combine(AppContext.BaseDirectory, "webviewer.json");
    }

    private static void EnsureDefaultConfig(string path)
    {
        if (File.Exists(path))
            return;

        var defaultConfig = new WebViewerConfigFile
        {
            Buttons = new List<WebViewerButtonConfig>
            {
                new()
                {
                    Name = "Website",
                    Icon = "clientlibrary\\guides\\kmt_web_viewer_1.ddj",
                    Url = "https://example.com",
                    FrameWidth = 1000,
                    FrameHeight = 650
                }
            }
        };

        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllText(path, JsonConvert.SerializeObject(defaultConfig, Formatting.Indented));
    }

    private static bool IsValid(WebViewerButtonConfig button)
    {
        return !string.IsNullOrWhiteSpace(button.Name) &&
               !string.IsNullOrWhiteSpace(button.Icon) &&
               !string.IsNullOrWhiteSpace(button.Url);
    }

    private static WebViewerButtonConfig Normalize(WebViewerButtonConfig button)
    {
        return new WebViewerButtonConfig
        {
            Name = Limit(button.Name.Trim(), 64),
            Icon = Limit(button.Icon.Trim(), 260),
            Url = Limit(button.Url.Trim(), 512),
            FrameWidth = Math.Clamp(button.FrameWidth, 320, 1200),
            FrameHeight = Math.Clamp(button.FrameHeight, 240, 900)
        };
    }

    private static string Limit(string value, int max)
    {
        return value.Length <= max ? value : value[..max];
    }

    private sealed class WebViewerButtonDbRow
    {
        public string? Name { get; set; }
        public string? IconPath { get; set; }
        public string? Url { get; set; }
        public int FrameWidth { get; set; }
        public int FrameHeight { get; set; }
    }
}
