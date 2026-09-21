using System;
using System.Runtime.InteropServices;
using KMTGuard.RuntimeContract;
using KMTGuard.ServerManagers;

namespace KMTGuard.ConsoleUi;

public static class FilterConsole
{
    private sealed record ChannelProfile(
        string Code,
        string Name,
        string Mission,
        ConsoleColor Accent);

    private const int Width = 88;
    private static readonly object Sync = new();
    private static FilterRole _role = FilterRole.All;
    private static ChannelProfile _profile = GetProfile(FilterRole.All);

    public static bool IsEnabled { get; private set; }

    public static void Configure(FilterRole role)
    {
        _role = role;
        _profile = GetProfile(role);
        IsEnabled = HasConsoleWindow();
    }

    public static void Initialize()
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            try
            {
                Console.OutputEncoding = System.Text.Encoding.UTF8;
                Console.Title = $"KMTGuard / {_profile.Code} / Initializing";
                Console.Clear();
                TrySetWindowSize();
                DrawHeader("BOOT SEQUENCE");
            }
            catch
            {
                IsEnabled = false;
            }
        }
    }

    public static void DrawHeader(string state)
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            WriteTopRule($" KMTGUARD / {_profile.Code} ");
            WritePanelLine(_profile.Name.ToUpperInvariant(), _profile.Accent, centered: true);
            WritePanelLine(_profile.Mission, ConsoleColor.DarkGray, centered: true);
            WriteDivider();
            WriteKeyValue("RUNTIME STATE", state, _profile.Accent);
            WriteKeyValue("CHANNEL", _role.ToString().ToUpperInvariant(), ConsoleColor.White);
            WriteBottomRule();
            Console.WriteLine();
        }
    }

    public static void WriteStartupStep(string name, string detail, bool ok = true)
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            WriteColored(ok ? "  ◆ PASS  " : "  ◆ HALT  ", ok ? _profile.Accent : ConsoleColor.Red);
            WriteColored(name.ToUpperInvariant().PadRight(22), ConsoleColor.White);
            WriteColored("│ ", ConsoleColor.DarkGray);
            WriteLine(detail, ok ? ConsoleColor.Gray : ConsoleColor.Red);
        }
    }

    public static void WriteReady(string serverIp, string database)
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            Console.WriteLine();
            WriteTopRule($" {_profile.Code} / OPERATIONAL ");
            WritePanelLine("RUNTIME CHANNEL ACCEPTING TRAFFIC", _profile.Accent, centered: true);
            WriteDivider();
            WriteKeyValue("AUTHORIZED NODE", serverIp, ConsoleColor.White);
            WriteKeyValue("CONTROL DATABASE", database, ConsoleColor.White);
            WriteKeyValue("EVENT SIGNATURE", $"KMT/{_role.ToString().ToUpperInvariant()} · numbered native stream", _profile.Accent);
            WriteBottomRule();
            Console.WriteLine();

            if (_role == FilterRole.All)
                WriteHelp();
            else
                WriteLine("  Live events follow below. Service control is available from KMTGuard Desktop.", ConsoleColor.DarkGray);
        }
    }

    public static void WriteHelp()
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            WriteLine("  CONTROL DECK", _profile.Accent);
            WriteLine("  /status          Live connection snapshot", ConsoleColor.Gray);
            WriteLine("  /reload 1..3     Settings, notices, or event schedule", ConsoleColor.Gray);
            WriteLine("  /reload language Reload editable player messages", ConsoleColor.Gray);
            WriteLine("  /clientless ...  Clientless account operations", ConsoleColor.Gray);
            WriteLine("  /exit            Graceful runtime shutdown", ConsoleColor.Gray);
            Console.WriteLine();
        }
    }

    public static void WriteConnectionSnapshot()
    {
        if (!IsEnabled)
            return;

        var gatewayCount = ServerManager.GatewaySessions?.Count ?? 0;
        var agentCount = ServerManager.AgentSessions?.Count ?? 0;
        var onlinePlayerCount = ServerManager.GetOnlinePlayerCount();
        var downloadCount = ServerManager.DownloadSessions?.Count ?? 0;

        lock (Sync)
        {
            WriteTopRule(" LIVE TRAFFIC SNAPSHOT ");
            WriteKeyValue("ONLINE PLAYERS", onlinePlayerCount.ToString(), ConsoleColor.White);
            WriteKeyValue("AGENT SESSIONS", agentCount.ToString(), ConsoleColor.White);
            WriteKeyValue("GATEWAY SESSIONS", gatewayCount.ToString(), ConsoleColor.White);
            WriteKeyValue("DOWNLOAD SESSIONS", downloadCount.ToString(), ConsoleColor.White);
            WriteBottomRule();
            Console.WriteLine();
        }
    }

    public static void SetConnectionTitle()
    {
        if (!IsEnabled)
            return;

        var gatewayCount = ServerManager.GatewaySessions?.Count ?? 0;
        var agentCount = ServerManager.AgentSessions?.Count ?? 0;
        var onlinePlayerCount = ServerManager.GetOnlinePlayerCount();
        var downloadCount = ServerManager.DownloadSessions?.Count ?? 0;
        Console.Title = _role switch
        {
            FilterRole.Agent => $"KMTGuard / A.GUARD / Players {onlinePlayerCount} / Sessions {agentCount}",
            FilterRole.Download => $"KMTGuard / D.RELAY / Transfers {downloadCount}",
            FilterRole.Gateway => $"KMTGuard / G.EDGE / Sessions {gatewayCount}",
            _ => $"KMTGuard / CORE / Online {onlinePlayerCount} / A {agentCount} / G {gatewayCount} / D {downloadCount}"
        };
    }

    public static void Prompt()
    {
        if (!IsEnabled || _role != FilterRole.All)
            return;

        lock (Sync)
        {
            WriteColored("  KMT", _profile.Accent);
            WriteColored("::CORE", ConsoleColor.White);
            WriteColored("  ›  ", ConsoleColor.DarkGray);
        }
    }

    public static void WriteCommandError(string message)
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            WriteColored("  ◇ ACTION  ", ConsoleColor.Yellow);
            WriteLine(message, ConsoleColor.Gray);
        }
    }

    public static void WriteCommandSuccess(string message)
    {
        if (!IsEnabled)
            return;

        lock (Sync)
        {
            WriteColored("  ◆ DONE    ", _profile.Accent);
            WriteLine(message, ConsoleColor.Gray);
        }
    }

    private static void WriteTopRule(string label)
    {
        var safeLabel = label.Length > Width - 4 ? label[..(Width - 4)] : label;
        var remaining = Width - safeLabel.Length - 2;
        WriteColored("╭", ConsoleColor.DarkGray);
        WriteColored(safeLabel, _profile.Accent);
        WriteLine(new string('─', remaining) + "╮", ConsoleColor.DarkGray);
    }

    private static void WriteDivider() =>
        WriteLine("├" + new string('─', Width - 2) + "┤", ConsoleColor.DarkGray);

    private static void WriteBottomRule() =>
        WriteLine("╰" + new string('─', Width - 2) + "╯", ConsoleColor.DarkGray);

    private static void WriteKeyValue(string key, string value, ConsoleColor valueColor)
    {
        var prefix = $"│  {key.PadRight(20)} ";
        var maxValueLength = Width - prefix.Length - 2;
        if (value.Length > maxValueLength)
            value = value[..Math.Max(0, maxValueLength - 1)] + "…";

        WriteColored(prefix, ConsoleColor.DarkGray);
        WriteColored(value, valueColor);
        WriteLine(new string(' ', Width - prefix.Length - value.Length - 1) + "│", ConsoleColor.DarkGray);
    }

    private static void WritePanelLine(string text, ConsoleColor color, bool centered)
    {
        var innerWidth = Width - 2;
        if (text.Length > innerWidth - 2)
            text = text[..(innerWidth - 3)] + "…";

        var left = centered ? (innerWidth - text.Length) / 2 : 2;
        WriteColored("│" + new string(' ', left), ConsoleColor.DarkGray);
        WriteColored(text, color);
        WriteLine(new string(' ', innerWidth - left - text.Length) + "│", ConsoleColor.DarkGray);
    }

    private static void WriteColored(string text, ConsoleColor color)
    {
        var original = Console.ForegroundColor;
        Console.ForegroundColor = color;
        Console.Write(text);
        Console.ForegroundColor = original;
    }

    private static void WriteLine(string text, ConsoleColor color)
    {
        WriteColored(text, color);
        Console.WriteLine();
    }

    private static ChannelProfile GetProfile(FilterRole role) => role switch
    {
        FilterRole.Agent => new ChannelProfile(
            "A.GUARD",
            "Agent protection channel",
            "Session intelligence · gameplay pipeline · active defense",
            ConsoleColor.Blue),
        FilterRole.Download => new ChannelProfile(
            "D.RELAY",
            "Download relay channel",
            "Patch traffic · content delivery · transfer integrity",
            ConsoleColor.Cyan),
        FilterRole.Gateway => new ChannelProfile(
            "G.EDGE",
            "Gateway edge channel",
            "Login edge · shard discovery · access control",
            ConsoleColor.Yellow),
        _ => new ChannelProfile(
            "CORE",
            "Unified filter control deck",
            "Silkroad traffic orchestration · protection · operations",
            ConsoleColor.Magenta)
    };

    private static bool HasConsoleWindow()
    {
        try
        {
            return GetConsoleWindow() != IntPtr.Zero;
        }
        catch
        {
            return false;
        }
    }

    private static void TrySetWindowSize()
    {
        if (!OperatingSystem.IsWindows())
            return;

        try
        {
            if (Console.BufferWidth < Width)
                Console.BufferWidth = Width;
            if (Console.WindowWidth < Width)
                Console.WindowWidth = Width;
        }
        catch
        {
        }
    }

    [DllImport("kernel32.dll")]
    private static extern IntPtr GetConsoleWindow();
}
