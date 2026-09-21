using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.Database.ModelsEvents;
using KMTGuard.Database.RankModels;
using KMTGuard.Helpers;
using KMTGuard.Features.UniqueHistory;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Net.NetworkInformation;
using System.Reflection.PortableExecutable;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using static System.Collections.Specialized.BitVector32;
using static System.Runtime.InteropServices.JavaScript.JSType;
using Timer = System.Timers.Timer;

namespace KMTGuard.Database
{
    public static class DatabaseCommands
    {

        public static string FormatNumber(long num)
        {
            if (num >= 100000000)
            {
                return (num / 1000000D).ToString("0.#M");
            }
            if (num >= 1000000)
            {
                return (num / 1000000D).ToString("0.##M");
            }
            if (num >= 100000)
            {
                return (num / 1000D).ToString("0.#k");
            }
            if (num >= 10000)
            {
                return (num / 1000D).ToString("0.##k");
            }

            return num.ToString("#,0");
        }

        private static Timer? _commandTimer;
        private static readonly SemaphoreSlim _commandLock = new(1, 1);
        private static readonly SemaphoreSlim _plannedCommandLock = new(1, 1);
        private static readonly SemaphoreSlim _timerUpdateLock = new(1, 1);
        private static readonly object _timerInitializationLock = new();
        private static DateTime _nextCommandRetryUtc = DateTime.MinValue;
        private static DateTime _nextPlannedCommandRetryUtc = DateTime.MinValue;
        private const int CommandBatchSize = 50;
        private const int PlannedCommandBatchSize = 20;
        private const int CommandQueryTimeoutSeconds = 30;
        private const int CommandRetryBackoffSeconds = 15;
        private const int CommandLeaseSeconds = 120;
        private const int CommandMaxAttempts = 5;
        private static readonly string CommandWorkerId =
            $"{Environment.MachineName}:{Environment.ProcessId}:{Guid.NewGuid():N}";
        private static readonly ConcurrentDictionary<int, long> _selfTeleportCooldown = new();
        private static readonly long SelfTeleportCooldownTicks =
            (long)(Stopwatch.Frequency * 1.0);
        private static readonly long SelfTeleportRetentionTicks =
            (long)(Stopwatch.Frequency * 60.0);
        private static readonly long SelfTeleportCleanupIntervalTicks =
            (long)(Stopwatch.Frequency * 60.0);
        private static long _lastSelfTeleportCleanupTimestamp;
        public static void InitializeTimer()
        {
            if (_commandTimer != null)
                return;

            _commandTimer = new Timer(2000);
            _commandTimer.Elapsed += OnTimerTick;
            _commandTimer.AutoReset = true;
            _commandTimer.Enabled = true;
        }
        public static System.Timers.Timer? timers;

        private static void EnsureTimerUpdaterStarted()
        {
            lock (_timerInitializationLock)
            {
                if (timers != null)
                    return;

                timers = new System.Timers.Timer
                {
                    Interval = 1000,
                    AutoReset = true
                };
                timers.Elapsed += (sender, e) => _ = UpdateTimers(null);
                timers.Start();
            }
        }

        public static async Task SetRegionTimerAsync(int regionId, int seconds)
        {
            if (regionId <= 0 || seconds < 0 || seconds > int.MaxValue / 1000)
            {
                Log.Warning(
                    "Ignored invalid region timer command. RegionID={RegionID}, Seconds={Seconds}",
                    regionId,
                    seconds);
                return;
            }

            await _timerUpdateLock.WaitAsync();
            try
            {
                Packet timer = new Packet(0x220A);
                if (seconds == 0)
                {
                    ActionManager.CreatedTimerListRegionID.TryRemove(regionId, out _);
                    await ServerManager.RestoreWorldTimerOrCloseByRegionID(regionId);
                    return;
                }

                int milliseconds = seconds * 1000;
                ActionManager.CreatedTimerListRegionID[regionId] = milliseconds;

                timer.WriteUInt8(0);
                timer.WriteInt32(milliseconds);
                await ServerManager.BroadcastPacketbyRegionID(regionId, timer);
                EnsureTimerUpdaterStarted();
            }
            finally
            {
                _timerUpdateLock.Release();
            }
        }

        public static void OnTimerTick(object? sender, ElapsedEventArgs e)
        {
            _ = RunCommandTimerAsync();
        }

