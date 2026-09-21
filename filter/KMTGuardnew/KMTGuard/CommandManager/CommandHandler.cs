using System;
using System.Threading.Tasks;
using KMTGuard.ConsoleUi;
using KMTGuard.Database;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Localization;
using KMTGuard.ServerManagers;
using Serilog;

namespace KMTGuard.CommandManager
{
    public class CommandHandler
    {
        public async Task ExecuteCommand(string command)
        {
            try
            {
                command = command.Trim();
                var parts = command.Split(' ', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

                if (command.Equals("/help", StringComparison.OrdinalIgnoreCase) ||
                    command.Equals("help", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteHelp();
                }
                else if (command.Equals("/status", StringComparison.OrdinalIgnoreCase) ||
                         command.Equals("status", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteConnectionSnapshot();
                }
                else if (command.Equals("start", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteCommandError("Enter UserID:");
                    string? userId = Console.ReadLine();
                    if (!string.IsNullOrWhiteSpace(userId))
                        FilterConsole.WriteCommandSuccess($"UserID received: {userId}");
                    else
                        FilterConsole.WriteCommandError("Invalid UserID.");
                }
                else if (command.Equals("/reload", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteCommandError("Reload options: /reload 1 = settings, /reload 2 = notices, /reload 3 = event schedule.");
                }
                else if (command.Equals("/reload 1", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteCommandError("Reloading _ServerSettings...");
                    // await Management.ReloadServerSettings();
                    FilterConsole.WriteCommandSuccess("Reload _ServerSettings completed.");
                }
                else if (command.Equals("/reload 2", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteCommandError("Reloading [dbo].[System_Notices]...");
                    // await Management.LoadNoticesIntoCache();
                    FilterConsole.WriteCommandSuccess("Reload [dbo].[System_Notices] completed.");
                }
                else if (command.Equals("/reload 3", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteCommandError("Reloading EventSchedule...");
                    await RefManager.LoadRefEventSchedule();
                    await Scheduler.ReloadAsync();
                    var autoEventsReloaded = await AutoEventService.ReloadAsync();
                    if (autoEventsReloaded)
                        FilterConsole.WriteCommandSuccess("Reload EventSchedule completed.");
                    else
                        FilterConsole.WriteCommandError("EventSchedule reload was incomplete; Auto Events kept the last known-good configuration.");
                }
                else if (command.Equals("/reload language", StringComparison.OrdinalIgnoreCase) ||
                         command.Equals("/reloadlang", StringComparison.OrdinalIgnoreCase))
                {
                    var result = PlayerLanguage.Reload();
                    FilterConsole.WriteCommandSuccess(
                        $"Player language reloaded: {result.Language}, {result.ActiveKeyCount} keys, {result.MissingKeyCount} English fallback.");
                }
                else if (parts.Length >= 2 && parts[0].Equals("/event", StringComparison.OrdinalIgnoreCase))
                {
                    switch (parts[1].ToLowerInvariant())
                    {
                        case "start" when parts.Length >= 3:
                            FilterConsole.WriteCommandSuccess(await AutoEventService.StartAsync(string.Join(' ', parts.Skip(2)), "Console"));
                            break;
                        case "stop":
                            FilterConsole.WriteCommandSuccess(await AutoEventService.StopAsync("Console"));
                            break;
                        case "reload":
                            if (await AutoEventService.ReloadAsync())
                                FilterConsole.WriteCommandSuccess("Auto Events reloaded.");
                            else
                                FilterConsole.WriteCommandError("Auto Events reload failed; the last known-good configuration remains active.");
                            break;
                        case "status":
                            FilterConsole.WriteCommandSuccess(AutoEventService.GetStatus());
                            break;
                        default:
                            FilterConsole.WriteCommandError("Event commands: /event start Retype, /event stop, /event reload, /event status.");
                            break;
                    }
                }
                else if (parts.Length >= 1 && parts[0].Equals("/clientless", StringComparison.OrdinalIgnoreCase))
                {
                    await ExecuteClientlessCommand(parts);
                }
                else
                {
                    FilterConsole.WriteCommandError($"Unknown command: {command}. Type /help.");
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Command failed");
                FilterConsole.WriteCommandError(ex.Message);
            }
        }

        private static async Task ExecuteClientlessCommand(string[] parts)
        {
            if (parts.Length == 1 || parts[1].Equals("help", StringComparison.OrdinalIgnoreCase))
            {
                FilterConsole.WriteCommandSuccess("Clientless commands: /clientless status, /clientless start [count], /clientless stop, /clientless reload [count], /clientless add <account> <password> <char> [shard] [locale], /clientless enable <id>, /clientless disable <id>.");
                return;
            }

            switch (parts[1].ToLowerInvariant())
            {
                case "status":
                    FilterConsole.WriteCommandSuccess(await KMTGuard.Clientless.ClientlessManager.GetStatusAsync());
                    break;

                case "start":
                    var startCount = ParseOptionalCount(parts);
                    var startResult = await KMTGuard.Clientless.ClientlessManager.StartAsync(startCount);
                    FilterConsole.WriteCommandSuccess(startResult);
                    break;

                case "stop":
                    KMTGuard.Clientless.ClientlessManager.Stop();
                    FilterConsole.WriteCommandSuccess("Clientless stopped.");
                    break;

                case "reload":
                    FilterConsole.WriteCommandSuccess(await KMTGuard.Clientless.ClientlessManager.ReloadAsync(ParseOptionalCount(parts)));
                    break;

                case "add":
                    if (parts.Length < 5)
                    {
                        FilterConsole.WriteCommandError("Usage: /clientless add <account> <password> <char> [shard] [locale]");
                        return;
                    }

                    var shard = parts.Length >= 6 && ushort.TryParse(parts[5], out var parsedShard)
                        ? parsedShard
                        : (ushort)64;
                    var locale = parts.Length >= 7 && byte.TryParse(parts[6], out var parsedLocale)
                        ? parsedLocale
                        : (byte)22;

                    FilterConsole.WriteCommandSuccess(await KMTGuard.Clientless.ClientlessManager.UpsertAccountAsync(
                        parts[2],
                        parts[3],
                        parts[4],
                        shard,
                        locale));
                    break;

                case "enable":
                case "disable":
                    if (parts.Length < 3 || !int.TryParse(parts[2], out var id))
                    {
                        FilterConsole.WriteCommandError("Usage: /clientless enable <id> or /clientless disable <id>");
                        return;
                    }

                    FilterConsole.WriteCommandSuccess(await KMTGuard.Clientless.ClientlessManager.SetEnabledAsync(
                        id,
                        parts[1].Equals("enable", StringComparison.OrdinalIgnoreCase)));
                    break;

                default:
                    FilterConsole.WriteCommandError("Clientless commands: status, start [count], stop, reload [count], add, enable, disable.");
                    break;
            }
        }

        private static int? ParseOptionalCount(string[] parts)
        {
            if (parts.Length < 3)
                return null;

            if (!int.TryParse(parts[2], out var count) || count <= 0)
                throw new ArgumentException("Count must be a positive number.");

            return count;
        }
    }
}
