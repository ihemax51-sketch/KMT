using System.Collections.Concurrent;
using System.Data;
using System.Text;
using System.Text.RegularExpressions;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.ServerManagers;
using KMTGuard.Features.Telegram;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Features.AutoEvents;

public static class AutoEventService
{
    private const string EventDatabaseName = "Events";
    private static readonly TimeSpan SchedulePollInterval = TimeSpan.FromSeconds(10);
    internal static readonly TimeSpan MinimumAutomaticEventGap = TimeSpan.FromMinutes(15);
    private static readonly TimeSpan CommandRecoveryInterval = TimeSpan.FromMinutes(1);
    private static readonly SemaphoreSlim StateLock = new(1, 1);
    private static readonly Regex SpaceRegex = new(@"\s+", RegexOptions.Compiled);
    private static readonly HashSet<string> ChatAnswerEvents = new(StringComparer.OrdinalIgnoreCase)
    {
        "Retype",
        "Trivia",
        "FirstType",
        "Math"
    };
    private static readonly IReadOnlySet<string> EventTableNames =
        new HashSet<string>(StringComparer.Ordinal)
        {
            "_AutoEventCommandQueue",
            "_AutoEventConfig",
            "_AutoEventReward",
            "_AutoEventRound",
            "_AutoEventRoundContent",
            "_AutoEventRun",
            "_AutoEventSchedule",
            "_AutoEventWinnerLog",
            "_HideAndSeekSchedule",
            "_SurvivalPartySchedule",
            "_SurvivalSoloSchedule"
        };

    private static Dictionary<string, AutoEventConfig> _configs = new(StringComparer.OrdinalIgnoreCase);
    private static Dictionary<string, List<AutoEventContent>> _content = new(StringComparer.OrdinalIgnoreCase);
    private static Dictionary<string, List<AutoEventReward>> _rewards = new(StringComparer.OrdinalIgnoreCase);
    private static ActiveEventRun? _activeRun;
    private static CancellationTokenSource? _serviceCts;
    private static Task? _commandTask;
    private static volatile bool _schemaReady;
    private static DateTime _nextSchedulePollUtc = DateTime.MinValue;
    private static DateTime _nextCommandRecoveryUtc = DateTime.MinValue;
    private static readonly ConcurrentDictionary<int, PartyMatchingRequest> LuckyPartyMatchingRequests = new();

    private static string EventTable(string tableName)
    {
        return $"{SqlIdentifier.Quote(EventDatabaseName)}.dbo.{SqlIdentifier.QuoteAllowed(tableName, EventTableNames)}";
    }

    public static async Task InitializeAsync()
    {
        var previousCts = _serviceCts;
        var previousTask = _commandTask;
        previousCts?.Cancel();
        if (previousTask != null)
        {
            try
            {
                await previousTask;
            }
            catch (OperationCanceledException)
            {
                // Expected while replacing the background worker.
            }
        }
        previousCts?.Dispose();

        await ReloadAsync();
        _serviceCts = new CancellationTokenSource();
        _commandTask = Task.Run(() => ProcessCommandsAsync(_serviceCts.Token));
    }

    public static void Stop() => StopAsync().GetAwaiter().GetResult();

    public static async Task StopAsync()
    {
        _serviceCts?.Cancel();
        var run = _activeRun;
        if (run != null)
        {
            run.TryRequestStop("Filter shutdown");
            run.Cancellation.Cancel();
        }

        var tasks = new List<Task>();
        if (_commandTask != null)
            tasks.Add(_commandTask);
        if (run != null)
            tasks.Add(run.Completion.Task);

        try
        {
            if (tasks.Count > 0)
                await Task.WhenAll(tasks);
        }
        catch (OperationCanceledException)
        {
            // Expected during shutdown.
        }
        finally
        {
            var source = Interlocked.Exchange(ref _serviceCts, null);
            Interlocked.Exchange(ref _commandTask, null);
            source?.Dispose();
        }
    }