        private static async Task RunCommandTimerAsync()
        {
            try
            {
                await ProcessCommands();
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Unhandled command timer failure");
            }
        }
        public static async Task UpdateTimers(object? state)
        {
            if (!await _timerUpdateLock.WaitAsync(0))
                return;

            try
            {
                foreach (var key in ActionManager.CreatedTimerListWorldID.Keys)
                {
                    if (ActionManager.CreatedTimerListWorldID.TryGetValue(key, out int remainingTime))
                    {
                        remainingTime -= 1000;
                        if (remainingTime <= 0)
                        {
                            ActionManager.CreatedTimerListWorldID.TryRemove(key, out _);
                            Packet timer = new Packet(0x220A);
                            timer.WriteUInt8(1);
                            await ServerManager.BroadcastPacketbyWorldID(key, timer);
                        }
                        else
                        {
                            ActionManager.CreatedTimerListWorldID[key] = remainingTime;
                        }
                    }
                }

                foreach (var key in ActionManager.CreatedTimerListRegionID.Keys)
                {
                    if (!ActionManager.CreatedTimerListRegionID.TryGetValue(key, out int remainingTime))
                        continue;

                    remainingTime -= 1000;
                    if (remainingTime <= 0)
                    {
                        ActionManager.CreatedTimerListRegionID.TryRemove(key, out _);
                        await ServerManager.RestoreWorldTimerOrCloseByRegionID(key);
                    }
                    else
                    {
                        ActionManager.CreatedTimerListRegionID[key] = remainingTime;
                    }
                }
            }
            finally
            {
                _timerUpdateLock.Release();
            }
        }
        public static async Task ProcessCommands()
        {
            if (DateTime.UtcNow < _nextCommandRetryUtc)
                return;

            if (!await _commandLock.WaitAsync(0))
                return;

            try
            {
                DateTime currentTime = DateTime.Now;

                using (SqlConnection sharedConnection = new SqlConnection(Program.Connectionstring))
                {
                    await sharedConnection.OpenAsync();

                    // VIP loyalty follows every actual decrease in the account's
                    // Silk balance, independently of Item Mall/NPC/custom feature
                    // packet paths. Resolve the JID to the active character here.
                    // The durable setting is loaded during filter startup, so a
                    // disabled VIP system leaves the events untouched until it is
                    // enabled again.
                    if (_serverSettings.VipSystemEnabled)
                        await ProcessVipSilkSpendEventsAsync(sharedConnection);

                    List<_AsyncFilterCommands> pendingCommands;

                    pendingCommands = (await sharedConnection.QueryAsync<_AsyncFilterCommands>(
                        @"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
                          ;WITH Claimable AS
                          (
                              SELECT TOP (@Take) *
                                FROM dbo.Command_FilterQueue WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
                               WHERE Status = 1
                                  OR (Status = 2 AND ClaimedUtc < DATEADD(SECOND, -@LeaseSeconds, SYSUTCDATETIME()))
                                  OR (Status = 3 AND ClaimedUtc <= SYSUTCDATETIME())
                               ORDER BY ID
                          )
                          UPDATE Claimable
                             SET Status = 2,
                                 ClaimToken = NEWID(),
                                 IdempotencyKey = COALESCE(IdempotencyKey, NEWID()),
                                 ClaimedUtc = SYSUTCDATETIME(),
                                 ClaimedBy = @Worker,
                                 Attempts = Attempts + 1,
                                 LastError = NULL
                          OUTPUT INSERTED.*;",
                        new
                        {
                            Take = CommandBatchSize,
                            LeaseSeconds = CommandLeaseSeconds,
                            Worker = CommandWorkerId
                        },
                        commandTimeout: CommandQueryTimeoutSeconds)).ToList();

                    foreach (var command in pendingCommands)
                    {
                        try
                        {
                            int CommandID = command.CommandID;
                            int ID = command.ID;
                            if (command.Status == 2)
                            {
                                if (!await TryBeginCommandExecutionAsync(
                                        sharedConnection, "Command_FilterQueue", ID,
                                        CommandID, command.IdempotencyKey))
                                    continue;

                                if (CommandID == 1)
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (ServerManager.AgentSessions != null)
                                        {
                                            var session = FindAgentSessionByCharName(command.Data1);
                                            if (session != null)
                                            {
                                                session.Stop("Filter command queue requested character disconnect (CommandID 1)");
                                                ServerManager.AgentSessions.Remove(session);
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested
                                else if (CommandID == 2)
                                {
                                    if (ServerManager.AgentSessions != null)
                                    {
                                        foreach (var line in SnapshotAgentSessions())
                                        {
                                            line.Stop("Filter command queue requested disconnect all (CommandID 2)");
                                            ServerManager.AgentSessions.Remove(line);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested
                                else if (CommandID == 3)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null)
                                    {
                                        if (byte.TryParse(command.Data2, out byte type))
                                        {
                                            if (command.Data1.ToLower().Contains("sendall"))
                                            {
                                                Packet Notice = new Packet(0x168A);
                                                Notice.WriteUInt8(type);
                                                Notice.WriteUnicode(command.Data3);// Message
                                                await ServerManager.BroadcastPacket(Notice);
                                            }
                                            else if (command.Data1.ToLower().Contains("sendchar") && command.Data4 != null)
                                            {
                                                if (command.Data4.Length > 0)
                                                {
                                                    Packet Notice = new Packet(0x168A);
                                                    Notice.WriteUInt8(type);
                                                    Notice.WriteUnicode(command.Data3);// Message
                                                    if (command.Data4 != null)
                                                    {
                                                        await ServerManager.BroadcastPacketToCharName(command.Data4, Notice);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested
                                else if (CommandID == 4)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int charid) && byte.TryParse(command.Data2, out byte titleid))
                                        {
                                            var session = FindAgentSessionByCharId(charid);

                                            if (session != null)
                                            {
                                                if (!session.SessionData.PlayerTitles.Contains(titleid))
                                                {
                                                    session.SessionData.PlayerTitles.Add(titleid);
                                                    Packet Title = new Packet(0x168B);
                                                    Title.WriteUInt8(titleid);
                                                    if (session.SessionData.CHChar)
                                                    {
                                                        Title.WriteAscii(RefManager.RefHwan[titleid].Title_CH70);
                                                    }
                                                    if (session.SessionData.EUChar)
                                                    {
                                                        Title.WriteAscii(RefManager.RefHwan[titleid].Title_EU70);
                                                    }
                                                    await session.SendToClient(Title);
                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }  // tested
                                else if (CommandID == 5)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int charid) && int.TryParse(command.Data4, out int ColorDBID))
                                        {
                                            var session = FindAgentSessionByCharId(charid);

                                            if (session != null)
                                            {
                                                if (!session.SessionData.PlayerTitleColors.ContainsKey(ColorDBID))
                                                {
                                                    var titlec = new PlayerTitleColor
                                                    {
                                                        CharID = session.SessionData.Charid,
                                                        ColorName = command.Data2,
                                                        ColorCode = command.Data3,
                                                    };

                                                    session.SessionData.PlayerTitleColors.TryAdd(ColorDBID, titlec);

                                                    Packet TitleColors = new Packet(0x168C);
                                                    TitleColors.WriteInt32(ColorDBID);

                                                    TitleColors.WriteAscii(command.Data2);
                                                    int argbInputColor = Int32.Parse(command.Data3.Replace("#", ""), NumberStyles.HexNumber);
                                                    TitleColors.WriteUInt32(argbInputColor);

                                                    await session.SendToClient(TitleColors);
                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested
                                else if (CommandID == 6)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID) && int.TryParse(command.Data2, out int IconID)
                                            && byte.TryParse(command.Data3, out byte Side) && int.TryParse(command.Data4, out int DBID))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);

                                            if (session != null)
                                            {
                                                if (!session.SessionData.PlayerIcons.ContainsKey(DBID))
                                                {

                                                    var icon = new PlayerIcon
                                                    {
                                                        ID = DBID,
                                                        CharID = session.SessionData.Charid,
                                                        IconID = IconID,
                                                        Side = Side,
                                                    };

                                                    session.SessionData.PlayerIcons.TryAdd(DBID, icon);
                                                    Packet IconPacket = new Packet(0x168D);

                                                    IconPacket.WriteInt32(DBID);

                                                    if (RefManager.Icons.ContainsKey(IconID))
                                                    {
                                                        IconPacket.WriteAscii(RefManager.Icons[IconID]);
                                                    }
                                                    else
                                                    {
                                                        IconPacket.WriteAscii("dummy");
                                                    }
                                                    IconPacket.WriteUInt8(Side);
                                                    await session.SendToClient(IconPacket);
                                                }
                                            }

                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 7)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID) && int.TryParse(command.Data2, out int TitleID))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);

                                            if (session != null)
                                            {
                                                if (session.SessionData.State.BodyState != BodyState.Berserk)
                                                {
                                                    Packet stAckMsg = new Packet(0x3501);
                                                    stAckMsg.WriteAscii(session.GameServerPacketKey);
                                                    stAckMsg.WriteUInt8(TitleID);
                                                    await session.SendToServer(stAckMsg);
                                                }
                                            }
                                        }

                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested
                                else if (CommandID == 8)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (RefManager.ActiveTitleColors.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveTitleColors[command.Data1] = command.Data2;
                                        }
                                        else
                                        {
                                            RefManager.ActiveTitleColors.TryAdd(command.Data1, command.Data2);
                                        }

                                        int argbInputColor = Int32.Parse(command.Data2.Replace("#", ""), NumberStyles.HexNumber);

                                        Packet stAckMsg = new Packet(0x170A);
                                        stAckMsg.WriteAscii(command.Data1);
                                        stAckMsg.WriteUInt32(argbInputColor);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested UPDATE TITLE COLOR
                                else if (CommandID == 9) /// REMOVE TITLE COLOR
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (RefManager.ActiveTitleColors.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveTitleColors.TryRemove(command.Data1, out _);
                                        }
                                        Packet stAckMsg = new Packet(0x170B);
                                        stAckMsg.WriteAscii(command.Data1);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 10) /// UPDATE LEFT CHAR ICON
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data2, out int IconID))
                                        {
                                        if (RefManager.ActiveLeftIcons.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveLeftIcons[command.Data1] = IconID;
                                        }
                                        else
                                        {
                                            RefManager.ActiveLeftIcons.TryAdd(command.Data1, IconID);
                                            }
                                            Packet stAckMsg = new Packet(0x173F);
                                            stAckMsg.WriteAscii(command.Data1);
                                            stAckMsg.WriteInt32(IconID);
                                            await ServerManager.BroadcastPacket(stAckMsg);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 11) /// UPDATE RIGHT CHAR ICON
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data2, out int IconID))
                                        {
                                        if (RefManager.ActiveRightIcons.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveRightIcons[command.Data1] = IconID;
                                        }
                                        else
                                        {
                                            RefManager.ActiveRightIcons.TryAdd(command.Data1, IconID);
                                            }
                                            Packet stAckMsg = new Packet(0x174B);
                                            stAckMsg.WriteAscii(command.Data1);
                                            stAckMsg.WriteInt32(IconID);
                                            await ServerManager.BroadcastPacket(stAckMsg);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 12) /// REMOVE LEFT CHAR ICON
                                {
                                    if (command.Data1 != null)
                                    {

                                        if (RefManager.ActiveLeftIcons.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveLeftIcons.TryRemove(command.Data1, out _);
                                        }
                                        Packet stAckMsg = new Packet(0x174A);
                                        stAckMsg.WriteAscii(command.Data1);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 13) /// REMOVE right CHAR ICON
                                {
                                    if (command.Data1 != null)
                                    {

                                        if (RefManager.ActiveRightIcons.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveRightIcons.TryRemove(command.Data1, out _);
                                        }
                                        Packet stAckMsg = new Packet(0x174E);
                                        stAckMsg.WriteAscii(command.Data1);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 15)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (byte.TryParse(command.Data2, out byte TitleID))
                                        {
                                            if (RefManager.Tags.ContainsKey(TitleID))
                                            {
                                                if (RefManager.ActiveTags.ContainsKey(command.Data1))
                                                {
                                                    RefManager.ActiveTags[command.Data1] = TitleID;
                                                }
                                                else
                                                {
                                                    RefManager.ActiveTags.TryAdd(command.Data1, TitleID);
                                                }
                                                Packet stAckMsg = new Packet(0x202B);
                                                stAckMsg.WriteAscii(command.Data1);
                                                stAckMsg.WriteAscii(RefManager.Tags[TitleID]);
                                                await ServerManager.BroadcastPacket(stAckMsg);
                                            }
                                        }


                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 16) /// REMOVE new title
                                {
                                    if (command.Data1 != null)
                                    {

                                        RefManager.ActiveTags.TryRemove(command.Data1, out _);
                                        Packet stAckMsg = new Packet(0x202C);
                                        stAckMsg.WriteAscii(command.Data1);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 17)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null && command.Data5 != null &&
                                        command.Data6 != null && command.Data7 != null && command.Data8 != null)
                                    {

                                        if (int.TryParse(command.Data1, out int DbID) && int.TryParse(command.Data2, out int CharID) && int.TryParse(command.Data4, out int ItemIdx)
                                            && int.TryParse(command.Data5, out int Quantity) && byte.TryParse(command.Data8, out byte Plus))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);
                                            if (session != null)
                                            {
                                                if (!session.SessionData.CharacterChest.ContainsKey(DbID))
                                                {
                                                    var newrow = new _ItemChest();
                                                    newrow.ID = DbID;
                                                    newrow.CharID = CharID;
                                                    newrow.ItemCodeName = command.Data3;
                                                    newrow.ItemID = ItemIdx;
                                                    newrow.Quantity = Quantity;
                                                    newrow.Date = command.Data6;
                                                    newrow.Type = command.Data7;
                                                    newrow.Plus = Plus;

                                                    session.SessionData.CharacterChest.TryAdd(DbID, newrow);

                                                    Packet ach = new Packet(0x203B);
                                                    ach.WriteInt32(DbID);
                                                    ach.WriteInt32(ItemIdx);
                                                    ach.WriteInt32(Quantity);

                                                    ach.WriteAscii(command.Data6 ?? string.Empty);
                                                    ach.WriteAscii(command.Data7 ?? string.Empty);
                                                    ach.WriteUInt8(Plus);
                                                    await session.SendToClient(ach);



                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 18)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int GateID)
                                            && GateID > 0
                                            && TryParseCommandState(command.Data2, out bool State))
                                        {
                                            ActionManager.m_CanTeleport[GateID] = State;
                                            Log.Information(
                                                "Live teleport state changed. GateID={GateID}, State={State}",
                                                GateID,
                                                State ? "Open" : "Closed");
                                        }
                                        else
                                        {
                                            Log.Warning(
                                                "Ignored invalid live teleport command. CommandRowID={CommandRowID}, GateID={GateID}, State={State}",
                                                ID,
                                                command.Data1,
                                                command.Data2);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 19)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int RegionID) && bool.TryParse(command.Data2, out bool State))
                                        {
                                            if (ActionManager.m_CanAttackbyregionId.ContainsKey(RegionID))
                                            {
                                                ActionManager.m_CanAttackbyregionId[RegionID] = State;
                                            }
                                            else
                                            {
                                                ActionManager.m_CanAttackbyregionId.TryAdd(RegionID, State);
                                            }
                                        }

                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 20)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int WorldID) && bool.TryParse(command.Data2, out bool State))
                                        {
                                            if (ActionManager.m_CanAttackbyWorldId.ContainsKey(WorldID))
                                            {
                                                ActionManager.m_CanAttackbyWorldId[WorldID] = State;
                                            }
                                            else
                                            {
                                                ActionManager.m_CanAttackbyWorldId.TryAdd(WorldID, State);
                                            }
                                        }

                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 21)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int TimeMin) && int.TryParse(command.Data2, out int WorldID))
                                        {
                                            Packet pck = new Packet(0x220A);
                                            pck.WriteUInt8(0);
                                            pck.WriteInt32(TimeMin * 1000);

                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 22)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int TimeMin) && int.TryParse(command.Data2, out int RegionID))
                                        {
                                            Packet pck = new Packet(0x220A);
                                            pck.WriteUInt8(0);
                                            pck.WriteInt32(TimeMin * 1000);

                                            await ServerManager.BroadcastPacketbyRegionID(RegionID, pck);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 23)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int TimeMin) && int.TryParse(command.Data2, out int WorldID))
                                        {
                                            Packet pck = new Packet(0x220A);
                                            pck.WriteUInt8(0);
                                            pck.WriteInt32(TimeMin * 1000);

                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);

                                            if (ActionManager.CreatedTimerListWorldID.ContainsKey(WorldID))
                                            {
                                                ActionManager.CreatedTimerListWorldID[WorldID] = TimeMin * 1000;
                                            }
                                            else
                                            {
                                                ActionManager.CreatedTimerListWorldID.TryAdd(WorldID, TimeMin * 1000);
                                            }
                                            EnsureTimerUpdaterStarted();
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 24)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int seconds) && int.TryParse(command.Data2, out int regionId))
                                        {
                                            await SetRegionTimerAsync(regionId, seconds);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 25)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data2, out int WorldID))
                                        {

                                            if (ActionManager.CreatedKillCounterWorldID.ContainsKey(WorldID))
                                            {
                                                ActionManager.CreatedKillCounterWorldID[WorldID] = command.Data1;
                                                Packet pck = new Packet(0x207A);
                                                pck.WriteUInt8(0);
                                                pck.WriteAscii(command.Data1);
                                                await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);

                                                if (ActionManager.KillCounterKillList.Count() > 0)
                                                {
                                                    foreach (var line in ActionManager.KillCounterKillList)
                                                    {
                                                        if (line.Value.WorldID == WorldID)
                                                        {
                                                            ActionManager.KillCounterKillList.TryRemove(line.Value.CharName16, out _);
                                                        }
                                                    }
                                                }

                                            }
                                            else
                                            {
                                                ActionManager.CreatedKillCounterWorldID.TryAdd(WorldID, command.Data1);
                                                Packet pck = new Packet(0x207A);
                                                pck.WriteUInt8(1);
                                                pck.WriteAscii(command.Data1);
                                                await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);

                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 26)
                                {

                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int WorldID) && int.TryParse(command.Data3, out int Kill))
                                        {
                                            if (ActionManager.CreatedKillCounterWorldID.ContainsKey(WorldID))
                                            {
                                                string Charname = command.Data2;
                                                if (ActionManager.KillCounterKillList.ContainsKey(Charname))
                                                {
                                                    if (ActionManager.KillCounterKillList[Charname].WorldID == WorldID)
                                                    {
                                                        ActionManager.KillCounterKillList[Charname].Kill += Kill;
                                                        var topKillers = ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();


                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x207C);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteInt32(line.Value.Kill);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }

                                                    }
                                                    else
                                                    {
                                                        ActionManager.KillCounterKillList[Charname].WorldID = WorldID;
                                                        ActionManager.KillCounterKillList[Charname].Kill = 1;
                                                        var topKillers = ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();


                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x207C);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteInt32(line.Value.Kill);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }
                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x207C);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteInt32(line.Value.Kill);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    var Create = new SCreatedKillCounterKillList();
                                                    Create.WorldID = WorldID;
                                                    Create.CharName16 = Charname;
                                                    Create.Kill = Kill;
                                                    ActionManager.KillCounterKillList.TryAdd(Charname, Create);
                                                    var topKillers = ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();

                                                    if (topKillers.Count() > 0)
                                                    {
                                                        Packet counter = new Packet(0x207C);
                                                        counter.WriteUInt8(topKillers.Count());
                                                        foreach (var line in topKillers)
                                                        {
                                                            counter.WriteAscii(line.Value.CharName16);
                                                            counter.WriteInt32(line.Value.Kill);
                                                        }
                                                        await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 27)
                                {
                                    if (_serverSettings.VipSystemEnabled &&
                                        command.Data1 != null &&
                                        int.TryParse(command.Data1, out int charId))
                                    {
                                        // Rank_Silk is authoritative. Reading the complete row here avoids
                                        // stale history after retries, dashboard recalculation, or legacy
                                        // Platinum commands that were inserted with Status = 2.
                                        var silkRank = await sharedConnection.QuerySingleOrDefaultAsync<_SilkRank>(
                                            @"SELECT TOP (1)
                                                     ID,
                                                     JID,
                                                     CharID,
                                                     SilkHistory,
                                                     SilkRank
                                              FROM [dbo].[Rank_Silk] WITH (NOLOCK)
                                              WHERE CharID = @CharID",
                                            new { CharID = charId });

                                        if (silkRank != null)
                                        {
                                            RankManager.m_SilkRank[charId] = silkRank;

                                            var session = FindAgentSessionByCharId(charId);
                                            if (session != null)
                                            {
                                                Packet packet = new Packet(0x209C);
                                                packet.WriteInt32(silkRank.SilkHistory);
                                                packet.WriteInt32(silkRank.SilkRank);
                                                await session.SendToClient(packet);
                                            }
                                        }
                                        else
                                        {
                                            RankManager.m_SilkRank.TryRemove(charId, out _);
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 28)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID) && byte.TryParse(command.Data3, out byte Type))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);

                                            if (session != null)
                                            {
                                                Packet Notice = new Packet(0x168A, false, false);
                                                Notice.WriteUInt8(Type);
                                                Notice.WriteUnicode(command.Data2); // Message
                                                await session.SendToClient(Notice);
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 29)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null && command.Data5 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID) && int.TryParse(command.Data2, out int RefAchievementID)
                                            && int.TryParse(command.Data3, out int RefAchievementConditionID) && Int64.TryParse(command.Data4, out Int64 Progress)
                                            && byte.TryParse(command.Data5, out byte State))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);

                                            if (session != null)
                                            {
                                                if (!session.SessionData.CharacterAchievement.TryGetValue(RefAchievementID, out var achievement))
                                                {
                                                    achievement = new _Achievement
                                                    {
                                                        CharID = CharID,
                                                        RefAchievementID = RefAchievementID,
                                                        State = State
                                                    };
                                                    session.SessionData.CharacterAchievement[RefAchievementID] = achievement;
                                                }

                                                achievement.State = State;
                                                if (!session.SessionData.CharacterAchievementCondition.TryGetValue(
                                                        RefAchievementConditionID, out var condition))
                                                {
                                                    condition = new _AchievementCondition
                                                    {
                                                        CharID = CharID,
                                                        RefAchievementID = RefAchievementID,
                                                        RefAchievementConditionID = RefAchievementConditionID
                                                    };
                                                    session.SessionData.CharacterAchievementCondition[RefAchievementConditionID] = condition;
                                                }

                                                condition.ProgressCount = Progress;
                                                Packet pck = new Packet(0x177E);
                                                pck.WriteInt32(RefAchievementID);
                                                pck.WriteInt32(RefAchievementConditionID);
                                                pck.WriteInt64(Progress);
                                                pck.WriteUInt8(State);
                                                await session.SendToClient(pck);
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);

                                }
                                else if (CommandID == 30)
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int WorldID))
                                        {
                                            if (ActionManager.CreatedTeamKillCounterWorldID.ContainsKey(WorldID))
                                            {
                                                Packet pck = new Packet(0x189A);
                                                pck.WriteUInt8(0);
                                                await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);
                                                if (ActionManager.TeamKillCounterKillList.Count() > 0)
                                                {
                                                    foreach (var line in ActionManager.TeamKillCounterKillList)
                                                    {
                                                        if (line.Value.WorldID == WorldID)
                                                        {
                                                            ActionManager.TeamKillCounterKillList.TryRemove(line.Value.CharName16, out _);
                                                        }
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ActionManager.CreatedTeamKillCounterWorldID.TryAdd(WorldID, command.Data1);
                                                Packet pck = new Packet(0x189A);
                                                pck.WriteUInt8(1);
                                                await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);

                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 31)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int WorldID) && int.TryParse(command.Data3, out int Kill) && int.TryParse(command.Data4, out int Team))
                                        {
                                            if (ActionManager.CreatedTeamKillCounterWorldID.ContainsKey(WorldID))
                                            {
                                                string Charname = command.Data2;
                                                if (ActionManager.TeamKillCounterKillList.ContainsKey(Charname))
                                                {
                                                    if (ActionManager.TeamKillCounterKillList[Charname].WorldID == WorldID)
                                                    {
                                                        ActionManager.TeamKillCounterKillList[Charname].Kill += Kill;
                                                        // update counter
                                                        var topKillers = ActionManager.TeamKillCounterKillList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();


                                                        int redteamkillss = 0;
                                                        int blueteamkillss = 0;
                                                        foreach (var x in topKillers)
                                                        {

                                                            if (x.Value.Team == 1)
                                                            {
                                                                redteamkillss += x.Value.Kill;
                                                            }
                                                            else if (x.Value.Team == 3)
                                                            {
                                                                blueteamkillss += x.Value.Kill;
                                                            }
                                                        }

                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x189B);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteUInt8(line.Value.Team);
                                                                counter.WriteInt32(line.Value.Kill);
                                                                counter.WriteInt32(redteamkillss);
                                                                counter.WriteInt32(blueteamkillss);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        ActionManager.TeamKillCounterKillList[Charname].WorldID = WorldID;
                                                        ActionManager.TeamKillCounterKillList[Charname].Kill = Kill;
                                                        // update counter
                                                        var topKillers = ActionManager.TeamKillCounterKillList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();


                                                        int redteamkillss = 0;
                                                        int blueteamkillss = 0;
                                                        foreach (var x in topKillers)
                                                        {

                                                            if (x.Value.Team == 1)
                                                            {
                                                                redteamkillss += x.Value.Kill;
                                                            }
                                                            else if (x.Value.Team == 3)
                                                            {
                                                                blueteamkillss += x.Value.Kill;
                                                            }
                                                        }

                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x189B);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteUInt8(line.Value.Team);
                                                                counter.WriteInt32(line.Value.Kill);
                                                                counter.WriteInt32(redteamkillss);
                                                                counter.WriteInt32(blueteamkillss);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    var Create = new SCreatedTeamKillCounterKillList();
                                                    Create.WorldID = WorldID;
                                                    Create.CharName16 = Charname;
                                                    Create.Kill = Kill;
                                                    Create.Team = Team;
                                                    ActionManager.TeamKillCounterKillList.TryAdd(Charname, Create);

                                                    var topKillers = ActionManager.TeamKillCounterKillList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();

                                                    int redteamkillss = 0;
                                                    int blueteamkillss = 0;
                                                    foreach (var x in topKillers)
                                                    {

                                                        if (x.Value.Team == 1)
                                                        {
                                                            redteamkillss += x.Value.Kill;
                                                        }
                                                        else if (x.Value.Team == 3)
                                                        {
                                                            blueteamkillss += x.Value.Kill;
                                                        }
                                                    }

                                                    if (topKillers.Count() > 0)
                                                    {
                                                        Packet counter = new Packet(0x189B);
                                                        counter.WriteUInt8(topKillers.Count());
                                                        foreach (var line in topKillers)
                                                        {
                                                            counter.WriteAscii(line.Value.CharName16);
                                                            counter.WriteUInt8(line.Value.Team);
                                                            counter.WriteInt32(line.Value.Kill);
                                                            counter.WriteInt32(redteamkillss);
                                                            counter.WriteInt32(blueteamkillss);
                                                        }
                                                        await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 32)
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int WorldID))
                                        {
                                            if (ActionManager.CreatedJobKillCounterWorldID.ContainsKey(WorldID))
                                            {
                                                Packet pck = new Packet(0x189C);
                                                pck.WriteUInt8(0);
                                                await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);
                                                if (ActionManager.CreatedJobKillCounterWorldID.Count() > 0)
                                                {
                                                    foreach (var line in ActionManager.JobKillCounterWorldID)
                                                    {
                                                        if (line.Value.WorldID == WorldID)
                                                        {
                                                            ActionManager.JobKillCounterWorldID.TryRemove(line.Value.CharName16, out _);
                                                        }
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                ActionManager.CreatedJobKillCounterWorldID.TryAdd(WorldID, command.Data1);
                                                Packet pck = new Packet(0x189C);
                                                pck.WriteUInt8(1);
                                                await ServerManager.BroadcastPacketbyWorldID(WorldID, pck);

                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 33)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int WorldID) && int.TryParse(command.Data3, out int Kill) && int.TryParse(command.Data4, out int Team))
                                        {
                                            if (ActionManager.CreatedJobKillCounterWorldID.ContainsKey(WorldID))
                                            {
                                                string Charname = command.Data2;
                                                if (ActionManager.JobKillCounterWorldID.ContainsKey(Charname))
                                                {
                                                    if (ActionManager.JobKillCounterWorldID[Charname].WorldID == WorldID)
                                                    {
                                                        ActionManager.JobKillCounterWorldID[Charname].Kill += Kill;
                                                        // update counter
                                                        var topKillers = ActionManager.JobKillCounterWorldID.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();


                                                        int redteamkillss = 0;
                                                        int blueteamkillss = 0;
                                                        foreach (var x in topKillers)
                                                        {

                                                            if (x.Value.Team == 1)
                                                            {
                                                                redteamkillss += x.Value.Kill;
                                                            }
                                                            else if (x.Value.Team == 3)
                                                            {
                                                                blueteamkillss += x.Value.Kill;
                                                            }
                                                        }

                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x189D);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteUInt8(line.Value.Team);
                                                                counter.WriteInt32(line.Value.Kill);
                                                                counter.WriteInt32(redteamkillss);
                                                                counter.WriteInt32(blueteamkillss);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        ActionManager.JobKillCounterWorldID[Charname].WorldID = WorldID;
                                                        ActionManager.JobKillCounterWorldID[Charname].Kill = Kill;
                                                        // update counter
                                                        var topKillers = ActionManager.JobKillCounterWorldID.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();


                                                        int redteamkillss = 0;
                                                        int blueteamkillss = 0;
                                                        foreach (var x in topKillers)
                                                        {

                                                            if (x.Value.Team == 1)
                                                            {
                                                                redteamkillss += x.Value.Kill;
                                                            }
                                                            else if (x.Value.Team == 3)
                                                            {
                                                                blueteamkillss += x.Value.Kill;
                                                            }
                                                        }

                                                        if (topKillers.Count() > 0)
                                                        {
                                                            Packet counter = new Packet(0x189D);
                                                            counter.WriteUInt8(topKillers.Count());
                                                            foreach (var line in topKillers)
                                                            {
                                                                counter.WriteAscii(line.Value.CharName16);
                                                                counter.WriteUInt8(line.Value.Team);
                                                                counter.WriteInt32(line.Value.Kill);
                                                                counter.WriteInt32(redteamkillss);
                                                                counter.WriteInt32(blueteamkillss);
                                                            }
                                                            await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    var Create = new SCreatedJobKillCounterKillList();
                                                    Create.WorldID = WorldID;
                                                    Create.CharName16 = Charname;
                                                    Create.Kill = Kill;
                                                    Create.Team = Team;
                                                    ActionManager.JobKillCounterWorldID.TryAdd(Charname, Create);

                                                    var topKillers = ActionManager.JobKillCounterWorldID.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();

                                                    int redteamkillss = 0;
                                                    int blueteamkillss = 0;
                                                    foreach (var x in topKillers)
                                                    {

                                                        if (x.Value.Team == 1)
                                                        {
                                                            redteamkillss += x.Value.Kill;
                                                        }
                                                        else if (x.Value.Team == 3)
                                                        {
                                                            blueteamkillss += x.Value.Kill;
                                                        }
                                                    }

                                                    if (topKillers.Count() > 0)
                                                    {
                                                        Packet counter = new Packet(0x189D);
                                                        counter.WriteUInt8(topKillers.Count());
                                                        foreach (var line in topKillers)
                                                        {
                                                            counter.WriteAscii(line.Value.CharName16);
                                                            counter.WriteUInt8(line.Value.Team);
                                                            counter.WriteInt32(line.Value.Kill);
                                                            counter.WriteInt32(redteamkillss);
                                                            counter.WriteInt32(blueteamkillss);
                                                        }
                                                        await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 34)
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);
                                            if (session != null)
                                            {
                                                session.Stop("Filter command queue requested CharID disconnect (CommandID 34)");
                                                ServerManager.AgentSessions?.Remove(session);
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 35)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null)
                                    {
                                        if (int.TryParse(command.Data4, out int WorldID))
                                        {
                                            string CharnameKiller = command.Data1;
                                            string GuildName = command.Data2;
                                            string UnionName = command.Data3;
                                            //if (ServerManager.eventManager.FtwKillCounterList.ContainsKey(CharnameKiller))
                                            //{
                                            //    if (ServerManager.eventManager.FtwKillCounterList[CharnameKiller].WorldID == WorldID)
                                            //    {
                                            //        ServerManager.eventManager.FtwKillCounterList[CharnameKiller].Kill += 1;
                                            //    }
                                            //}
                                            //else
                                            //{
                                            //    var SFtwCounter = new SFortressWarCounter();
                                            //    SFtwCounter.CharName = CharnameKiller;
                                            //    SFtwCounter.GuildName = GuildName;
                                            //    SFtwCounter.UnionName = UnionName;
                                            //    SFtwCounter.WorldID = WorldID;
                                            //    SFtwCounter.Kill = 1;

                                            //    ServerManager.eventManager.FtwKillCounterList.TryAdd(CharnameKiller, SFtwCounter);
                                            //}

                                            //var topPlayerKillers = ServerManager.eventManager.FtwKillCounterList.Where(x => x.Value.WorldID == WorldID).OrderByDescending(x => x.Value.Kill).Take(5).ToList();

                                            //if (topPlayerKillers.Count() > 0)
                                            //{
                                            //    Packet counter = new Packet(0x193F);
                                            //    counter.WriteUInt8(0);
                                            //    counter.WriteUInt8(topPlayerKillers.Count());
                                            //    foreach (var line in topPlayerKillers)
                                            //    {
                                            //        counter.WriteAscii(line.Value.CharName);
                                            //        counter.WriteInt32(line.Value.Kill);
                                            //    }
                                            //    await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                            //}

                                            //var topGuildKillers = ServerManager.eventManager.FtwKillCounterList.Where(x => x.Value.WorldID == WorldID).GroupBy(x => x.Value.GuildName).Select(g => new { GuildName = g.Key, TotalKills = g.Sum(x => x.Value.Kill) }).OrderByDescending(x => x.TotalKills).Take(5).ToList();

                                            //if (topGuildKillers.Count() > 0)
                                            //{
                                            //    Packet counter = new Packet(0x193F);
                                            //    counter.WriteUInt8(1);
                                            //    counter.WriteUInt8(topGuildKillers.Count());
                                            //    foreach (var line in topGuildKillers)
                                            //    {
                                            //        counter.WriteAscii(line.GuildName); // GuildName
                                            //        counter.WriteInt32(line.TotalKills); // Total Kills
                                            //    }
                                            //    await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                            //}

                                            //// Union Kill Counter (top 5)
                                            //var topUnionKillers = ServerManager.eventManager.FtwKillCounterList.Where(x => x.Value.WorldID == WorldID).GroupBy(x => x.Value.UnionName).Select(g => new { UnionName = g.Key, TotalKills = g.Sum(x => x.Value.Kill) }).OrderByDescending(x => x.TotalKills).Take(5).ToList();

                                            //if (topUnionKillers.Count() > 0)
                                            //{
                                            //    Packet counter = new Packet(0x193F);
                                            //    counter.WriteUInt8(2);
                                            //    counter.WriteUInt8(topUnionKillers.Count());
                                            //    foreach (var line in topUnionKillers)
                                            //    {
                                            //        counter.WriteAscii(line.UnionName); // GuildName
                                            //        counter.WriteInt32(line.TotalKills); // Total Kills
                                            //    }
                                            //    await ServerManager.BroadcastPacketbyWorldID(WorldID, counter);
                                            //}
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);

                                }
                                else if (CommandID == 36)
                                {
                                    if (command.Data1 != null && command.Data2 != null && command.Data3 != null && command.Data4 != null && command.Data5 != null && command.Data6 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID) && int.TryParse(command.Data2, out int RegionID)
                                            && int.TryParse(command.Data3, out int PosX) && int.TryParse(command.Data4, out int PosY) &&
                                            int.TryParse(command.Data5, out int PosZ) && int.TryParse(command.Data6, out int Second))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);
                                            if (session != null)
                                            {
                                                Packet packet = new Packet(0x193E);
                                                packet.WriteUInt8(0);
                                                packet.WriteInt32(RegionID);
                                                packet.WriteInt32(PosX);
                                                packet.WriteInt32(PosY);
                                                packet.WriteInt32(PosZ);
                                                packet.WriteInt32(Second);
                                                await session.SendToClient(packet);
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 37)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (RefManager.ActiveNameColors.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveNameColors[command.Data1] = command.Data2;
                                        }
                                        else
                                        {
                                            RefManager.ActiveNameColors.TryAdd(command.Data1, command.Data2);
                                        }

                                        int argbInputColor = Int32.Parse(command.Data2.Replace("#", ""), NumberStyles.HexNumber);

                                        Packet stAckMsg = new Packet(0xA405);
                                        stAckMsg.WriteAscii(command.Data1);
                                        stAckMsg.WriteUInt32(argbInputColor);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                } // tested UPDATE TITLE COLOR
                                else if (CommandID == 38) /// REMOVE TITLE COLOR
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (RefManager.ActiveNameColors.ContainsKey(command.Data1))
                                        {
                                            RefManager.ActiveNameColors.TryRemove(command.Data1, out _);
                                        }
                                        Packet stAckMsg = new Packet(0xA406);
                                        stAckMsg.WriteAscii(command.Data1);
                                        await ServerManager.BroadcastPacket(stAckMsg);
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }

                                else if (CommandID == 39) /// REMOVE TITLE COLOR
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int CharID))
                                        {
                                            var session = FindAgentSessionByCharId(CharID);
                                            if (session != null)
                                            {
                                                var job = new DelayedJobItem(
                                                     250, session, null,
                                                     async (s, p) =>
                                                     {
                                                         Packet pck = new Packet(0x7061);
                                                         await session.SendToServer(pck);
                                                     });

                                                ServerManager.g_DelayedJobMgr.CreateJob(job);
                                            }
                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 40)
                                {

                                    if (command.Data1 != null)
                                    {
                                        await RefManager.RefreshRanksAndCategories();
                                        await RefManager.LoadRefAchievements();
                                        await RefManager.LoadRefAchievementsCondition();
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 41)
                                {
                                    if (!int.TryParse(command.Data1, out int charId) || charId <= 0)
                                    {
                                        Log.Warning(
                                            "Self teleport command {CommandRowId} has an invalid CharID: {CharID}",
                                            ID,
                                            command.Data1);
                                    }
                                    else
                                    {
                                        var session = ServerManager.AgentSessions!.FindByCharId(charId);

                                        if (session == null || !session.CharacterGameReady)
                                        {
                                            Log.Warning(
                                                "Self teleport skipped for CharID {CharID}: player is offline or not game-ready.",
                                                charId);
                                        }
                                        else if (TryAcquireSelfTeleportCooldown(charId))
                                        {
                                            // Re-register the per-session key first.  The initial key packet is
                                            // queued from the client GameReady handler and may reach GameServer
                                            // before the player object is fully ready, causing custom packets to
                                            // be rejected silently.
                                            Packet registerSessionKey = GameServerPacketAuthenticator.BuildRegistrationPacket(
                                                session.GameServerPacketKey,
                                                session.SessionData.UniqueCharId);
                                            await session.SendToServer(registerSessionKey);

                                            var teleportJob = new DelayedJobItem(
                                                150,
                                                session,
                                                null,
                                                async (s, p) =>
                                                {
                                                    if (s is ISession currentSession &&
                                                        currentSession.CharacterGameReady)
                                                    {
                                                        Packet selfTeleport = new Packet(0x3539, false, false);
                                                        selfTeleport.WriteAscii(currentSession.GameServerPacketKey);
                                                        await currentSession.SendToServer(selfTeleport);
                                                    }
                                                });

                                            ServerManager.g_DelayedJobMgr.CreateJob(teleportJob);
                                        }
                                        else
                                        {
                                            Log.Debug(
                                                "Self teleport throttled for CharID {CharID}; keeping the command pending.",
                                                charId);
                                            continue;
                                        }
                                    }

                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 42)
                                {
                                    if (!int.TryParse(command.Data1, out int itemRefObjId) ||
                                        itemRefObjId <= 0 ||
                                        !int.TryParse(command.Data2, out int quantity) ||
                                        quantity <= 0 ||
                                        !int.TryParse(command.Data4, out int plus) ||
                                        plus < 0 ||
                                        plus > byte.MaxValue ||
                                        !Guid.TryParse(command.Data5, out Guid batchId))
                                    {
                                        Log.Error(
                                            "Online Item Chest broadcast command {CommandRowId} has invalid data.",
                                            ID);
                                        await MarkCommandCompleteAsync(sharedConnection, ID);
                                        continue;
                                    }

                                    var source = string.IsNullOrWhiteSpace(command.Data3)
                                        ? "OnlineReward"
                                        : command.Data3.Trim();

                                    var onlinePlayers = SnapshotAgentSessions()
                                        .Where(ServerManager.IsOnlinePlayer)
                                        .Where(session => session.SessionData.Charid > 0)
                                        .GroupBy(session => session.SessionData.Charid)
                                        .Select(group => group.First())
                                        .ToArray();

                                    foreach (var onlinePlayer in onlinePlayers)
                                    {
                                        await sharedConnection.ExecuteAsync(
                                            @"EXEC dbo.Item_AddChest
                                                @CharID = @CharID,
                                                @ItemRefObjID = @ItemRefObjID,
                                                @Quantity = @Quantity,
                                                @From = @Source,
                                                @Plus = @Plus,
                                                @BatchID = @BatchID",
                                            new
                                            {
                                                BatchID = batchId,
                                                CharID = onlinePlayer.SessionData.Charid,
                                                ItemRefObjID = itemRefObjId,
                                                Quantity = quantity,
                                                Source = source,
                                                Plus = plus
                                            },
                                            commandTimeout: CommandQueryTimeoutSeconds);
                                    }

                                    Log.Information(
                                        "Online Item Chest broadcast {BatchID} queued rewards for {PlayerCount} game-ready players. ItemRefObjID={ItemRefObjID}, Quantity={Quantity}, Plus={Plus}, Source={Source}",
                                        batchId,
                                        onlinePlayers.Length,
                                        itemRefObjId,
                                        quantity,
                                        plus,
                                        source);

                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                                else if (CommandID == 1000)
                                {
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                    await RefManager.InitNPCUniqueIds();
                                    await RefManager.InitRefObjCommon();
                                    await RefManager.InitRefObjChar();
                                }
                                else if (CommandID == 9999)
                                {
                                    if (command.Data1 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int MobID))
                                        {
                                            DateTime now = DateTime.UtcNow;
                                            long unixTimestamp = (long)(now - new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc)).TotalSeconds;

                                            var history = UniqueHistoryService.SetAlive(MobID, unixTimestamp);
                                            await UniqueHistoryService.PersistAsync(history);
                                            await ServerManager.BroadcastPacket(UniqueHistoryService.CreateUpdatePacket(history));

                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);

                                }
                                else if (CommandID == 10000)
                                {
                                    if (command.Data1 != null && command.Data2 != null)
                                    {
                                        if (int.TryParse(command.Data1, out int MobID))
                                        {
                                            var history = UniqueHistoryService.FillMissingKiller(MobID, command.Data2);
                                            if (history != null)
                                            {
                                                await UniqueHistoryService.PersistAsync(history);
                                                await ServerManager.BroadcastPacket(UniqueHistoryService.CreateUpdatePacket(history));
                                            }

                                        }
                                    }
                                    await MarkCommandCompleteAsync(sharedConnection, ID);

                                }
                                else
                                {
                                    await MarkCommandCompleteAsync(sharedConnection, ID);
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            await MarkCommandFailedAsync(
                                sharedConnection, "Command_FilterQueue", command.ID,
                                command.Attempts, ex);
                            Log.Warning(ex, "Command {CommandID} row {CommandRowID} failed",
                                command.CommandID, command.ID);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                // Genel hatalari yakalayin ve loglayin, baglantiyi kapatmadan devam edin
                if (IsSqlTimeout(ex))
                    _nextCommandRetryUtc = DateTime.UtcNow.AddSeconds(CommandRetryBackoffSeconds);
                Log.Warning($"Error during OnTimerTick: {ex.Message}");
            }
            finally
            {
                _commandLock.Release();
            }
        }

        private static bool TryAcquireSelfTeleportCooldown(int charId)
        {
            long now = Stopwatch.GetTimestamp();
            RemoveExpiredSelfTeleportCooldowns(now);

            while (true)
            {
                if (!_selfTeleportCooldown.TryGetValue(charId, out long previous))
                    return _selfTeleportCooldown.TryAdd(charId, now);

                if (now - previous < SelfTeleportCooldownTicks)
                    return false;

                if (_selfTeleportCooldown.TryUpdate(charId, now, previous))
                    return true;
            }
        }

        private static void RemoveExpiredSelfTeleportCooldowns(long now)
        {
            long previousCleanup = Volatile.Read(ref _lastSelfTeleportCleanupTimestamp);
            if (now - previousCleanup < SelfTeleportCleanupIntervalTicks ||
                Interlocked.CompareExchange(
                    ref _lastSelfTeleportCleanupTimestamp,
                    now,
                    previousCleanup) != previousCleanup)
                return;

            foreach (var entry in _selfTeleportCooldown)
            {
                if (now - entry.Value >= SelfTeleportRetentionTicks)
                    _selfTeleportCooldown.TryRemove(entry);
            }
        }

        private static Timer? _plannedCommandTimer;

        public static void InitializePlannedTimer()
        {
            if (_plannedCommandTimer != null)
                return;

            _plannedCommandTimer = new Timer(3000); // 1 saniye araliklarla �alistir
            _plannedCommandTimer.Elapsed += OnPlannedTimerTick;
            _plannedCommandTimer.AutoReset = true;
            _plannedCommandTimer.Enabled = true;
        }

        public static void StopTimers()
        {
            _commandTimer?.Stop();
            _commandTimer?.Dispose();
            _plannedCommandTimer?.Stop();
            _plannedCommandTimer?.Dispose();
            timers?.Stop();
            timers?.Dispose();
            _commandTimer = null;
            _plannedCommandTimer = null;
            timers = null;
            _selfTeleportCooldown.Clear();
            Volatile.Write(ref _lastSelfTeleportCleanupTimestamp, 0);
        }

        public static void OnPlannedTimerTick(object? sender, ElapsedEventArgs e)
        {
            _ = RunPlannedCommandTimerAsync();
        }

        private static async Task RunPlannedCommandTimerAsync()
        {
            try
            {
                await ProcessPlannedCommands();
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Unhandled planned command timer failure");
            }
        }
        public static async Task ProcessPlannedCommands()
        {
            if (DateTime.UtcNow < _nextPlannedCommandRetryUtc)
                return;

            if (!await _plannedCommandLock.WaitAsync(0))
                return;

            try
            {
                DateTime currentTime = DateTime.Now;

                using (SqlConnection sharedConnection = new SqlConnection(Program.Connectionstring))
                {
                    await sharedConnection.OpenAsync();
                    // Planlanmis komutlari �ek
                    List<_AsyncFilterCommandsPlanned> pendingCommands;

                    pendingCommands = (await sharedConnection.QueryAsync<_AsyncFilterCommandsPlanned>(
                        @"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
                          ;WITH Claimable AS
                          (
                              SELECT TOP (@Take) *
                                FROM dbo.Command_PlannedQueue WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
                               WHERE DateToExecute <= @CurrentTime
                                 AND (Status = 1
                                   OR (Status = 2 AND ClaimedUtc < DATEADD(SECOND, -@LeaseSeconds, SYSUTCDATETIME()))
                                   OR (Status = 3 AND ClaimedUtc <= SYSUTCDATETIME()))
                               ORDER BY DateToExecute, ID
                          )
                          UPDATE Claimable
                             SET Status = 2,
                                 ClaimToken = NEWID(),
                                 IdempotencyKey = COALESCE(IdempotencyKey, NEWID()),
                                 ClaimedUtc = SYSUTCDATETIME(),
                                 ClaimedBy = @Worker,
                                 Attempts = Attempts + 1,
                                 LastError = NULL
                          OUTPUT INSERTED.*;",
                        new
                        {
                            CurrentTime = currentTime,
                            Take = PlannedCommandBatchSize,
                            LeaseSeconds = CommandLeaseSeconds,
                            Worker = CommandWorkerId
                        },
                        commandTimeout: CommandQueryTimeoutSeconds)).ToList();

                    // Komutlari paralel �alistir
                    foreach (var command in pendingCommands)
                    {
                        try
                        {
                            int CommandID = command.CommandID;
                            int ID = command.ID;

                            if (command.Status == 2)
                            {
                                if (CommandID == 100)
                                {
                                    await QuarantinePlannedSqlAsync(sharedConnection, command);
                                    continue;
                                }

                                if (!await TryBeginCommandExecutionAsync(
                                        sharedConnection, "Command_PlannedQueue", ID,
                                        CommandID, command.IdempotencyKey))
                                    continue;

                                // Komut t�r�ne g�re islem yap
                                switch (CommandID)
                                {
                                    case 1:
                                        if (command.Data1 != null)
                                        {
                                            if (ServerManager.AgentSessions != null)
                                            {
                                                var session = FindAgentSessionByCharName(command.Data1);
                                                if (session != null)
                                                {
                                                    session.Stop("Planned command queue requested character disconnect (CommandID 1)");
                                                    ServerManager.AgentSessions.Remove(session);
                                                }
                                            }
                                        }
                                        break;

                                    case 2:
                                        if (ServerManager.AgentSessions != null)
                                        {
                                            foreach (var line in SnapshotAgentSessions())
                                            {
                                                line.Stop("Planned command queue requested disconnect all (CommandID 2)");
                                                ServerManager.AgentSessions.Remove(line);
                                            }
                                        }
                                        break;

                                    case 3:
                                        if (command.Data1 != null && command.Data2 != null && command.Data3 != null)
                                        {
                                            if (byte.TryParse(command.Data2, out byte type))
                                            {
                                                if (command.Data1.ToLower().Contains("sendall"))
                                                {
                                                    Packet Notice = new Packet(0x168A);
                                                    Notice.WriteUInt8(type);
                                                    Notice.WriteUnicode(command.Data3); // Message
                                                    await ServerManager.BroadcastPacket(Notice);
                                                }
                                                else if (command.Data1.ToLower().Contains("sendchar") && command.Data4 != null)
                                                {
                                                    if (command.Data4.Length > 0)
                                                    {
                                                        Packet Notice = new Packet(0x168A);
                                                        Notice.WriteUInt8(type);
                                                        Notice.WriteUnicode(command.Data3); // Message
                                                        await ServerManager.BroadcastPacketToCharName(command.Data4, Notice);
                                                    }
                                                }
                                            }
                                        }
                                        break;

                                }

                                // Status g�ncelle
                                await MarkPlannedCommandCompleteAsync(sharedConnection, ID);
                            }
                        }
                        catch (Exception ex)
                        {
                            await MarkCommandFailedAsync(
                                sharedConnection, "Command_PlannedQueue", command.ID,
                                command.Attempts, ex);
                            Log.Warning(ex, "Planned command {CommandID} row {CommandRowID} failed",
                                command.CommandID, command.ID);
                        }
                    }

                    // T�m islemleri paralel tamamla
                }

            }
            catch (Exception ex)
            {
                if (IsSqlTimeout(ex))
                    _nextPlannedCommandRetryUtc = DateTime.UtcNow.AddSeconds(CommandRetryBackoffSeconds);
                Log.Warning($"Error in ProcessPlannedCommands: {ex.Message}");
            }
            finally
            {
                _plannedCommandLock.Release();
            }
        }

        private static Task MarkCommandCompleteAsync(SqlConnection connection, int id)
        {
            return connection.ExecuteAsync(
                @"UPDATE L
                     SET Result=1, CompletedUtc=SYSUTCDATETIME()
                    FROM dbo.Command_ExecutionLedger L
                    JOIN dbo.Command_FilterQueue Q ON Q.IdempotencyKey=L.IdempotencyKey
                   WHERE Q.ID=@ID AND Q.ClaimedBy=@Worker AND Q.Status=2;
                  UPDATE dbo.Command_FilterQueue
                     SET Status=0, CompletedUtc=SYSUTCDATETIME(), ClaimToken=NULL
                   WHERE ID=@ID AND ClaimedBy=@Worker AND Status=2;",
                new { ID = id, Worker = CommandWorkerId },
                commandTimeout: CommandQueryTimeoutSeconds);
        }

        private static async Task ProcessVipSilkSpendEventsAsync(SqlConnection connection)
        {
            var tableExists = await connection.ExecuteScalarAsync<int>(
                @"SELECT CASE WHEN OBJECT_ID(N'dbo.Vip_SilkSpendEvents', N'U') IS NULL
                                   THEN 0 ELSE 1 END;",
                commandTimeout: CommandQueryTimeoutSeconds);
            if (tableExists == 0)
                return;

            for (var processed = 0; processed < CommandBatchSize; processed++)
            {
                var pending = await connection.QuerySingleOrDefaultAsync<VipSilkSpendEvent>(
                    @"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
                      SELECT TOP (1) EventID, JID, SilkSpent
                      FROM dbo.Vip_SilkSpendEvents WITH (READPAST, READCOMMITTEDLOCK)
                      WHERE Status IN (0, 2)
                        AND NextAttemptAt <= SYSUTCDATETIME()
                      ORDER BY EventID;",
                    commandTimeout: CommandQueryTimeoutSeconds);
                if (pending == null)
                    return;

                var session = ServerManager.AgentSessions
                    .FirstOrDefault(candidate =>
                        ServerManager.IsOnlinePlayer(candidate) &&
                        candidate.SessionData.JID == pending.JID);
                if (session == null)
                {
                    await connection.ExecuteAsync(
                        @"UPDATE dbo.Vip_SilkSpendEvents
                          SET NextAttemptAt = DATEADD(SECOND, 5, SYSUTCDATETIME())
                          WHERE EventID = @EventID AND Status IN (0, 2);",
                        new { pending.EventID },
                        commandTimeout: CommandQueryTimeoutSeconds);
                    continue;
                }

                await using var transaction =
                    (SqlTransaction)await connection.BeginTransactionAsync(
                        System.Data.IsolationLevel.ReadCommitted);
                try
                {
                    var claimed = await connection.QuerySingleOrDefaultAsync<VipSilkSpendEvent>(
                        @"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
                          SELECT EventID, JID, SilkSpent
                          FROM dbo.Vip_SilkSpendEvents WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
                          WHERE EventID = @EventID AND Status IN (0, 2);",
                        new { pending.EventID },
                        transaction,
                        commandTimeout: CommandQueryTimeoutSeconds);
                    if (claimed == null)
                    {
                        await transaction.CommitAsync();
                        continue;
                    }

                    await connection.ExecuteAsync(
                        @"EXEC dbo.Hook_ItemMallBuy
                              @JID = @JID,
                              @CharID = @CharID,
                              @CharName16 = @CharName,
                              @ItemID = 0,
                              @Silk = @SilkSpent,
                              @SourceEventID = @EventID;",
                        new
                        {
                            claimed.EventID,
                            claimed.JID,
                            CharID = session.SessionData.Charid,
                            CharName = session.SessionData.Charname,
                            claimed.SilkSpent
                        },
                        transaction,
                        commandTimeout: 60);

                    await connection.ExecuteAsync(
                        @"UPDATE dbo.Vip_SilkSpendEvents
                          SET Status = 1,
                              AttemptCount = AttemptCount + 1,
                              CharID = @CharID,
                              CharName16 = @CharName,
                              ProcessedAt = SYSUTCDATETIME(),
                              LastError = NULL
                          WHERE EventID = @EventID;",
                        new
                        {
                            claimed.EventID,
                            CharID = session.SessionData.Charid,
                            CharName = session.SessionData.Charname
                        },
                        transaction,
                        commandTimeout: CommandQueryTimeoutSeconds);

                    await transaction.CommitAsync();
                }
                catch (Exception ex)
                {
                    await transaction.RollbackAsync();
                    await connection.ExecuteAsync(
                        @"UPDATE dbo.Vip_SilkSpendEvents
                          SET Status = CASE WHEN AttemptCount + 1 >= 10 THEN 2 ELSE 0 END,
                              AttemptCount = AttemptCount + 1,
                              NextAttemptAt = CASE
                                  WHEN AttemptCount + 1 >= 10
                                      THEN DATEADD(MINUTE, 15, SYSUTCDATETIME())
                                  ELSE DATEADD(SECOND, 15, SYSUTCDATETIME())
                              END,
                              LastError = LEFT(@Error, 1000)
                          WHERE EventID = @EventID;",
                        new { pending.EventID, Error = ex.Message },
                        commandTimeout: CommandQueryTimeoutSeconds);
                    Log.Warning(
                        ex,
                        "VIP Silk spend event {EventID} failed for JID {JID}",
                        pending.EventID,
                        pending.JID);
                }
            }
        }

        private static bool TryParseCommandState(string value, out bool state)
        {
            value = value.Trim();
            if (value == "1")
            {
                state = true;
                return true;
            }

            if (value == "0")
            {
                state = false;
                return true;
            }

            return bool.TryParse(value, out state);
        }

        private static ISession[] SnapshotAgentSessions()
        {
            return ServerManager.AgentSessions?.ToArray() ?? Array.Empty<ISession>();
        }

        private static ISession? FindAgentSessionByCharId(int charId)
        {
            return ServerManager.AgentSessions.FindByCharId(charId);
        }

        private static ISession? FindAgentSessionByCharName(string charName)
        {
            return ServerManager.AgentSessions.FindByCharName(charName);
        }

        private static async Task MarkPlannedCommandCompleteAsync(int id)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await MarkPlannedCommandCompleteAsync(connection, id);
        }

        private static Task MarkPlannedCommandCompleteAsync(SqlConnection connection, int id)
        {
            return connection.ExecuteAsync(
                @"UPDATE L
                     SET Result=1, CompletedUtc=SYSUTCDATETIME()
                    FROM dbo.Command_ExecutionLedger L
                    JOIN dbo.Command_PlannedQueue Q ON Q.IdempotencyKey=L.IdempotencyKey
                   WHERE Q.ID=@ID AND Q.ClaimedBy=@Worker AND Q.Status=2;
                  UPDATE dbo.Command_PlannedQueue
                     SET Status=0, CompletedUtc=SYSUTCDATETIME(), ClaimToken=NULL
                   WHERE ID=@ID AND ClaimedBy=@Worker AND Status=2;",
                new { ID = id, Worker = CommandWorkerId },
                commandTimeout: CommandQueryTimeoutSeconds);
        }

        private static async Task<bool> TryBeginCommandExecutionAsync(
            SqlConnection connection,
            string sourceQueue,
            int sourceId,
            int commandId,
            Guid? idempotencyKey)
        {
            if (!idempotencyKey.HasValue)
                throw new InvalidOperationException("Claimed command has no idempotency key.");

            var inserted = await connection.ExecuteAsync(
                @"INSERT dbo.Command_ExecutionLedger
                      (IdempotencyKey,SourceQueue,SourceID,CommandID,StartedUtc,Result)
                  SELECT @Key,@Queue,@SourceID,@CommandID,SYSUTCDATETIME(),0
                   WHERE NOT EXISTS
                         (SELECT 1 FROM dbo.Command_ExecutionLedger WITH (UPDLOCK,HOLDLOCK)
                           WHERE IdempotencyKey=@Key);",
                new
                {
                    Key = idempotencyKey.Value,
                    Queue = sourceQueue,
                    SourceID = sourceId,
                    CommandID = commandId
                },
                commandTimeout: CommandQueryTimeoutSeconds);
            if (inserted == 1)
                return true;

            var result = await connection.ExecuteScalarAsync<byte>(
                "SELECT Result FROM dbo.Command_ExecutionLedger WHERE IdempotencyKey=@Key",
                new { Key = idempotencyKey.Value },
                commandTimeout: CommandQueryTimeoutSeconds);
            if (result == 1)
            {
                await MarkQueueAlreadyCompletedAsync(connection, sourceQueue, sourceId);
                return false;
            }

            // A lease reclaimed after execution started cannot prove whether an
            // external game/financial effect committed. Dead-lettering prevents
            // the same effect from being issued twice and requires reconciliation.
            await MarkQueueDeadLetterAsync(
                connection, sourceQueue, sourceId,
                "Execution ledger is incomplete after lease recovery; manual reconciliation required.");
            return false;
        }

        private static Task MarkQueueAlreadyCompletedAsync(
            SqlConnection connection, string sourceQueue, int sourceId)
        {
            var table = GetQueueTable(sourceQueue);
            return connection.ExecuteAsync(
                $@"UPDATE dbo.{table}
                       SET Status=0, CompletedUtc=COALESCE(CompletedUtc,SYSUTCDATETIME()), ClaimToken=NULL
                     WHERE ID=@ID AND ClaimedBy=@Worker AND Status=2;",
                new { ID = sourceId, Worker = CommandWorkerId },
                commandTimeout: CommandQueryTimeoutSeconds);
        }

        private static Task MarkQueueDeadLetterAsync(
            SqlConnection connection, string sourceQueue, int sourceId, string error)
        {
            var table = GetQueueTable(sourceQueue);
            return connection.ExecuteAsync(
                $@"UPDATE dbo.{table}
                       SET Status=4, CompletedUtc=SYSUTCDATETIME(), ClaimToken=NULL, LastError=@Error
                     WHERE ID=@ID AND ClaimedBy=@Worker AND Status=2;",
                new
                {
                    ID = sourceId,
                    Worker = CommandWorkerId,
                    Error = error.Length <= 1000 ? error : error[..1000]
                },
                commandTimeout: CommandQueryTimeoutSeconds);
        }

        private static Task MarkCommandFailedAsync(
            SqlConnection connection,
            string sourceQueue,
            int sourceId,
            int attempts,
            Exception exception)
        {
            var table = GetQueueTable(sourceQueue);
            var error = $"{exception.GetType().Name}: {exception.Message}";
            if (error.Length > 1000)
                error = error[..1000];
            var deadLetter = attempts >= CommandMaxAttempts;
            return connection.ExecuteAsync(
                $@"UPDATE dbo.{table}
                       SET Status=@Status,
                           ClaimedUtc=CASE WHEN @Status=3
                                           THEN DATEADD(SECOND,@BackoffSeconds,SYSUTCDATETIME())
                                           ELSE ClaimedUtc END,
                           CompletedUtc=CASE WHEN @Status=4 THEN SYSUTCDATETIME() ELSE NULL END,
                           ClaimToken=NULL,
                           LastError=@Error
                     WHERE ID=@ID AND ClaimedBy=@Worker AND Status=2;",
                new
                {
                    ID = sourceId,
                    Worker = CommandWorkerId,
                    Status = deadLetter ? 4 : 3,
                    BackoffSeconds = CommandRetryBackoffSeconds,
                    Error = error
                },
                commandTimeout: CommandQueryTimeoutSeconds);
        }

        private static async Task QuarantinePlannedSqlAsync(
            SqlConnection connection, _AsyncFilterCommandsPlanned command)
        {
            await connection.ExecuteAsync(
                @"INSERT dbo.Command_Quarantine(SourceQueue,SourceID,CommandID,RawCommand,Reason)
                  SELECT N'Command_PlannedQueue',@ID,@CommandID,@RawCommand,
                         N'Free-form SQL commands are disabled in KMTGuard v3.'
                   WHERE NOT EXISTS
                         (SELECT 1 FROM dbo.Command_Quarantine
                           WHERE SourceQueue=N'Command_PlannedQueue' AND SourceID=@ID);
                  UPDATE dbo.Command_PlannedQueue
                     SET Status=4, CompletedUtc=SYSUTCDATETIME(), ClaimToken=NULL,
                         LastError=N'Quarantined: free-form SQL is disabled.'
                   WHERE ID=@ID AND ClaimedBy=@Worker AND Status=2;",
                new
                {
                    command.ID,
                    command.CommandID,
                    RawCommand = command.Data1,
                    Worker = CommandWorkerId
                },
                commandTimeout: CommandQueryTimeoutSeconds);
            Log.Warning(
                "Planned free-form SQL row {CommandRowID} was quarantined without execution",
                command.ID);
        }

        private static string GetQueueTable(string sourceQueue) => sourceQueue switch
        {
            "Command_FilterQueue" => "Command_FilterQueue",
            "Command_PlannedQueue" => "Command_PlannedQueue",
            _ => throw new ArgumentOutOfRangeException(nameof(sourceQueue))
        };

        private static bool IsSqlTimeout(Exception ex)
        {
            if (ex is SqlException sqlException && sqlException.Number == -2)
                return true;

            return ex.Message.IndexOf("timeout", StringComparison.OrdinalIgnoreCase) >= 0;
        }

        private sealed class VipSilkSpendEvent
        {
            public long EventID { get; set; }
            public int JID { get; set; }
            public int SilkSpent { get; set; }
        }

    }
}
