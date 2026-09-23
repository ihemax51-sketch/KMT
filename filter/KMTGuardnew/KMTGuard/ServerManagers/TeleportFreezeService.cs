using System.Collections.Concurrent;
using Dapper;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public static class TeleportFreezeService
{
    private const ushort FilterTeleportOpcode = 0x3540;
    private const int MaxFreezeSeconds = 600;
    private static readonly TimeSpan ActivePollInterval = TimeSpan.FromSeconds(1);
    private static readonly TimeSpan IdlePollInterval = TimeSpan.FromSeconds(5);
    private static readonly TimeSpan MissingQueuePollInterval = TimeSpan.FromSeconds(30);
    private static readonly ConcurrentDictionary<int, DateTime> FrozenUntilByCharId = new();
    private static CancellationTokenSource? _shutdown;
    private static Task? _worker;
    private static int _queueMissingLogged;

    public static void Start()
    {
        if (_worker != null)
            return;

        Interlocked.Exchange(ref _queueMissingLogged, 0);
        _shutdown = new CancellationTokenSource();
        _worker = Task.Run(() => ProcessQueueAsync(_shutdown.Token));
    }

    public static void Stop() => StopAsync().GetAwaiter().GetResult();

    public static async Task StopAsync()
    {
        CancellationTokenSource? shutdown = _shutdown;
        if (shutdown == null)
            return;

        try
        {
            shutdown.Cancel();
            if (_worker != null)
                await _worker;
        }
        catch (OperationCanceledException) when (shutdown.IsCancellationRequested)
        {
        }
        finally
        {
            shutdown.Dispose();
            _shutdown = null;
            _worker = null;
            FrozenUntilByCharId.Clear();
        }
    }

    public static async Task TeleportToPositionAsync(
        ISession? session,
        int gameWorldId,
        int regionId,
        int posX,
        int posY,
        int posZ,
        int freezeSeconds = 0)
    {
        regionId = RegionControlService.NormalizeRegionId(regionId);
        if (session == null || !session.CharacterGameReady || gameWorldId <= 0 ||
            !RegionControlService.IsValidRegionId(regionId))
            return;

        var admission = await RegionControlService.CheckAdmissionAsync(
            session,
            gameWorldId,
            regionId,
            RegionTravelMethod.FilterTeleport);
        if (!admission.Allowed)
            return;
        RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.FilterTeleport);

        // Custom GameServer commands are authenticated per live CGObjPC instance.
        // Register immediately before the teleport so reconnects and an early
        // post-login registration cannot leave this command silently rejected.
        var registerSessionKey = GameServerPacketAuthenticator.BuildRegistrationPacket(
            session.GameServerPacketKey,
            session.SessionData.UniqueCharId);
        await session.SendToServer(registerSessionKey);

        var packet = new Packet(FilterTeleportOpcode, false, false);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteInt32(gameWorldId);
        packet.WriteInt32(regionId);
        packet.WriteInt32(posX);
        packet.WriteInt32(posY);
        packet.WriteInt32(posZ);
        await session.SendToServer(packet);

        Freeze(session.SessionData.Charid, freezeSeconds);
    }

    public static Task<PacketResult?> BlockMovementIfFrozenAsync(ISession session)
    {
        int charId = session.SessionData.Charid;
        if (charId <= 0)
            return Task.FromResult<PacketResult?>(null);

        if (!FrozenUntilByCharId.TryGetValue(charId, out var frozenUntilUtc))
            return Task.FromResult<PacketResult?>(null);

        if (DateTime.UtcNow >= frozenUntilUtc)
        {
            FrozenUntilByCharId.TryRemove(charId, out _);
            return Task.FromResult<PacketResult?>(null);
        }

        return Task.FromResult<PacketResult?>(new PacketResult(PacketResultType.Block));
    }

    public static bool IsFrozen(int charId)
    {
        if (charId <= 0 || !FrozenUntilByCharId.TryGetValue(charId, out var frozenUntilUtc))
            return false;

        if (DateTime.UtcNow < frozenUntilUtc)
            return true;

        FrozenUntilByCharId.TryRemove(charId, out _);
        return false;
    }

    public static void Freeze(int charId, int seconds)
    {
        if (charId <= 0 || seconds <= 0)
            return;

        seconds = Math.Min(seconds, MaxFreezeSeconds);
        DateTime freezeUntilUtc = DateTime.UtcNow.AddSeconds(seconds);

        FrozenUntilByCharId.AddOrUpdate(
            charId,
            freezeUntilUtc,
            (_, current) => current > freezeUntilUtc ? current : freezeUntilUtc);
    }

    private static async Task ProcessQueueAsync(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            var nextPollInterval = IdlePollInterval;
            try
            {
                var commands = await ClaimPendingCommandsAsync(cancellationToken);
                foreach (var command in commands)
                    await ExecuteCommandAsync(command, cancellationToken);

                nextPollInterval = commands.Count > 0
                    ? ActivePollInterval
                    : IdlePollInterval;
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                return;
            }
            catch (SqlException ex) when (ex.Number == 208)
            {
                if (Interlocked.Exchange(ref _queueMissingLogged, 1) == 0)
                    Log.Information("Teleport freeze queue table is not installed yet.");
                nextPollInterval = MissingQueuePollInterval;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Teleport freeze queue polling failed.");
            }

            RemoveExpiredFreezes(DateTime.UtcNow);
            await Task.Delay(nextPollInterval, cancellationToken);
        }
    }

    private static void RemoveExpiredFreezes(DateTime nowUtc)
    {
        foreach (var entry in FrozenUntilByCharId)
        {
            if (entry.Value <= nowUtc)
                FrozenUntilByCharId.TryRemove(entry);
        }
    }

    private static async Task<IReadOnlyList<TeleportFreezeCommand>> ClaimPendingCommandsAsync(CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        const string sql = """
            SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
            ;WITH Pending AS
            (
                SELECT TOP (20) *
                FROM [dbo].[Teleport_FreezeQueue] WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
                WHERE Status = 0
                ORDER BY CommandID
            )
            UPDATE Pending
            SET Status = 1,
                PickedAt = SYSUTCDATETIME()
            OUTPUT
                INSERTED.CommandID,
                INSERTED.CharID,
                INSERTED.GameWorldID,
                INSERTED.RegionID,
                INSERTED.PosX,
                INSERTED.PosY,
                INSERTED.PosZ,
                INSERTED.FreezeSeconds;
            """;

        var commands = await connection.QueryAsync<TeleportFreezeCommand>(
            new CommandDefinition(sql, cancellationToken: cancellationToken));

        return commands.AsList();
    }

    private static async Task ExecuteCommandAsync(TeleportFreezeCommand command, CancellationToken cancellationToken)
    {
        try
        {
            var session = FindOnlineCharacter(command.CharID);
            if (session == null || !session.CharacterGameReady)
            {
                await CompleteCommandAsync(command.CommandID, 3, "Character is not online or not game ready.", cancellationToken);
                return;
            }

            await TeleportToPositionAsync(
                session,
                command.GameWorldID,
                command.RegionID,
                command.PosX,
                command.PosY,
                command.PosZ,
                command.FreezeSeconds);

            await CompleteCommandAsync(command.CommandID, 2, null, cancellationToken);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Teleport freeze command failed. CommandID={CommandID}", command.CommandID);
            await CompleteCommandAsync(command.CommandID, 3, ex.Message, cancellationToken);
        }
    }

    private static async Task CompleteCommandAsync(long commandId, byte status, string? errorMessage, CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        const string sql = """
            UPDATE [dbo].[Teleport_FreezeQueue]
            SET Status = @Status,
                CompletedAt = SYSUTCDATETIME(),
                ErrorMessage = @ErrorMessage
            WHERE CommandID = @CommandID;
            """;

        await connection.ExecuteAsync(
            new CommandDefinition(
                sql,
                new
                {
                    CommandID = commandId,
                    Status = status,
                    ErrorMessage = errorMessage != null && errorMessage.Length > 512
                        ? errorMessage.Substring(0, 512)
                        : errorMessage
                },
                cancellationToken: cancellationToken));
    }

    private static ISession? FindOnlineCharacter(int charId)
    {
        return ServerManager.AgentSessions.FindByCharId(
            charId,
            static session => session.CharacterGameReady && !session.IsStopped && !session.ClientDetached);
    }

    private sealed class TeleportFreezeCommand
    {
        public long CommandID { get; set; }
        public int CharID { get; set; }
        public int GameWorldID { get; set; }
        public int RegionID { get; set; }
        public int PosX { get; set; }
        public int PosY { get; set; }
        public int PosZ { get; set; }
        public int FreezeSeconds { get; set; }
    }
}