    public static async Task<bool> ReloadAsync()
    {
        var hadUsableSnapshot = _schemaReady;
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureSurvivalPartySchemaAsync(connection);
            await EnsureAutoEventScheduleSchemaAsync(connection);
            try
            {
                await SurvivalSoloEventService.EnsureSchemaAsync(connection);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Survival Solo schema initialization failed; existing Auto Events remain available.");
            }
            try
            {
                await CompetitiveEventService.EnsureSchemaAsync(connection);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Competitive Events schema initialization failed; existing Auto Events remain available.");
            }
            try
            {
                await HideAndSeekEventService.EnsureSchemaAsync(connection);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Hide and Seek schema initialization failed; existing Auto Events remain available.");
            }

            _schemaReady = await HasTableAsync(connection, "_AutoEventConfig") &&
                           await HasTableAsync(connection, "_AutoEventReward");
            if (!_schemaReady)
            {
                Log.Warning("Auto Events schema is not installed. Run database/migrations/20260711_events_database.sql.");
                return false;
            }

            var configs = (await connection.QueryAsync<AutoEventConfig>(
                $"SELECT * FROM {EventTable("_AutoEventConfig")} WITH (NOLOCK) WHERE EventCode NOT IN (N'SPARTY', N'SSOLO')"))
                .ToDictionary(x => x.EventCode, StringComparer.OrdinalIgnoreCase);

            var content = (await connection.QueryAsync<AutoEventContent>(
                $"SELECT * FROM {EventTable("_AutoEventRoundContent")} WITH (NOLOCK) WHERE IsActive = 1 AND EventCode NOT IN (N'SPARTY', N'SSOLO')"))
                .GroupBy(x => x.EventCode, StringComparer.OrdinalIgnoreCase)
                .ToDictionary(x => x.Key, x => x.ToList(), StringComparer.OrdinalIgnoreCase);

            var rewards = (await connection.QueryAsync<AutoEventReward>(
                $"SELECT * FROM {EventTable("_AutoEventReward")} WITH (NOLOCK) WHERE IsActive = 1 AND EventCode NOT IN (N'SPARTY', N'SSOLO')"))
                .GroupBy(x => x.EventCode, StringComparer.OrdinalIgnoreCase)
                .ToDictionary(x => x.Key, x => x.OrderBy(r => r.Placement).ThenBy(r => r.RewardID).ToList(), StringComparer.OrdinalIgnoreCase);

            await StateLock.WaitAsync();
            try
            {
                _configs = configs;
                _content = content;
                _rewards = rewards;
            }
            finally
            {
                StateLock.Release();
            }

            Log.Information("Automation blueprints synchronized :: scenarios={ConfigCount:N0} :: reward entries={RewardCount:N0}",
                configs.Count,
                rewards.Values.Sum(x => x.Count));
            return true;
        }
        catch (Exception ex)
        {
            // A transient database outage must not discard the last known-good
            // snapshot or permanently stop the command/schedule worker.
            _schemaReady = hadUsableSnapshot;
            Log.Warning(ex, "Auto Events reload failed");
            return false;
        }
    }

    public static string GetStatus()
    {
        var run = _activeRun;
        if (run == null)
            return "AutoEvent: idle.";

        var round = run.CurrentRound;
        if (round == null)
            return $"AutoEvent: {run.Config.EventCode} starting.";

        var winner = string.IsNullOrWhiteSpace(round.WinnerCharName) ? "no winner yet" : round.WinnerCharName;
        return $"AutoEvent: {run.Config.EventCode} round {round.RoundNo}/{run.Config.RoundCount}, winner: {winner}.";
    }

    public static async Task<string> StartAsync(string eventCode, string startedBy)
    {
        eventCode = NormalizeEventCode(eventCode);
        if (eventCode.Length == 0)
            return "Event code is required.";

        if (!_schemaReady)
            await ReloadAsync();

        await StateLock.WaitAsync();
        try
        {
            if (_activeRun != null)
                return $"Another event is already running: {_activeRun.Config.EventCode}.";
        }
        finally
        {
            StateLock.Release();
        }

        AutoEventConfig? config = null;
        SurvivalPartyRuntimeConfig? survivalPartyConfig = null;
        SurvivalSoloRuntimeConfig? survivalSoloConfig = null;
        CompetitiveEventRuntimeConfig? competitiveConfig = null;
        HideAndSeekRuntimeConfig? hideAndSeekConfig = null;
        if (eventCode.Equals("SPARTY", StringComparison.OrdinalIgnoreCase))
        {
            survivalPartyConfig = await SurvivalPartyEventService.LoadRuntimeConfigAsync();
            config = new AutoEventConfig
            {
                EventCode = survivalPartyConfig.EventCode,
                DisplayName = survivalPartyConfig.DisplayName,
                Enabled = survivalPartyConfig.Enabled,
                StartDelaySeconds = survivalPartyConfig.StartDelaySeconds,
                RoundCount = 1,
                RoundDurationSeconds = survivalPartyConfig.FightSeconds,
                InterRoundDelaySeconds = 0,
                MinLevel = survivalPartyConfig.MinLevel,
                HwidLimit = survivalPartyConfig.HwidLimit,
                UniqueWinnerPerRun = false,
                RequireHwid = survivalPartyConfig.RequireHwid
            };
        }
        else if (eventCode.Equals("SSOLO", StringComparison.OrdinalIgnoreCase))
        {
            survivalSoloConfig = await SurvivalSoloEventService.LoadRuntimeConfigAsync();
            config = new AutoEventConfig
            {
                EventCode = survivalSoloConfig.EventCode,
                DisplayName = survivalSoloConfig.DisplayName,
                Enabled = survivalSoloConfig.Enabled,
                StartDelaySeconds = survivalSoloConfig.StartDelaySeconds,
                RoundCount = 1,
                RoundDurationSeconds = survivalSoloConfig.FightSeconds,
                InterRoundDelaySeconds = 0,
                MinLevel = survivalSoloConfig.MinLevel,
                HwidLimit = survivalSoloConfig.HwidLimit,
                UniqueWinnerPerRun = false,
                RequireHwid = survivalSoloConfig.RequireHwid
            };
        }
        else if (eventCode is "LMS" or "MADNESS" or "DTT")
        {
            competitiveConfig = await CompetitiveEventService.LoadRuntimeConfigAsync(eventCode);
            var validationError = CompetitiveEventService.ValidateForStart(competitiveConfig);
            if (validationError != null)
                return validationError;
            config = new AutoEventConfig
            {
                EventCode = competitiveConfig.EventCode,
                DisplayName = competitiveConfig.DisplayName,
                Enabled = competitiveConfig.Enabled,
                StartDelaySeconds = competitiveConfig.StartDelaySeconds,
                RoundCount = 1,
                RoundDurationSeconds = competitiveConfig.FightSeconds,
                InterRoundDelaySeconds = 0,
                MinLevel = competitiveConfig.MinLevel,
                HwidLimit = competitiveConfig.HwidLimit,
                UniqueWinnerPerRun = false,
                RequireHwid = competitiveConfig.RequireHwid
            };
        }
        else if (eventCode.Equals("HNS", StringComparison.OrdinalIgnoreCase))
        {
            hideAndSeekConfig = await HideAndSeekEventService.LoadRuntimeConfigAsync();
            if (!hideAndSeekConfig.Enabled)
                return "Event HNS is disabled.";
            try
            {
                hideAndSeekConfig = await HideAndSeekEventService.PrepareForStartAsync(hideAndSeekConfig);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Hide and Seek start validation failed");
                return ex.Message;
            }
            var validationError = HideAndSeekEventService.ValidateForStart(hideAndSeekConfig);
            if (validationError != null)
                return validationError;
            config = new AutoEventConfig
            {
                EventCode = hideAndSeekConfig.EventCode,
                DisplayName = hideAndSeekConfig.DisplayName,
                Enabled = hideAndSeekConfig.Enabled,
                StartDelaySeconds = hideAndSeekConfig.StartDelaySeconds,
                RoundCount = 1,
                RoundDurationSeconds = hideAndSeekConfig.SearchSeconds,
                InterRoundDelaySeconds = 0,
                MinLevel = hideAndSeekConfig.MinLevel,
                HwidLimit = hideAndSeekConfig.HwidLimit,
                UniqueWinnerPerRun = true,
                RequireHwid = hideAndSeekConfig.RequireHwid
            };
        }

        await StateLock.WaitAsync();
        try
        {
            if (_activeRun != null)
                return $"Another event is already running: {_activeRun.Config.EventCode}.";

            if (config == null && !_configs.TryGetValue(eventCode, out config))
                return $"Unknown event: {eventCode}.";

            if (!config.Enabled)
                return $"Event {eventCode} is disabled.";

            if (IsScheduledStart(startedBy))
            {
                var lastFinishedAtUtc = await LoadMostRecentFinishedRunUtcAsync();
                if (!HasAutomaticEventGapElapsed(lastFinishedAtUtc, DateTime.UtcNow))
                {
                    var remaining = MinimumAutomaticEventGap - (DateTime.UtcNow - lastFinishedAtUtc!.Value);
                    var remainingMinutes = Math.Max(1, (int)Math.Ceiling(remaining.TotalMinutes));
                    return $"Scheduled event waiting for the 15-minute safety gap ({remainingMinutes} minute(s) remaining).";
                }
            }

            var runId = await CreateRunAsync(config.EventCode, startedBy);
            var contentSnapshot = _content.TryGetValue(config.EventCode, out var configuredContent)
                ? configuredContent.ToArray()
                : Array.Empty<AutoEventContent>();
            var rewardSnapshot = _rewards.TryGetValue(config.EventCode, out var configuredRewards)
                ? configuredRewards.ToArray()
                : Array.Empty<AutoEventReward>();
            var run = new ActiveEventRun(runId, config, startedBy, contentSnapshot, rewardSnapshot)
            {
                SurvivalPartyConfig = survivalPartyConfig,
                SurvivalSoloConfig = survivalSoloConfig,
                CompetitiveConfig = competitiveConfig,
                HideAndSeekConfig = hideAndSeekConfig
            };
            _activeRun = run;
            run.ExecutionTask = Task.Run(() => RunEventAsync(run));
        }
        finally
        {
            StateLock.Release();
        }

        return config.StartDelaySeconds > 0
            ? $"{config.EventCode} scheduled. First round starts in {PlayerLanguage.FormatDurationSeconds(config.StartDelaySeconds)}. Rounds: {config.RoundCount}."
            : $"Started {config.EventCode} with {config.RoundCount} rounds.";
    }

    public static async Task<string> StopAsync(string reason)
    {
        var run = _activeRun;
        if (run == null)
            return "No AutoEvent is running.";

        run.TryRequestStop(reason);
        run.Cancellation.Cancel();
        await run.Completion.Task;
        return $"Stopped {run.Config.EventCode}.";
    }

    public static async Task HandleChatAnswerAsync(ISession session, byte chatType, string message)
    {
        if (chatType != 0x01 || string.IsNullOrWhiteSpace(message))
            return;

        var run = _activeRun;
        var round = run?.CurrentRound;
        if (run == null || round == null || !ChatAnswerEvents.Contains(run.Config.EventCode))
            return;

        await TryWinRoundAsync(run, round, session, message, NormalizeAnswer(message));
    }

    public static async Task HandleAlchemySuccessAsync(ISession session, int plus)
    {
        var run = _activeRun;
        var round = run?.CurrentRound;
        if (run == null || round == null || !run.Config.EventCode.Equals("Alchemy", StringComparison.OrdinalIgnoreCase))
            return;

        await TryWinRoundAsync(run, round, session, plus.ToString(), plus.ToString());
    }

    internal static void MarkPartyMatchingFormRequested(ISession session)
    {
        var run = _activeRun;
        var round = run?.CurrentRound;
        if (run == null || round == null || !IsCurrentEvent("LuckyParty"))
            return;

        if (session.SessionData.Charid <= 0 || !session.CharacterGameReady)
            return;

        LuckyPartyMatchingRequests[session.SessionData.Charid] =
            new PartyMatchingRequest(run.RunID, round.RoundID);
    }

    internal static void RegisterPartyMatchingFormCreated(ISession session, int matchingId)
    {
        var run = _activeRun;
        var round = run?.CurrentRound;
        if (run == null || round == null || !IsCurrentEvent("LuckyParty"))
            return;

        if (!LuckyPartyMatchingRequests.TryRemove(session.SessionData.Charid, out var request) ||
            request.RunID != run.RunID ||
            request.RoundID != round.RoundID ||
            matchingId <= 0)
        {
            return;
        }

        RegisterParticipation(
            run,
            round,
            ParticipationKind.Party,
            session,
            matchingId.ToString(),
            $"MatchingID={matchingId}");
    }

    internal static void CancelPartyMatchingFormRequest(ISession session)
    {
        if (session.SessionData.Charid > 0)
            LuckyPartyMatchingRequests.TryRemove(session.SessionData.Charid, out _);
    }

    public static void RegisterGlobalSent(ISession session, string message)
    {
        var run = _activeRun;
        var round = run?.CurrentRound;
        if (run == null || round == null || !IsCurrentEvent("LuckyGlobal"))
            return;

        RegisterParticipation(run, round, ParticipationKind.Global, session, "global", Trim(message, 128));
    }

    public static void RegisterStallOpened(ISession session)
    {
        var run = _activeRun;
        var round = run?.CurrentRound;
        if (run == null || round == null || !IsLuckyStallEvent(run.Config.EventCode))
            return;

        RegisterParticipation(run, round, ParticipationKind.Stall, session, "stall", "Stall opened");
    }

    private static async Task RunEventAsync(ActiveEventRun run)
    {
        try
        {
            if (run.Config.EventCode.Equals("SPARTY", StringComparison.OrdinalIgnoreCase))
            {
                await SendStartCountdownAsync(run);
                var survivalConfig = run.SurvivalPartyConfig ?? await SurvivalPartyEventService.LoadRuntimeConfigAsync();
                await SurvivalPartyEventService.RunAsync(survivalConfig, run.RunID, run.StartedBy, run.Cancellation.Token);
                run.Cancellation.Token.ThrowIfCancellationRequested();
                await FinishRunAsync(run, "Completed", "Survival Party completed.");
                return;
            }
            if (run.Config.EventCode.Equals("SSOLO", StringComparison.OrdinalIgnoreCase))
            {
                await SendStartCountdownAsync(run);
                var soloConfig = run.SurvivalSoloConfig ?? await SurvivalSoloEventService.LoadRuntimeConfigAsync();
                await SurvivalSoloEventService.RunAsync(soloConfig, run.RunID, run.StartedBy, run.Cancellation.Token);
                run.Cancellation.Token.ThrowIfCancellationRequested();
                await FinishRunAsync(run, "Completed", "Survival Solo completed.");
                return;
            }
            if (run.Config.EventCode is "LMS" or "MADNESS" or "DTT")
            {
                await SendStartCountdownAsync(run);
                var competitiveConfig = run.CompetitiveConfig ?? await CompetitiveEventService.LoadRuntimeConfigAsync(run.Config.EventCode);
                await CompetitiveEventService.RunAsync(competitiveConfig, run.RunID, run.StartedBy, run.Cancellation.Token);
                run.Cancellation.Token.ThrowIfCancellationRequested();
                await FinishRunAsync(run, "Completed", $"{competitiveConfig.DisplayName} completed.");
                return;
            }
            if (run.Config.EventCode.Equals("HNS", StringComparison.OrdinalIgnoreCase))
            {
                await SendStartCountdownAsync(run);
                var hideAndSeekConfig = run.HideAndSeekConfig ?? await HideAndSeekEventService.LoadRuntimeConfigAsync();
                await HideAndSeekEventService.RunAsync(
                    hideAndSeekConfig,
                    run.RunID,
                    run.StartedBy,
                    run.Cancellation.Token);
                run.Cancellation.Token.ThrowIfCancellationRequested();
                await FinishRunAsync(run, "Completed", "Hide and Seek completed.");
                return;
            }

            await SendStartCountdownAsync(run);
            await BroadcastNoticeAsync(PlayerLanguage.Get("AutoEvent.Started", run.Config.DisplayName, run.Config.RoundCount), NoticeType.NOTICE);

            for (var roundNo = 1; roundNo <= run.Config.RoundCount; roundNo++)
            {
                run.Cancellation.Token.ThrowIfCancellationRequested();

                var content = PickRoundContent(run);
                var roundId = await CreateRoundAsync(run.RunID, roundNo, content.Prompt, MaskAnswer(content.Answer));
                var round = new ActiveEventRound(
                    roundId,
                    roundNo,
                    content.Prompt,
                    content.Answer,
                    content.NormalizedAnswer,
                    run.Config.RoundDurationSeconds);
                if (run.Config.EventCode.Equals("LuckyParty", StringComparison.OrdinalIgnoreCase))
                    LuckyPartyMatchingRequests.Clear();
                run.CurrentRound = round;

                await BroadcastRoundStartAsync(run, round);

                if (run.Config.EventCode.Equals("LongestOnline", StringComparison.OrdinalIgnoreCase))
                {
                    await DelayRoundAsync(run, round);
                    await AwardLongestOnlineAsync(run, round);
                }
                else if (run.Config.EventCode.Equals("LuckyStaller", StringComparison.OrdinalIgnoreCase))
                {
                    await DelayRoundAsync(run, round);
                    await AwardLuckyStallerAsync(run, round);
                }
                else if (run.Config.EventCode.Equals("LuckyParty", StringComparison.OrdinalIgnoreCase))
                {
                    await DelayRoundAsync(run, round);
                    await AwardParticipationAsync(run, round, ParticipationKind.Party);
                }
                else if (run.Config.EventCode.Equals("LuckyGlobal", StringComparison.OrdinalIgnoreCase))
                {
                    await DelayRoundAsync(run, round);
                    await AwardParticipationAsync(run, round, ParticipationKind.Global);
                }
                else if (run.Config.EventCode.Equals("LuckyStall", StringComparison.OrdinalIgnoreCase))
                {
                    await DelayRoundAsync(run, round);
                    await AwardLuckyStallerAsync(run, round);
                }
                else
                {
                    await WaitForRoundWinnerAsync(run, round);
                }

                if (string.IsNullOrWhiteSpace(round.WinnerCharName))
                {
                    await FinalizeRoundAsync(round, null, null, "TimedOut");
                    await BroadcastNoticeAsync(PlayerLanguage.Get("AutoEvent.RoundNoWinner", run.Config.DisplayName, round.RoundNo), NoticeType.WARNING);
                }

                if (roundNo < run.Config.RoundCount)
                    await Task.Delay(TimeSpan.FromSeconds(run.Config.InterRoundDelaySeconds), run.Cancellation.Token);
            }

            run.Cancellation.Token.ThrowIfCancellationRequested();
            await FinishRunAsync(run, "Completed", "All rounds completed.");
            await TryBroadcastNoticeAsync(PlayerLanguage.Get("AutoEvent.Completed", run.Config.DisplayName), NoticeType.NOTICE);
        }
        catch (OperationCanceledException)
        {
            await FinalizeCurrentRoundAsync(run, "Stopped");
            var reason = run.RequestedStopReason ?? "Cancelled.";
            await FinishRunAsync(run, "Stopped", reason);
            await TryBroadcastNoticeAsync(
                PlayerLanguage.Get("AutoEvent.Stopped", run.Config.DisplayName, FormatStopReason(reason)),
                NoticeType.WARNING);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "AutoEvent run failed: {EventCode}", run.Config.EventCode);
            await FinalizeCurrentRoundAsync(run, "Failed");
            await FinishRunAsync(run, "Failed", ex.Message);
        }
        finally
        {
            if (run.Config.EventCode.Equals("LuckyParty", StringComparison.OrdinalIgnoreCase))
                LuckyPartyMatchingRequests.Clear();

            await StateLock.WaitAsync();
            try
            {
                if (ReferenceEquals(_activeRun, run))
                    _activeRun = null;
            }
            finally
            {
                StateLock.Release();
            }
            run.Completion.TrySetResult();
        }
    }

    private static async Task SendStartCountdownAsync(ActiveEventRun run)
    {
        var delay = Math.Max(0, run.Config.StartDelaySeconds);
        if (delay <= 0)
        {
            await TelegramNotificationService.QueueEventStartedAsync(
                run.RunID,
                run.Config.EventCode,
                run.Config.DisplayName);
            return;
        }

        await BroadcastNoticeAsync(
            PlayerLanguage.Get("AutoEvent.StartsIn", run.Config.DisplayName, PlayerLanguage.FormatDurationSeconds(delay)),
            NoticeType.NOTICE);

        if (delay > 30)
        {
            await Task.Delay(TimeSpan.FromSeconds(delay - 30), run.Cancellation.Token);
            await BroadcastNoticeAsync(PlayerLanguage.Get("AutoEvent.StartsInSeconds", run.Config.DisplayName, 30), NoticeType.NOTICE);
            delay = 30;
        }

        if (delay > 10)
        {
            await Task.Delay(TimeSpan.FromSeconds(delay - 10), run.Cancellation.Token);
            await BroadcastNoticeAsync(PlayerLanguage.Get("AutoEvent.StartsInSeconds", run.Config.DisplayName, 10), NoticeType.NOTICE);
            delay = 10;
        }

        await Task.Delay(TimeSpan.FromSeconds(delay), run.Cancellation.Token);
        await TelegramNotificationService.QueueEventStartedAsync(
            run.RunID,
            run.Config.EventCode,
            run.Config.DisplayName);
    }

    private static async Task TryWinRoundAsync(
        ActiveEventRun run,
        ActiveEventRound round,
        ISession session,
        string rawAnswer,
        string normalizedAnswer,
        DateTime? occurredAtUtc = null)
    {
        var claimAtUtc = occurredAtUtc ?? DateTime.UtcNow;
        await round.WinnerLock.WaitAsync();

        try
        {
            if (round.HasWinner ||
                run.Cancellation.IsCancellationRequested ||
                Volatile.Read(ref round.Finalized) != 0 ||
                !ReferenceEquals(run.CurrentRound, round) ||
                !IsWithinRoundWindow(round.StartedAtUtc, round.EndsAtUtc, claimAtUtc))
                return;

            if (!IsEligible(run, session))
                return;

            if (!IsAnswerCorrect(run.Config.EventCode, round, rawAnswer, normalizedAnswer))
                return;

            var inserted = await InsertWinnerLogAsync(run, round, session, rawAnswer);
            if (!inserted)
            {
                // Another worker already persisted the only valid winner claim.
                // Stop accepting answers without issuing a duplicate reward.
                round.HasWinner = true;
                return;
            }

            RegisterWinnerFingerprint(run, session);
            round.HasWinner = true;
            round.WinnerCharID = session.SessionData.Charid;
            round.WinnerCharName = session.SessionData.Charname;

            var summary = await ApplyRewardsAsync(run, round, session);
            try
            {
                await UpdateWinnerRewardSummaryAsync(round.RoundID, session.SessionData.Charid, summary);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "AutoEvent reward summary update failed for round {RoundID}", round.RoundID);
            }
            await FinalizeRoundAsync(round, session.SessionData.Charid, session.SessionData.Charname, "Won");
            await BroadcastNoticeAsync(
                PlayerLanguage.Get("AutoEvent.RoundWinner", session.SessionData.Charname, run.Config.DisplayName, round.RoundNo),
                NoticeType.NOTICE);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "AutoEvent winner handling failed");
            if (round.HasWinner)
            {
                try
                {
                    await FinalizeRoundAsync(round, round.WinnerCharID, round.WinnerCharName, "Failed");
                }
                catch (Exception finalizeEx)
                {
                    Log.Warning(finalizeEx, "AutoEvent failed winner round could not be finalized: {RoundID}", round.RoundID);
                }
            }
        }
        finally
        {
            round.WinnerLock.Release();
        }
    }

    private static bool IsAnswerCorrect(
        string eventCode,
        ActiveEventRound round,
        string rawAnswer,
        string normalizedAnswer)
    {
        return IsConfiguredAnswerCorrect(eventCode, round.Answer, rawAnswer, normalizedAnswer);
    }

    internal static bool IsConfiguredAnswerCorrect(
        string eventCode,
        string expectedAnswer,
        string rawAnswer,
        string? normalizedAnswer = null)
    {
        if (eventCode.Equals("Retype", StringComparison.OrdinalIgnoreCase))
            return string.Equals(rawAnswer, expectedAnswer, StringComparison.Ordinal);

        normalizedAnswer ??= NormalizeAnswer(rawAnswer);
        return string.Equals(normalizedAnswer, NormalizeAnswer(expectedAnswer), StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsEligible(ActiveEventRun run, ISession session)
    {
        if (!IsBasicEligible(run, session))
            return false;

        var now = DateTime.UtcNow;
        if (run.LastAnswerUtc.TryGetValue(session.SessionData.Charid, out var last) &&
            (now - last).TotalMilliseconds < run.Config.AnswerCooldownMs)
        {
            return false;
        }

        run.LastAnswerUtc[session.SessionData.Charid] = now;

        if (!run.Config.UniqueWinnerPerRun)
            return true;

        var hwid = session.SessionData.Hwid ?? string.Empty;
        var ip = session.ClientIp ?? string.Empty;
        return !run.WinnerCharIds.ContainsKey(session.SessionData.Charid) &&
               (hwid.Length == 0 || !run.WinnerHwids.ContainsKey(hwid)) &&
               (ip.Length == 0 || !run.WinnerIps.ContainsKey(ip));
    }

    private static bool IsBasicEligible(ActiveEventRun run, ISession session)
    {
        if (!ServerManager.IsOnlinePlayer(session))
            return false;

        if (run.Config.MinLevel > 0 && session.SessionData.CurLevel < run.Config.MinLevel)
            return false;

        if (run.Config.RequireHwid && string.IsNullOrWhiteSpace(session.SessionData.Hwid))
            return false;

        if (run.Config.HwidLimit > 0 && !IsWithinHwidLimit(session, run.Config.HwidLimit))
            return false;

        if (!run.Config.UniqueWinnerPerRun)
            return true;

        var hwid = session.SessionData.Hwid ?? string.Empty;
        var ip = session.ClientIp ?? string.Empty;
        return !run.WinnerCharIds.ContainsKey(session.SessionData.Charid) &&
               (hwid.Length == 0 || !run.WinnerHwids.ContainsKey(hwid)) &&
               (ip.Length == 0 || !run.WinnerIps.ContainsKey(ip));
    }

    private static void RegisterWinnerFingerprint(ActiveEventRun run, ISession session)
    {
        if (!run.Config.UniqueWinnerPerRun)
            return;

        run.WinnerCharIds.TryAdd(session.SessionData.Charid, 0);
        if (!string.IsNullOrWhiteSpace(session.SessionData.Hwid))
            run.WinnerHwids.TryAdd(session.SessionData.Hwid, 0);
        if (!string.IsNullOrWhiteSpace(session.ClientIp))
            run.WinnerIps.TryAdd(session.ClientIp, 0);
    }

    private static bool IsWithinHwidLimit(ISession session, int limit)
    {
        var hwid = session.SessionData.Hwid;
        if (string.IsNullOrWhiteSpace(hwid))
            return false;

        var count = ServerManager.AgentSessions.CountWhere(s =>
            s.CharacterGameReady &&
            !s.IsStopped &&
            !s.ClientDetached &&
            s.SessionData.Charid > 0 &&
            string.Equals(s.SessionData.Hwid, hwid, StringComparison.OrdinalIgnoreCase));

        return count <= limit;
    }

    private static async Task AwardLongestOnlineAsync(ActiveEventRun run, ActiveEventRound round)
    {
        var winner = ServerManager.AgentSessions
            .Where(s => IsBasicEligible(run, s))
            .OrderBy(s => s.SessionData.CharacterReadyAtUtc)
            .ThenBy(s => s.SessionData.Charid)
            .FirstOrDefault();

        if (winner != null)
            await TryWinRoundAsync(run, round, winner, "online-time", round.NormalizedAnswer, round.EndsAtUtc);
    }

    private static async Task AwardLuckyStallerAsync(ActiveEventRun run, ActiveEventRound round)
    {
        var tracked = GetParticipationEntries(run, ParticipationKind.Stall, round.StartedAtUtc)
            .OrderBy(_ => Random.Shared.Next())
            .ToList();

        foreach (var entry in tracked)
        {
            var trackedSession = FindSessionByCharId(entry.CharID);
            if (trackedSession == null)
                continue;

            await TryWinRoundAsync(run, round, trackedSession, entry.Evidence, round.NormalizedAnswer, entry.CreatedAtUtc);
            if (round.HasWinner)
                return;
        }

        var openStalls = ActionManager.OpenStalls.Values
            .Where(stall => stall.OpenedAtUtc >= round.StartedAtUtc && stall.OpenedAtUtc <= DateTime.UtcNow)
            .OrderBy(_ => Random.Shared.Next())
            .ToList();

        foreach (var stall in openStalls)
        {
            var session = FindSessionByCharId(stall.CharId);
            if (session == null)
                continue;

            await TryWinRoundAsync(run, round, session, "stall-open", round.NormalizedAnswer, stall.OpenedAtUtc);
            if (round.HasWinner)
                break;
        }
    }

    private static async Task AwardParticipationAsync(ActiveEventRun run, ActiveEventRound round, ParticipationKind kind)
    {
        var entries = GetParticipationEntries(run, kind, round.StartedAtUtc)
            .OrderBy(_ => Random.Shared.Next())
            .ToList();

        foreach (var entry in entries)
        {
            var session = FindSessionByCharId(entry.CharID);
            if (session == null)
                continue;

            await TryWinRoundAsync(run, round, session, entry.Evidence, round.NormalizedAnswer, entry.CreatedAtUtc);
            if (round.HasWinner)
                break;
        }
    }

    private static IEnumerable<ParticipationEntry> GetParticipationEntries(
        ActiveEventRun run,
        ParticipationKind kind,
        DateTime startedAtUtc)
    {
        var source = kind switch
        {
            ParticipationKind.Party => run.LuckyPartyEntries.Values,
            ParticipationKind.Global => run.LuckyGlobalEntries.Values,
            ParticipationKind.Stall => run.LuckyStallEntries.Values,
            _ => Enumerable.Empty<ParticipationEntry>()
        };

        return source.Where(x => x.CreatedAtUtc >= startedAtUtc);
    }

    private static void RegisterParticipation(
        ActiveEventRun run,
        ActiveEventRound round,
        ParticipationKind kind,
        ISession session,
        string value,
        string evidence)
    {
        var occurredAtUtc = DateTime.UtcNow;
        if (!IsBasicEligible(run, session) ||
            Volatile.Read(ref round.Finalized) != 0 ||
            !ReferenceEquals(run.CurrentRound, round) ||
            !IsWithinRoundWindow(round.StartedAtUtc, round.EndsAtUtc, occurredAtUtc))
            return;

        var entry = new ParticipationEntry(
            session.SessionData.Charid,
            session.SessionData.Charname,
            session.SessionData.JID,
            session.SessionData.Hwid,
            session.ClientIp,
            value,
            evidence,
            occurredAtUtc);

        var target = kind switch
        {
            ParticipationKind.Party => run.LuckyPartyEntries,
            ParticipationKind.Global => run.LuckyGlobalEntries,
            ParticipationKind.Stall => run.LuckyStallEntries,
            _ => run.LuckyGlobalEntries
        };

        target[session.SessionData.Charid] = entry;
    }

    private static ISession? FindSessionByCharId(int charId)
    {
        return ServerManager.AgentSessions.FindByCharId(
            charId,
            static session => session.CharacterGameReady && !session.IsStopped && !session.ClientDetached);
    }

    private static async Task WaitForRoundWinnerAsync(ActiveEventRun run, ActiveEventRound round)
    {
        var until = round.EndsAtUtc;
        while (!round.HasWinner && DateTime.UtcNow < until)
        {
            run.Cancellation.Token.ThrowIfCancellationRequested();
            await Task.Delay(250, run.Cancellation.Token);
        }

        // HasWinner becomes visible as soon as the durable claim succeeds. Wait
        // for the same lock so rewards and the round audit finish before moving on.
        await round.WinnerLock.WaitAsync(run.Cancellation.Token);
        round.WinnerLock.Release();
    }

    private static async Task FinalizeCurrentRoundAsync(ActiveEventRun run, string status)
    {
        var round = run.CurrentRound;
        if (round == null)
            return;

        await round.WinnerLock.WaitAsync();
        try
        {
            await FinalizeRoundAsync(round, round.WinnerCharID > 0 ? round.WinnerCharID : null,
                string.IsNullOrWhiteSpace(round.WinnerCharName) ? null : round.WinnerCharName, status);
        }
        finally
        {
            round.WinnerLock.Release();
        }
    }

    private static async Task FinalizeRoundAsync(
        ActiveEventRound round,
        int? winnerCharId,
        string? winnerName,
        string status)
    {
        if (Interlocked.CompareExchange(ref round.Finalized, 1, 0) != 0)
            return;

        try
        {
            await CompleteRoundAsync(round.RoundID, winnerCharId, winnerName, status);
        }
        catch
        {
            Interlocked.Exchange(ref round.Finalized, 0);
            throw;
        }
    }

    private static Task DelayRoundAsync(ActiveEventRun run, ActiveEventRound round)
    {
        var remaining = round.EndsAtUtc - DateTime.UtcNow;
        return remaining > TimeSpan.Zero
            ? Task.Delay(remaining, run.Cancellation.Token)
            : Task.CompletedTask;
    }

    private static async Task BroadcastRoundStartAsync(ActiveEventRun run, ActiveEventRound round)
    {
        var message = run.Config.EventCode switch
        {
            "LongestOnline" => PlayerLanguage.Get("AutoEvent.Round.LongestOnline", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, run.Config.RoundDurationSeconds),
            "LuckyStaller" => PlayerLanguage.Get("AutoEvent.Round.LuckyStaller", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, run.Config.RoundDurationSeconds),
            "LuckyStall" => PlayerLanguage.Get("AutoEvent.Round.LuckyStall", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, run.Config.RoundDurationSeconds),
            "LuckyParty" => PlayerLanguage.Get("AutoEvent.Round.LuckyParty", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, run.Config.RoundDurationSeconds),
            "LuckyGlobal" => PlayerLanguage.Get("AutoEvent.Round.LuckyGlobal", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, run.Config.RoundDurationSeconds),
            "Alchemy" => PlayerLanguage.Get("AutoEvent.Round.Alchemy", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, round.Answer),
            _ => PlayerLanguage.Get("AutoEvent.Round.Prompt", run.Config.DisplayName, round.RoundNo, run.Config.RoundCount, round.Prompt)
        };

        await BroadcastNoticeAsync(message, NoticeType.NOTICE);
    }

    private static RoundContent PickRoundContent(ActiveEventRun run)
    {
        var config = run.Config;

        if (config.EventCode.Equals("Math", StringComparison.OrdinalIgnoreCase))
            return GenerateMath(run);

        if (config.EventCode.Equals("LongestOnline", StringComparison.OrdinalIgnoreCase))
            return new RoundContent("Longest online wins.", "online-time");

        if (IsLuckyStallEvent(config.EventCode))
            return new RoundContent("Open a stall and keep it open.", "stall-open");

        if (config.EventCode.Equals("LuckyParty", StringComparison.OrdinalIgnoreCase))
            return new RoundContent("Register a Party Matching form.", "lucky-party-matching");

        if (config.EventCode.Equals("LuckyGlobal", StringComparison.OrdinalIgnoreCase))
            return new RoundContent("Send a global.", "lucky-global");

        if (config.EventCode.Equals("Alchemy", StringComparison.OrdinalIgnoreCase))
        {
            var target = ResolveAlchemyTargetPlus(config.AlchemyTargetPlus);
            return new RoundContent(PlayerLanguage.Get("AutoEvent.Prompt.Alchemy", target), target.ToString());
        }

        var list = run.Content;
        if (list.Count > 0)
        {
            var unused = list
                .Where(x => !run.UsedContentIds.Contains(x.ContentID) &&
                            !run.UsedAnswers.Contains(NormalizeAnswer(x.Answer)))
                .ToList();

            if (unused.Count == 0 &&
                (config.EventCode.Equals("Retype", StringComparison.OrdinalIgnoreCase) ||
                 config.EventCode.Equals("FirstType", StringComparison.OrdinalIgnoreCase)))
            {
                var generated = GenerateUniqueToken(run, 8);
                return new RoundContent(PlayerLanguage.Get("AutoEvent.Prompt.TypeExactly", generated), generated);
            }

            var selectable = unused.Count > 0 ? unused : list;
            var totalWeight = selectable.Sum(x => Math.Max(1, x.Weight));
            var pick = Random.Shared.Next(1, totalWeight + 1);
            foreach (var item in selectable)
            {
                pick -= Math.Max(1, item.Weight);
                if (pick <= 0)
                {
                    run.UsedContentIds.Add(item.ContentID);
                    run.UsedAnswers.Add(NormalizeAnswer(item.Answer));
                    return new RoundContent(item.Prompt, item.Answer);
                }
            }
        }

        if (config.EventCode.Equals("Trivia", StringComparison.OrdinalIgnoreCase))
            return new RoundContent(PlayerLanguage.Get("AutoEvent.Prompt.ServerName"), _serverSettings.ServerName);

        var token = GenerateUniqueToken(run, 8);
        return new RoundContent(PlayerLanguage.Get("AutoEvent.Prompt.TypeExactly", token), token);
    }

    private static RoundContent GenerateMath(ActiveEventRun run)
    {
        for (var i = 0; i < 50; i++)
        {
            var a = Random.Shared.Next(5, 50);
            var b = Random.Shared.Next(2, 25);
            var op = Random.Shared.Next(0, 3);
            var content = op switch
            {
                0 => new RoundContent($"{a} + {b} = ?", (a + b).ToString()),
                1 => new RoundContent($"{a} - {b} = ?", (a - b).ToString()),
                _ => new RoundContent($"{a} x {b} = ?", (a * b).ToString())
            };

            var key = NormalizeAnswer(content.Prompt);
            if (run.UsedAnswers.Add(key))
                return content;
        }

        var fallback = new RoundContent($"{Random.Shared.Next(50, 99)} + {Random.Shared.Next(50, 99)} = ?", "0");
        return new RoundContent(fallback.Prompt, EvaluateSimpleAddition(fallback.Prompt).ToString());
    }

    private static int EvaluateSimpleAddition(string prompt)
    {
        var parts = prompt.Replace("= ?", string.Empty).Split('+', StringSplitOptions.TrimEntries);
        return int.Parse(parts[0]) + int.Parse(parts[1]);
    }

    private static string GenerateToken(int length)
    {
        const string chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        var builder = new StringBuilder(length);
        for (var i = 0; i < length; i++)
            builder.Append(chars[Random.Shared.Next(chars.Length)]);
        return builder.ToString();
    }

    private static string GenerateUniqueToken(ActiveEventRun run, int length)
    {
        for (var i = 0; i < 50; i++)
        {
            var token = GenerateToken(length);
            if (run.UsedAnswers.Add(NormalizeAnswer(token)))
                return token;
        }

        var fallback = GenerateToken(length) + Random.Shared.Next(10, 99);
        run.UsedAnswers.Add(NormalizeAnswer(fallback));
        return fallback;
    }

    private static async Task ProcessCommandsAsync(CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            try
            {
                if (!_schemaReady)
                    await ReloadAsync();

                if (_schemaReady)
                {
                    await ProcessCommandBatchAsync(token);
                    var nowUtc = DateTime.UtcNow;
                    if (nowUtc >= _nextSchedulePollUtc)
                    {
                        _nextSchedulePollUtc = nowUtc.Add(SchedulePollInterval);
                        await ProcessAutoEventSchedulesAsync(token);
                        await ProcessSurvivalEventSchedulesAsync("_SurvivalPartySchedule", "SPARTY", "Survival Party", token);
                        await ProcessSurvivalEventSchedulesAsync("_SurvivalSoloSchedule", "SSOLO", "Survival Solo", token);
                        await ProcessCompetitiveEventSchedulesAsync("LMS", "Last Man Standing", token);
                        await ProcessCompetitiveEventSchedulesAsync("MADNESS", "Madness Solo", token);
                        await ProcessCompetitiveEventSchedulesAsync("DTT", "Defend The Tower", token);
                        await ProcessSurvivalEventSchedulesAsync("_HideAndSeekSchedule", "HNS", "Hide and Seek", token);
                    }

                    await HideAndSeekEventService.MaintainBotAsync(token);
                }
            }
            catch (OperationCanceledException)
            {
                return;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "AutoEvent command queue failed");
            }

            await Task.Delay(2000, token);
        }
    }

    private static async Task ProcessAutoEventSchedulesAsync(CancellationToken token)
    {
        var now = DateTime.Now;
        var scheduleTable = EventTable("_AutoEventSchedule");

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        var schedules = (await connection.QueryAsync<AutoEventSchedule>(
            new CommandDefinition(
                $@"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
                   SELECT ScheduleID, EventCode, StartTime, DaysMask,
                          ISNULL(RepeatMinutes, 0) AS RepeatMinutes,
                          LastRunLocalDate, LastRunAtLocal
                   FROM {scheduleTable} WITH (READPAST, READCOMMITTEDLOCK)
                   WHERE IsActive = 1
                     AND EventCode NOT IN (N'SPARTY', N'SSOLO', N'HNS', N'LMS', N'MADNESS', N'DTT')
                   ORDER BY StartTime, ScheduleID",
                cancellationToken: token))).ToList();

        foreach (var schedule in schedules)
        {
            var displayName = _configs.TryGetValue(schedule.EventCode, out var eventConfig)
                ? eventConfig.DisplayName
                : schedule.EventCode;

            var nextOccurrence = GetNextAutoEventScheduleOccurrence(
                schedule.StartTime,
                schedule.DaysMask,
                schedule.RepeatMinutes,
                now);
            if (nextOccurrence.HasValue)
            {
                await TelegramNotificationService.QueueEventRemindersIfDueAsync(
                    schedule.EventCode,
                    displayName,
                    $"auto-{schedule.ScheduleID}",
                    nextOccurrence.Value,
                    now,
                    token);
            }

            if (!TryGetDueAutoEventScheduleOccurrence(
                    schedule.StartTime,
                    schedule.DaysMask,
                    schedule.RepeatMinutes,
                    schedule.LastRunLocalDate,
                    schedule.LastRunAtLocal,
                    now,
                    out var scheduledAt))
                continue;

            var claimed = await connection.ExecuteAsync(
                new CommandDefinition(
                    $@"UPDATE {scheduleTable}
                       SET LastRunLocalDate = @Today,
                           LastRunAtLocal = @ScheduledAt,
                           UpdatedAtUtc = SYSUTCDATETIME()
                       WHERE ScheduleID = @ScheduleID AND IsActive = 1
                         AND
                         (
                             (@RepeatMinutes = 0 AND (LastRunLocalDate IS NULL OR LastRunLocalDate <> @Today))
                             OR
                             (@RepeatMinutes > 0 AND (LastRunAtLocal IS NULL OR LastRunAtLocal < @ScheduledAt))
                         )",
                    new
                    {
                        Today = scheduledAt.Date,
                        ScheduledAt = scheduledAt,
                        RepeatMinutes = schedule.RepeatMinutes,
                        schedule.ScheduleID
                    },
                    cancellationToken: token));
            if (claimed != 1)
                continue;

            string result;
            try
            {
                result = await StartAsync(schedule.EventCode, $"Schedule #{schedule.ScheduleID}");
                if (ShouldRetryScheduledStart(result))
                    await ResetAutoEventScheduleClaimAsync(connection, scheduleTable, schedule, scheduledAt, token);
            }
            catch
            {
                await ResetAutoEventScheduleClaimAsync(connection, scheduleTable, schedule, scheduledAt, token);
                throw;
            }
            Log.Information("Auto Event {EventCode} schedule {ScheduleID} fired at {LocalTime}: {Result}",
                schedule.EventCode,
                schedule.ScheduleID,
                now,
                result);
        }
    }

    private static async Task ProcessCommandBatchAsync(CancellationToken token)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        var commandQueueTable = EventTable("_AutoEventCommandQueue");
        var nowUtc = DateTime.UtcNow;
        if (nowUtc >= _nextCommandRecoveryUtc)
        {
            _nextCommandRecoveryUtc = nowUtc.Add(CommandRecoveryInterval);
            await connection.ExecuteAsync(new CommandDefinition(
                $@"UPDATE {commandQueueTable}
                   SET Status = 0, ProcessedAtUtc = NULL,
                       Message = N'Recovered after an interrupted worker.'
                   WHERE Status = 1
                     AND ProcessedAtUtc < DATEADD(MINUTE, -5, SYSUTCDATETIME())",
                cancellationToken: token));
        }

        var commands = (await connection.QueryAsync<AutoEventCommand>(
            new CommandDefinition(
                $"SET TRANSACTION ISOLATION LEVEL READ COMMITTED; SELECT TOP (10) * FROM {commandQueueTable} WITH (READPAST, READCOMMITTEDLOCK) WHERE Status = 0 ORDER BY CommandID",
                cancellationToken: token))).ToList();

        foreach (var command in commands)
        {
            var claimed = await connection.ExecuteAsync(
                new CommandDefinition(
                    $@"UPDATE {commandQueueTable}
                      SET Status = 1, ProcessedAtUtc = SYSUTCDATETIME()
                      WHERE CommandID = @CommandID AND Status = 0",
                    new { command.CommandID },
                    cancellationToken: token));
            if (claimed != 1)
                continue;

            string result;
            try
            {
                result = command.CommandType.ToUpperInvariant() switch
                {
                    "START" => await StartAsync(command.EventCode, command.RequestedBy),
                    "STOP" => await StopRequestedEventAsync(command.EventCode, $"Queued by {command.RequestedBy}"),
                    "RELOAD" => await ReloadAndReturnAsync(),
                    _ => $"Unknown command type: {command.CommandType}"
                };

                await connection.ExecuteAsync(
                    new CommandDefinition(
                        $"UPDATE {commandQueueTable} SET Status = 2, Message = @Message WHERE CommandID = @CommandID",
                        new { Message = Trim(result, 512), command.CommandID },
                        cancellationToken: token));
            }
            catch (Exception ex)
            {
                await connection.ExecuteAsync(
                    new CommandDefinition(
                        $"UPDATE {commandQueueTable} SET Status = 3, Message = @Message WHERE CommandID = @CommandID",
                        new { Message = Trim(ex.Message, 512), command.CommandID },
                        cancellationToken: token));
            }
        }
    }

    private static async Task ProcessSurvivalEventSchedulesAsync(
        string scheduleTableName,
        string eventCode,
        string displayName,
        CancellationToken token)
    {
        var now = DateTime.Now;
        var today = now.Date;
        var dayBit = 1 << (int)now.DayOfWeek;
        var scheduleTable = EventTable(scheduleTableName);

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        var schedules = (await connection.QueryAsync<SurvivalPartySchedule>(
            new CommandDefinition(
                $@"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
                   SELECT ScheduleID, StartTime, DaysMask, LastRunLocalDate
                   FROM {scheduleTable} WITH (READPAST, READCOMMITTEDLOCK)
                   WHERE IsActive = 1 AND (LastRunLocalDate IS NULL OR LastRunLocalDate <> @Today)
                   ORDER BY StartTime, ScheduleID",
                new { Today = today },
                cancellationToken: token))).ToList();

        foreach (var schedule in schedules)
        {
            if ((schedule.DaysMask & dayBit) == 0)
                continue;

            var scheduledAt = today.Add(schedule.StartTime);
            await TelegramNotificationService.QueueEventRemindersIfDueAsync(
                eventCode,
                displayName,
                $"{scheduleTableName}-{schedule.ScheduleID}",
                scheduledAt,
                now,
                token);
            // Keep an unclaimed Survival schedule eligible after its exact time so a
            // currently running event delays it instead of consuming the whole day.
            if (now < scheduledAt)
                continue;

            var claimed = await connection.ExecuteAsync(
                new CommandDefinition(
                    $@"UPDATE {scheduleTable}
                       SET LastRunLocalDate = @Today, UpdatedAtUtc = SYSUTCDATETIME()
                       WHERE ScheduleID = @ScheduleID AND IsActive = 1
                         AND (LastRunLocalDate IS NULL OR LastRunLocalDate <> @Today)",
                    new { Today = today, schedule.ScheduleID },
                    cancellationToken: token));
            if (claimed != 1)
                continue;

            string result;
            try
            {
                result = await StartAsync(eventCode, $"Schedule #{schedule.ScheduleID}");
                if (ShouldRetryScheduledStart(result))
                    await ResetScheduleClaimAsync(connection, scheduleTable, schedule.ScheduleID, today, token);
            }
            catch
            {
                await ResetScheduleClaimAsync(connection, scheduleTable, schedule.ScheduleID, today, token);
                throw;
            }
            Log.Information("{EventName} schedule {ScheduleID} fired at {LocalTime}: {Result}",
                displayName,
                schedule.ScheduleID,
                now,
                result);
        }
    }

    internal static bool WasEventStartAccepted(string result, string eventCode)
    {
        return result.StartsWith($"Started {eventCode} ", StringComparison.OrdinalIgnoreCase) ||
               result.StartsWith($"{eventCode} scheduled.", StringComparison.OrdinalIgnoreCase);
    }

    internal static bool ShouldRetryScheduledStart(string result)
    {
        return result.StartsWith("Another event is already running:", StringComparison.OrdinalIgnoreCase) ||
               result.StartsWith("Scheduled event waiting for the 15-minute safety gap", StringComparison.OrdinalIgnoreCase);
    }

    private static Task<int> ResetAutoEventScheduleClaimAsync(
        SqlConnection connection,
        string scheduleTable,
        AutoEventSchedule schedule,
        DateTime claimedOccurrence,
        CancellationToken token)
    {
        return connection.ExecuteAsync(new CommandDefinition(
            $@"UPDATE {scheduleTable}
               SET LastRunLocalDate = @PreviousRunDate,
                   LastRunAtLocal = @PreviousRunAt,
                   UpdatedAtUtc = SYSUTCDATETIME()
               WHERE ScheduleID = @ScheduleID AND LastRunAtLocal = @ClaimedOccurrence",
            new
            {
                PreviousRunDate = schedule.LastRunLocalDate,
                PreviousRunAt = schedule.LastRunAtLocal,
                schedule.ScheduleID,
                ClaimedOccurrence = claimedOccurrence
            },
            cancellationToken: token));
    }

    private static Task<int> ResetScheduleClaimAsync(
        SqlConnection connection,
        string scheduleTable,
        int scheduleId,
        DateTime today,
        CancellationToken token)
    {
        return connection.ExecuteAsync(new CommandDefinition(
            $@"UPDATE {scheduleTable}
               SET LastRunLocalDate = NULL, UpdatedAtUtc = SYSUTCDATETIME()
               WHERE ScheduleID = @ScheduleID AND LastRunLocalDate = @Today",
            new { ScheduleID = scheduleId, Today = today },
            cancellationToken: token));
    }

    private static async Task ProcessCompetitiveEventSchedulesAsync(string eventCode, string displayName, CancellationToken token)
    {
        var now = DateTime.Now;
        var today = now.Date;
        var dayBit = 1 << (int)now.DayOfWeek;
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        var schedules = (await connection.QueryAsync<SurvivalPartySchedule>(new CommandDefinition(@"
SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
SELECT ScheduleID,StartTime,DaysMask,LastRunLocalDate
FROM Events.dbo._CompetitiveEventSchedule WITH(READPAST, READCOMMITTEDLOCK)
WHERE EventCode=@EventCode AND IsActive=1 AND (LastRunLocalDate IS NULL OR LastRunLocalDate<>@Today)
ORDER BY StartTime,ScheduleID;", new { EventCode = eventCode, Today = today }, cancellationToken: token))).ToList();

        foreach (var schedule in schedules)
        {
            if ((schedule.DaysMask & dayBit) == 0)
                continue;
            var scheduledAt = today.Add(schedule.StartTime);
            await TelegramNotificationService.QueueEventRemindersIfDueAsync(
                eventCode,
                displayName,
                $"competitive-{schedule.ScheduleID}",
                scheduledAt,
                now,
                token);
            if (now < scheduledAt)
                continue;
            var claimed = await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE Events.dbo._CompetitiveEventSchedule
SET LastRunLocalDate=@Today,UpdatedAtUtc=SYSUTCDATETIME()
WHERE ScheduleID=@ScheduleID AND EventCode=@EventCode AND IsActive=1
  AND (LastRunLocalDate IS NULL OR LastRunLocalDate<>@Today);",
                new { Today = today, schedule.ScheduleID, EventCode = eventCode }, cancellationToken: token));
            if (claimed != 1)
                continue;
            string result;
            try
            {
                result = await StartAsync(eventCode, $"Schedule #{schedule.ScheduleID}");
                if (ShouldRetryScheduledStart(result))
                    await ResetScheduleClaimAsync(connection, "Events.dbo._CompetitiveEventSchedule", schedule.ScheduleID, today, token);
            }
            catch
            {
                await ResetScheduleClaimAsync(connection, "Events.dbo._CompetitiveEventSchedule", schedule.ScheduleID, today, token);
                throw;
            }
            Log.Information("{EventName} schedule {ScheduleID} fired at {LocalTime}: {Result}", displayName, schedule.ScheduleID, now, result);
        }
    }

    private static async Task<string> ReloadAndReturnAsync()
    {
        return await ReloadAsync()
            ? "Auto Events reloaded."
            : "Auto Events reload failed; the last known-good configuration remains active.";
    }

    private static async Task<string> StopRequestedEventAsync(string eventCode, string reason)
    {
        var requestedCode = NormalizeEventCode(eventCode);
        var activeCode = _activeRun?.Config.EventCode;
        if (requestedCode.Length > 0 && !string.Equals(activeCode, requestedCode, StringComparison.OrdinalIgnoreCase))
            return $"Event {requestedCode} is not running.";

        return await StopAsync(reason);
    }

    private static string FormatStopReason(string? reason)
    {
        var value = (reason ?? string.Empty).Trim();
        if (string.Equals(value, "Console", StringComparison.OrdinalIgnoreCase))
            return PlayerLanguage.Get("AutoEvent.StopReason.Console");

        const string queuedByPrefix = "Queued by ";
        if (value.StartsWith(queuedByPrefix, StringComparison.OrdinalIgnoreCase))
            return PlayerLanguage.Get("AutoEvent.StopReason.QueuedBy", value[queuedByPrefix.Length..].Trim());

        return value;
    }

    private static async Task<long> CreateRunAsync(string eventCode, string startedBy)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var runTable = EventTable("_AutoEventRun");
        return await connection.ExecuteScalarAsync<long>($@"
INSERT INTO {runTable} (EventCode, Status, StartedAtUtc, StartedBy)
VALUES (@EventCode, N'Running', SYSUTCDATETIME(), @StartedBy);
SELECT CONVERT(bigint, SCOPE_IDENTITY());",
            new { EventCode = eventCode, StartedBy = Trim(startedBy, 64) });
    }

    private static async Task<DateTime?> LoadMostRecentFinishedRunUtcAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        return await connection.ExecuteScalarAsync<DateTime?>(
            $@"SELECT TOP (1) FinishedAtUtc
               FROM {EventTable("_AutoEventRun")} WITH (NOLOCK)
               WHERE FinishedAtUtc IS NOT NULL
               ORDER BY FinishedAtUtc DESC;");
    }

    private static async Task<long> CreateRoundAsync(long runId, int roundNo, string prompt, string answerMasked)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var roundTable = EventTable("_AutoEventRound");
        return await connection.ExecuteScalarAsync<long>($@"
INSERT INTO {roundTable} (RunID, RoundNo, Prompt, AnswerMasked, Status, StartedAtUtc)
VALUES (@RunID, @RoundNo, @Prompt, @AnswerMasked, N'Running', SYSUTCDATETIME());
SELECT CONVERT(bigint, SCOPE_IDENTITY());",
            new { RunID = runId, RoundNo = roundNo, Prompt = Trim(prompt, 512), AnswerMasked = answerMasked });
    }

    private static async Task CompleteRoundAsync(long roundId, int? winnerCharId, string? winnerName, string status)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var roundTable = EventTable("_AutoEventRound");
        await connection.ExecuteAsync($@"
UPDATE {roundTable}
SET Status = @Status,
    WinnerCharID = @WinnerCharID,
    WinnerCharName = @WinnerCharName,
    EndedAtUtc = SYSUTCDATETIME()
WHERE RoundID = @RoundID AND EndedAtUtc IS NULL",
            new
            {
                RoundID = roundId,
                Status = status,
                WinnerCharID = winnerCharId,
                WinnerCharName = Trim(winnerName ?? string.Empty, 64)
            });
    }

    private static async Task FinishRunAsync(ActiveEventRun run, string status, string message)
    {
        if (Interlocked.CompareExchange(ref run.Finalized, 1, 0) != 0)
            return;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var runTable = EventTable("_AutoEventRun");
            await connection.ExecuteAsync($@"
UPDATE {runTable}
SET Status = @Status,
    FinishedAtUtc = COALESCE(FinishedAtUtc, SYSUTCDATETIME()),
    Message = @Message
 WHERE RunID = @RunID AND FinishedAtUtc IS NULL",
                new { run.RunID, Status = status, Message = Trim(message, 512) });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "AutoEvent finish run failed");
        }

        try
        {
            await TelegramNotificationService.QueueEventFinishedAsync(
                run.RunID,
                run.Config.EventCode,
                run.Config.DisplayName,
                status);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "AutoEvent Telegram finish notification failed for run {RunID}", run.RunID);
        }
    }

    private static async Task<bool> InsertWinnerLogAsync(
        ActiveEventRun run,
        ActiveEventRound round,
        ISession session,
        string answer)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = await connection.BeginTransactionAsync();
        var winnerLogTable = EventTable("_AutoEventWinnerLog");
        var inserted = await connection.ExecuteScalarAsync<int>($@"
IF NOT EXISTS (SELECT 1 FROM {winnerLogTable} WITH (UPDLOCK, HOLDLOCK) WHERE RoundID = @RoundID)
BEGIN
    INSERT INTO {winnerLogTable}
        (RunID, RoundID, EventCode, RoundNo, CharID, CharName, JID, Hwid, ClientIP, Answer, WonAtUtc, RewardSummary)
    VALUES
        (@RunID, @RoundID, @EventCode, @RoundNo, @CharID, @CharName, @JID, @Hwid, @ClientIP, @Answer, SYSUTCDATETIME(), N'Pending');
    SELECT 1;
END
ELSE
    SELECT 0;",
            new
            {
                run.RunID,
                round.RoundID,
                run.Config.EventCode,
                round.RoundNo,
                CharID = session.SessionData.Charid,
                CharName = Trim(session.SessionData.Charname, 64),
                session.SessionData.JID,
                Hwid = Trim(session.SessionData.Hwid, 128),
                ClientIP = Trim(session.ClientIp, 64),
                Answer = Trim(answer, 256)
            }, transaction);
        if (inserted == 1)
        {
            foreach (var reward in run.Rewards.Where(value => value.Placement == 1))
            {
                string rewardValue = string.Join("|",
                    reward.Amount,
                    reward.ItemCodeName128 ?? string.Empty,
                    reward.ItemID.GetValueOrDefault(),
                    reward.ItemCount,
                    reward.Plus);
                await connection.ExecuteAsync(@"
INSERT INTO Events.dbo.EventRewardOutbox
    (RunID, RoundID, CharID, RewardID, RewardType, RewardValue, Status, Attempts, CreatedDate)
VALUES
    (@RunID, @RoundID, @CharID, @RewardID, @RewardType, @RewardValue, N'Pending', 0, SYSUTCDATETIME());",
                    new
                    {
                        run.RunID,
                        round.RoundID,
                        CharID = session.SessionData.Charid,
                        reward.RewardID,
                        reward.RewardType,
                        RewardValue = rewardValue
                    },
                    transaction);
            }
        }

        await transaction.CommitAsync();
        return inserted == 1;
    }

    private static async Task UpdateWinnerRewardSummaryAsync(long roundId, int charId, string summary)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var winnerLogTable = EventTable("_AutoEventWinnerLog");
        await connection.ExecuteAsync($@"
UPDATE {winnerLogTable}
SET RewardSummary = @Summary
WHERE RoundID = @RoundID AND CharID = @CharID",
            new { RoundID = roundId, CharID = charId, Summary = Trim(summary, 512) });
    }

    private static async Task<string> ApplyRewardsAsync(
        ActiveEventRun run,
        ActiveEventRound round,
        ISession session)
    {
        var rewards = run.Rewards;
        if (rewards.Count == 0)
            return PlayerLanguage.Get("Common.NoRewardConfigured");

        var summaries = new List<string>();
        foreach (var reward in rewards.Where(x => x.Placement == 1))
        {
            try
            {
                await UpdateRewardOutboxAsync(run.RunID, round.RoundID, session.SessionData.Charid, reward.RewardID, "Processing");
                summaries.Add(await ApplyRewardAsync(session, reward));
                await UpdateRewardOutboxAsync(run.RunID, round.RoundID, session.SessionData.Charid, reward.RewardID, "Completed");
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "AutoEvent reward delivery failed for {EventCode}, CharID {CharID}, RewardID {RewardID}",
                    run.Config.EventCode,
                    session.SessionData.Charid,
                    reward.RewardID);
                await UpdateRewardOutboxAsync(run.RunID, round.RoundID, session.SessionData.Charid, reward.RewardID, "Pending");
                summaries.Add(PlayerLanguage.Get("Reward.DeliveryFailed"));
            }
        }

        return string.Join(", ", summaries.Where(x => !string.IsNullOrWhiteSpace(x)));
    }

    private static async Task UpdateRewardOutboxAsync(
        long runId,
        long roundId,
        int charId,
        int rewardId,
        string status)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE Events.dbo.EventRewardOutbox
SET Status = @Status,
    Attempts = CASE WHEN @Status = N'Processing' THEN Attempts + 1 ELSE Attempts END,
    CompletedDate = CASE WHEN @Status = N'Completed' THEN SYSUTCDATETIME() ELSE NULL END
WHERE RunID = @RunID AND RoundID = @RoundID AND CharID = @CharID AND RewardID = @RewardID;",
            new { RunID = runId, RoundID = roundId, CharID = charId, RewardID = rewardId, Status = status },
            commandTimeout: SqlExecutionPolicy.EventSeconds));
    }

    private static Task<string> ApplyRewardAsync(ISession session, AutoEventReward reward)
    {
        return ApplyRewardCoreAsync(
            session,
            reward.RewardType,
            reward.Amount,
            reward.ItemCodeName128,
            reward.ItemID,
            reward.ItemCount,
            reward.Plus,
            "AutoEvent");
    }

    internal static async Task<string> ApplyRewardCoreAsync(
        ISession session,
        string rewardType,
        long amount,
        string? itemCodeName,
        int? itemId,
        int itemCount,
        int plus,
        string source)
    {
        rewardType = rewardType.Trim();
        if (rewardType.Equals("ItemChest", StringComparison.OrdinalIgnoreCase))
        {
            if (!string.IsNullOrWhiteSpace(itemCodeName))
            {
                var quantity = Math.Clamp(itemCount, 1, 10000);
                bool added = await sqlQueryHelper.AddItemToChest(
                    session.SessionData.Charid,
                    itemCodeName,
                    quantity,
                    source,
                    Math.Clamp(plus, 0, 20));
                return added
                    ? PlayerLanguage.Get("Reward.Item", itemCodeName, quantity)
                    : PlayerLanguage.Get("Reward.ItemFailed", itemCodeName, quantity);
            }

            if (itemId.GetValueOrDefault() > 0)
            {
                var concreteItemId = itemId.GetValueOrDefault();
                var quantity = Math.Clamp(itemCount, 1, 10000);
                bool added = await sqlQueryHelper.AddItemToChest2(
                    session.SessionData.Charid,
                    concreteItemId,
                    quantity,
                    source,
                    Math.Clamp(plus, 0, 20));
                return added
                    ? PlayerLanguage.Get("Reward.ItemId", concreteItemId, quantity)
                    : PlayerLanguage.Get("Reward.ItemIdFailed", concreteItemId, quantity);
            }

            return PlayerLanguage.Get("Reward.InvalidItem");
        }

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        if (rewardType.Equals("Gold", StringComparison.OrdinalIgnoreCase))
        {
            if (amount <= 0)
                return PlayerLanguage.Get("Reward.InvalidAmount");

            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            var updated = await connection.ExecuteAsync(
                $@"UPDATE {shardDb}.._Char
                   SET RemainGold = RemainGold + @Amount
                   WHERE CharID = @CharID",
                new { Amount = amount, CharID = session.SessionData.Charid });
            return updated == 1
                ? PlayerLanguage.Get("Reward.Gold", amount)
                : PlayerLanguage.Get("Reward.DeliveryFailed");
        }

        var silkColumn = rewardType.ToUpperInvariant() switch
        {
            "SILKOWN" => "silk_own",
            "SILKGIFT" => "silk_gift",
            "SILKPOINT" => "silk_point",
            _ => string.Empty
        };

        if (silkColumn.Length > 0)
        {
            if (amount <= 0 || session.SessionData.JID <= 0)
                return PlayerLanguage.Get("Reward.InvalidAmount");

            var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
            var effectiveAmount = (int)Math.Min(amount, int.MaxValue);
            await connection.ExecuteAsync($@"
IF NOT EXISTS (SELECT 1 FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK) WHERE JID = @JID)
    INSERT INTO {accountDb}..SK_Silk (JID, silk_own, silk_gift, silk_point)
    VALUES (@JID, 0, 0, 0);

UPDATE {accountDb}..SK_Silk
SET {silkColumn} = {silkColumn} + @Amount
WHERE JID = @JID",
                new { session.SessionData.JID, Amount = effectiveAmount });
            var rewardName = rewardType.ToUpperInvariant() switch
            {
                "SILKOWN" => PlayerLanguage.Get("Reward.SilkOwn"),
                "SILKGIFT" => PlayerLanguage.Get("Reward.SilkGift"),
                "SILKPOINT" => PlayerLanguage.Get("Reward.SilkPoint"),
                _ => rewardType
            };
            return PlayerLanguage.Get("Reward.Currency", effectiveAmount, rewardName);
        }

        return PlayerLanguage.Get("Reward.UnsupportedType", rewardType);
    }

    private static async Task BroadcastNoticeAsync(string message, NoticeType type)
    {
        var packet = new Packet(0x168A);
        packet.WriteUInt8(type);
        packet.WriteUnicode(PlayerLanguage.Get("AutoEvent.Prefix") + message);
        await ServerManager.BroadcastPacket(packet);
    }

    private static async Task TryBroadcastNoticeAsync(string message, NoticeType type)
    {
        try
        {
            await BroadcastNoticeAsync(message, type);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "AutoEvent terminal notice broadcast failed");
        }
    }

    internal static bool TryRecordDeath(
        ConcurrentDictionary<int, DateTime> recentDeaths,
        int charId,
        DateTime nowUtc,
        TimeSpan duplicateWindow)
    {
        while (true)
        {
            if (!recentDeaths.TryGetValue(charId, out var previous))
            {
                if (recentDeaths.TryAdd(charId, nowUtc))
                    return true;
                continue;
            }

            if (nowUtc - previous < duplicateWindow)
                return false;

            if (recentDeaths.TryUpdate(charId, nowUtc, previous))
                return true;
        }
    }

    private static bool IsCurrentEvent(string eventCode)
    {
        return _activeRun?.Config.EventCode.Equals(eventCode, StringComparison.OrdinalIgnoreCase) == true;
    }

    private static bool IsLuckyStallEvent(string eventCode)
    {
        return eventCode.Equals("LuckyStall", StringComparison.OrdinalIgnoreCase) ||
               eventCode.Equals("LuckyStaller", StringComparison.OrdinalIgnoreCase);
    }

    internal static bool TryGetDueAutoEventScheduleOccurrence(
        TimeSpan startTime,
        int daysMask,
        int repeatMinutes,
        DateTime? lastRunLocalDate,
        DateTime? lastRunAtLocal,
        DateTime nowLocal,
        out DateTime occurrence)
    {
        occurrence = default;
        repeatMinutes = repeatMinutes is >= 30 and <= 1440 ? repeatMinutes : 0;
        var today = nowLocal.Date;
        if ((daysMask & (1 << (int)nowLocal.DayOfWeek)) == 0)
            return false;

        var firstOccurrence = today.Add(startTime);
        if (nowLocal < firstOccurrence)
            return false;

        if (repeatMinutes <= 0)
        {
            if (lastRunLocalDate?.Date == today)
                return false;

            occurrence = firstOccurrence;
            return true;
        }

        var elapsedMinutes = (int)Math.Floor((nowLocal - firstOccurrence).TotalMinutes);
        occurrence = firstOccurrence.AddMinutes((elapsedMinutes / repeatMinutes) * repeatMinutes);
        if (occurrence.Date != today || lastRunAtLocal >= occurrence)
        {
            occurrence = default;
            return false;
        }

        return true;
    }

    internal static DateTime? GetNextAutoEventScheduleOccurrence(
        TimeSpan startTime,
        int daysMask,
        int repeatMinutes,
        DateTime nowLocal)
    {
        repeatMinutes = repeatMinutes is >= 30 and <= 1440 ? repeatMinutes : 0;
        for (var dayOffset = 0; dayOffset <= 7; dayOffset++)
        {
            var date = nowLocal.Date.AddDays(dayOffset);
            if ((daysMask & (1 << (int)date.DayOfWeek)) == 0)
                continue;

            var firstOccurrence = date.Add(startTime);
            if (dayOffset > 0 || nowLocal <= firstOccurrence)
                return firstOccurrence;

            if (repeatMinutes <= 0)
                continue;

            var nextIndex = (int)Math.Ceiling((nowLocal - firstOccurrence).TotalMinutes / repeatMinutes);
            var nextOccurrence = firstOccurrence.AddMinutes(nextIndex * repeatMinutes);
            if (nextOccurrence.Date == date)
                return nextOccurrence;
        }

        return null;
    }

    internal static bool HasAutomaticEventGapElapsed(DateTime? lastFinishedAtUtc, DateTime nowUtc)
    {
        return !lastFinishedAtUtc.HasValue || nowUtc - lastFinishedAtUtc.Value >= MinimumAutomaticEventGap;
    }

    private static bool IsScheduledStart(string startedBy)
    {
        return startedBy.StartsWith("Schedule #", StringComparison.OrdinalIgnoreCase);
    }

    private static async Task<bool> HasTableAsync(SqlConnection connection, string tableName)
    {
        return await connection.ExecuteScalarAsync<int>(
            "SELECT CASE WHEN DB_ID(@DatabaseName) IS NOT NULL AND OBJECT_ID(@ObjectName, N'U') IS NOT NULL THEN 1 ELSE 0 END",
            new
            {
                DatabaseName = EventDatabaseName,
                ObjectName = $"{EventDatabaseName}.dbo.{tableName}"
            }) == 1;
    }
    private static async Task EnsureAutoEventScheduleSchemaAsync(SqlConnection connection)
    {
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'Events.dbo._AutoEventSchedule', N'U') IS NOT NULL
                  AND COL_LENGTH(N'Events.dbo._AutoEventSchedule', N'RepeatMinutes') IS NOT NULL
                  AND COL_LENGTH(N'Events.dbo._AutoEventSchedule', N'LastRunAtLocal') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Auto Event recurring schedule schema is incomplete. Apply the packaged v3.1.0 database update.");
    }
    private static async Task EnsureSurvivalPartySchemaAsync(SqlConnection connection)
    {
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN DB_ID(N'Events') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.Event_Control', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.Event_RegPlayers', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.Event_Wins', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.SPARTY_Score', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.survival_kill', N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Event_CurrentTeams', N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Event_CurrentTeams', N'EventName') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Survival Party base schema is incomplete. Apply the packaged database updates.");

        await SurvivalPartyEventService.EnsureSchemaAsync(connection);
    }

    internal static string NormalizeEventCode(string eventCode)
    {
        eventCode = (eventCode ?? string.Empty).Trim();
        var compact = eventCode.Replace(" ", string.Empty).Replace("_", string.Empty).Replace("-", string.Empty);
        return compact.ToLowerInvariant() switch
        {
            "first" or "firsttype" => "FirstType",
            "retype" => "Retype",
            "trivia" => "Trivia",
            "math" => "Math",
            "longest" or "longestonline" => "LongestOnline",
            "staller" or "luckystaller" => "LuckyStaller",
            "luckystall" or "stall" => "LuckyStall",
            "luckyparty" or "party" => "LuckyParty",
            "luckyglobal" or "global" => "LuckyGlobal",
            "alchemy" or "alchemyevent" => "Alchemy",
            "sparty" or "survivalparty" or "survivalpartyevent" => "SPARTY",
            "ssolo" or "survivalsolo" or "survivalsoloevent" => "SSOLO",
            "lms" or "lastmanstanding" or "lastmanstandingevent" => "LMS",
            "madness" or "madnesssolo" or "madnesssoloevent" => "MADNESS",
            "dtt" or "defendthetower" or "defendthetowerevent" => "DTT",
            "hns" or "hideandseek" or "hideandseekevent" => "HNS",
            _ => eventCode
        };
    }

    private static string NormalizeAnswer(string value)
    {
        value = value.Trim();
        value = SpaceRegex.Replace(value, " ");
        return value.ToUpperInvariant();
    }

    internal static int ResolveAlchemyTargetPlus(int configuredTarget)
    {
        return Math.Clamp(configuredTarget, 1, 20);
    }

    internal static bool IsWithinRoundWindow(DateTime startedAtUtc, DateTime endsAtUtc, DateTime occurredAtUtc)
    {
        return occurredAtUtc >= startedAtUtc && occurredAtUtc <= endsAtUtc;
    }

    private static string MaskAnswer(string answer)
    {
        if (string.IsNullOrEmpty(answer))
            return string.Empty;

        return answer.Length <= 2 ? "**" : answer[0] + new string('*', answer.Length - 2) + answer[^1];
    }

    private static string Trim(string? value, int max)
    {
        value ??= string.Empty;
        value = value.Trim();
        return value.Length <= max ? value : value[..max];
    }

    private sealed record RoundContent(string Prompt, string Answer)
    {
        public string NormalizedAnswer => NormalizeAnswer(Answer);
    }

    private sealed class ActiveEventRun
    {
        public ActiveEventRun(
            long runId,
            AutoEventConfig config,
            string startedBy,
            IReadOnlyList<AutoEventContent> content,
            IReadOnlyList<AutoEventReward> rewards)
        {
            RunID = runId;
            Config = config;
            StartedBy = startedBy;
            Content = content;
            Rewards = rewards;
        }

        public long RunID { get; }
        public AutoEventConfig Config { get; }
        public string StartedBy { get; }
        public IReadOnlyList<AutoEventContent> Content { get; }
        public IReadOnlyList<AutoEventReward> Rewards { get; }
        public CancellationTokenSource Cancellation { get; } = new();
        public TaskCompletionSource Completion { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public Task? ExecutionTask { get; set; }
        public string? RequestedStopReason { get; private set; }
        public int Finalized;
        public ActiveEventRound? CurrentRound { get; set; }
        public SurvivalPartyRuntimeConfig? SurvivalPartyConfig { get; set; }
        public SurvivalSoloRuntimeConfig? SurvivalSoloConfig { get; set; }
        public CompetitiveEventRuntimeConfig? CompetitiveConfig { get; set; }
        public HideAndSeekRuntimeConfig? HideAndSeekConfig { get; set; }
        public ConcurrentDictionary<int, DateTime> LastAnswerUtc { get; } = new();
        public ConcurrentDictionary<int, byte> WinnerCharIds { get; } = new();
        public ConcurrentDictionary<string, byte> WinnerHwids { get; } = new(StringComparer.OrdinalIgnoreCase);
        public ConcurrentDictionary<string, byte> WinnerIps { get; } = new(StringComparer.OrdinalIgnoreCase);
        public HashSet<int> UsedContentIds { get; } = new();
        public HashSet<string> UsedAnswers { get; } = new(StringComparer.OrdinalIgnoreCase);
        public ConcurrentDictionary<int, ParticipationEntry> LuckyPartyEntries { get; } = new();
        public ConcurrentDictionary<int, ParticipationEntry> LuckyGlobalEntries { get; } = new();
        public ConcurrentDictionary<int, ParticipationEntry> LuckyStallEntries { get; } = new();

        public bool TryRequestStop(string reason)
        {
            if (Interlocked.CompareExchange(ref _stopRequested, 1, 0) != 0)
                return false;

            RequestedStopReason = string.IsNullOrWhiteSpace(reason) ? "Stopped." : reason.Trim();
            return true;
        }

        private int _stopRequested;
    }

    private sealed class ActiveEventRound
    {
        public ActiveEventRound(
            long roundId,
            int roundNo,
            string prompt,
            string answer,
            string normalizedAnswer,
            int durationSeconds)
        {
            RoundID = roundId;
            RoundNo = roundNo;
            Prompt = prompt;
            Answer = answer;
            NormalizedAnswer = normalizedAnswer;
            EndsAtUtc = StartedAtUtc.AddSeconds(Math.Max(0, durationSeconds));
        }

        public long RoundID { get; }
        public int RoundNo { get; }
        public string Prompt { get; }
        public string Answer { get; }
        public string NormalizedAnswer { get; }
        public DateTime StartedAtUtc { get; } = DateTime.UtcNow;
        public DateTime EndsAtUtc { get; }
        public SemaphoreSlim WinnerLock { get; } = new(1, 1);
        public bool HasWinner { get; set; }
        public int Finalized;
        public int WinnerCharID { get; set; }
        public string WinnerCharName { get; set; } = string.Empty;
    }

    private sealed class AutoEventConfig
    {
        public string EventCode { get; set; } = string.Empty;
        public string DisplayName { get; set; } = string.Empty;
        public bool Enabled { get; set; }
        public int StartDelaySeconds { get; set; } = 60;
        public int RoundCount { get; set; } = 3;
        public int RoundDurationSeconds { get; set; } = 60;
        public int InterRoundDelaySeconds { get; set; } = 10;
        public int MinLevel { get; set; }
        public int HwidLimit { get; set; } = 1;
        public bool UniqueWinnerPerRun { get; set; } = true;
        public bool RequireHwid { get; set; } = true;
        public int AnswerCooldownMs { get; set; } = 750;
        public int AlchemyTargetPlus { get; set; } = 7;
    }

    private sealed class AutoEventContent
    {
        public int ContentID { get; set; }
        public string EventCode { get; set; } = string.Empty;
        public string Prompt { get; set; } = string.Empty;
        public string Answer { get; set; } = string.Empty;
        public int Weight { get; set; } = 1;
    }

    private sealed class AutoEventReward
    {
        public int RewardID { get; set; }
        public string EventCode { get; set; } = string.Empty;
        public int Placement { get; set; } = 1;
        public string RewardType { get; set; } = string.Empty;
        public long Amount { get; set; }
        public string? ItemCodeName128 { get; set; }
        public int? ItemID { get; set; }
        public int ItemCount { get; set; } = 1;
        public int Plus { get; set; }
    }

    private sealed class AutoEventCommand
    {
        public long CommandID { get; set; }
        public string CommandType { get; set; } = string.Empty;
        public string EventCode { get; set; } = string.Empty;
        public string RequestedBy { get; set; } = string.Empty;
    }

    private sealed class SurvivalPartySchedule
    {
        public int ScheduleID { get; set; }
        public TimeSpan StartTime { get; set; }
        public int DaysMask { get; set; } = 127;
        public DateTime? LastRunLocalDate { get; set; }
    }

    private sealed class AutoEventSchedule
    {
        public int ScheduleID { get; set; }
        public string EventCode { get; set; } = string.Empty;
        public TimeSpan StartTime { get; set; }
        public int DaysMask { get; set; } = 127;
        public int RepeatMinutes { get; set; }
        public DateTime? LastRunLocalDate { get; set; }
        public DateTime? LastRunAtLocal { get; set; }
    }

    private enum ParticipationKind
    {
        Party,
        Global,
        Stall
    }

    private sealed record ParticipationEntry(
        int CharID,
        string CharName,
        int JID,
        string Hwid,
        string ClientIP,
        string Value,
        string Evidence,
        DateTime CreatedAtUtc);

    private sealed record PartyMatchingRequest(long RunID, long RoundID);
}
