using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Database.ModelsEvents;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Features.UniqueHistory;
using KMTGuard.Features.Telegram;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Microsoft.IdentityModel.Tokens;
using Serilog;
using SilkroadSecurityAPI;
using static System.Net.Mime.MediaTypeNames;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class CustomGameServerPacketHandler
    {
        private static readonly ConcurrentDictionary<int, long> _lastHwidListUpdateTicks = new();
        private static readonly long HwidListUpdateCooldownTicks =
            (long)(Stopwatch.Frequency * 15.0);

        private AgentServer AgentServer { get; set; }
        public CustomGameServerPacketHandler(AgentServer agentServer, IPacketHandler packetHandler)
        {
            AgentServer = agentServer;
            packetHandler.RegisterModuleHandler(0x5038, TARGET_PLAYER_ITEM_INFO);

            packetHandler.RegisterModuleHandler(0xB034, SERVER_ITEM_MOVE);
            packetHandler.RegisterClientHandler(0x7034, CLIENT_ITEM_MOVE);

            packetHandler.RegisterModuleHandler(0x30BF, SERVER_ENTITY_STATE_UPDATE);
            packetHandler.RegisterModuleHandler(0x5013, SERVER_KILL_LOGGER);
            packetHandler.RegisterModuleHandler(0x5010, UNIQUE_DPS);
            packetHandler.RegisterModuleHandler(0x5016, FORTRESS_DPS);
            packetHandler.RegisterModuleHandler(0x5014, SERVER_MOB_KILL_LOGGER);
            packetHandler.RegisterModuleHandler(0x385F, SERVER_FORTRESS_UPDATE);
            packetHandler.RegisterModuleHandler(0x5030, SERVER_ITEM_LOCK_INFO_LOCKED);
            packetHandler.RegisterModuleHandler(0x5031, SERVER_ITEM_LOCK_INFO_UNLOCKED);
            packetHandler.RegisterModuleHandler(0x5032, SERVER_ITEM_LOCK_RESULT);
            packetHandler.RegisterModuleHandler(0x5035, GET_POS_INFO_FROM_GS_EVERY_TELEPORT);
            packetHandler.RegisterModuleHandler(0x3571, SERVER_REGION_HAS_CHANGED);
            packetHandler.RegisterModuleHandler(0x3572, SERVER_ITEM_REGION_TRAVEL_BLOCKED);

        }

        private static async Task<PacketResult> SERVER_ITEM_REGION_TRAVEL_BLOCKED(
            Packet packet, ISession session, object obj)
        {
            if (packet.RemainingRead() == 0)
                await ServerManager.sendNotice(
                    session, NoticeType.WARNING, PlayerLanguage.Get("Region.ItemDisabled"));
            return new PacketResult(packet, PacketResultType.Block);
        }

        private static async Task<PacketResult> SERVER_FORTRESS_UPDATE(Packet packet, ISession session, object obj)
        {
            try
            {
                if (packet.GetBytes().Length < 1)
                    return new PacketResult(packet, PacketResultType.Nothing);

                byte updateType = packet.ReadUInt8();
                if (updateType == 8)
                {
                    int fortressId = packet.ReadInt32();
                    string guildName = packet.ReadAscii();
                    await TelegramNotificationService.QueueFortressUpdateAsync(updateType, fortressId, guildName);
                }
                else if (updateType is 1 or 2 or 6)
                {
                    await TelegramNotificationService.QueueFortressUpdateAsync(updateType);
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Telegram Fortress observation failed; original 0x385F packet was preserved");
            }

            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private async Task<PacketResult> SERVER_REGION_HAS_CHANGED(Packet packet, ISession session, object obj)
        {
            try
            {
                int previousRegion = session.SessionData.LatestRegion;
                bool leftTimedRegion = ActionManager.CreatedTimerListRegionID.ContainsKey(previousRegion);
                session.SessionData.LatestRegion = RegionControlService.NormalizeRegionId(packet.ReadUInt16());
                RegionControlService.MarkMovement(session);
                await RegionControlService.ApplyAutoPvpAsync(session, force: true);
                QueueEventSuitSnapshot(session);
                if (leftTimedRegion || ActionManager.CreatedTimerListRegionID.ContainsKey(session.SessionData.LatestRegion))
                    await SyncPersistentTimerAsync(session, leftTimedRegion);
                QueueHwidListUpdate(session);
            }
            catch (Exception EX)
            {
                Log.Fatal(EX.Message.ToString() + $"SERVER_REGION_HAS_CHANGED");
            }

            return new PacketResult(packet, PacketResultType.Block);
        }

        private static async Task SyncPersistentTimerAsync(ISession session, bool leftTimedRegion)
        {
            if (ActionManager.CreatedTimerListRegionID.TryGetValue(
                    session.SessionData.LatestRegion,
                    out int regionRemainingMilliseconds) &&
                regionRemainingMilliseconds > 0)
            {
                Packet regionTimer = new Packet(0x220A);
                regionTimer.WriteUInt8(0);
                regionTimer.WriteInt32(regionRemainingMilliseconds);
                await session.SendToClient(regionTimer);
                return;
            }

            if (ActionManager.CreatedTimerListWorldID.TryGetValue(
                    session.SessionData.WorldID,
                    out int worldRemainingMilliseconds) &&
                worldRemainingMilliseconds > 0)
            {
                Packet worldTimer = new Packet(0x220A);
                worldTimer.WriteUInt8(0);
                worldTimer.WriteInt32(worldRemainingMilliseconds);
                await session.SendToClient(worldTimer);
                return;
            }

            if (leftTimedRegion)
            {
                Packet closeTimer = new Packet(0x220A);
                closeTimer.WriteUInt8(1);
                await session.SendToClient(closeTimer);
            }
        }

        private async Task<PacketResult> GET_POS_INFO_FROM_GS_EVERY_TELEPORT(Packet packet, ISession session, object obj)
        {
            try
            {
                const int fixedPositionPayloadSize = sizeof(ushort) + sizeof(ushort) + sizeof(byte) + sizeof(int);
                if (packet.RemainingRead() < fixedPositionPayloadSize)
                {
                    Log.Warning(
                        "Blocked short teleport-position packet. Char={CharName} Opcode=0x{Opcode:X4} Remaining={Remaining}",
                        session.SessionData.Charname,
                        packet.Opcode,
                        packet.RemainingRead());
                    return new PacketResult(packet, PacketResultType.Block);
                }

                int previousRegion = session.SessionData.LatestRegion;
                bool leftTimedRegion = ActionManager.CreatedTimerListRegionID.ContainsKey(previousRegion);
                session.SessionData.WorldID = packet.ReadUInt16();
                session.SessionData.WorldLayerID = packet.ReadUInt16();
                session.SessionData.JobType = packet.ReadUInt8();
                session.SessionData.LatestRegion = RegionControlService.NormalizeRegionId(packet.ReadInt32());
                RegionControlService.MarkMovement(session);
                await PvpChallengeService.HandleRegionChangedAsync(
                    session,
                    session.SessionData.WorldID,
                    session.SessionData.LatestRegion);
                await RegionControlService.ApplyAutoPvpAsync(session, force: true);
                QueueEventSuitSnapshot(session);

                if (session.SessionData.JobType != 4)
                {
                    session.SessionData.JobName = packet.ReadAscii();
                }

                QueueHwidListUpdate(session);
                if (!session.IsManagedClientless && _serverSettings.IPLimit > 0)
                {
                    if (!string.IsNullOrEmpty(session.ClientIp))
                    {
                        if (RefManager.m_bypassHwidbyIP.ContainsKey(session.ClientIp))
                        {
                            int PC_CNT = await ServerManager.GetIPCount(session.ClientIp);
                            if (PC_CNT > RefManager.m_bypassHwidbyIP[session.ClientIp])
                            {
                                string noticeMessage = RefManager.GetNoticeMessage("IP_LIMIT_REACHED");
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Disconnect);
                            }
                        }
                        else
                        {
                            int PC_CNT = await ServerManager.GetIPCount(session.ClientIp);
                            if (PC_CNT > _serverSettings.IPLimit)
                            {
                                string noticeMessage = RefManager.GetNoticeMessage("IP_LIMIT_REACHED");
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Disconnect);
                            }
                        }
                    }
                    else
                    {
                        return new PacketResult(PacketResultType.Disconnect);
                    }
                }
                if (!session.IsManagedClientless && _serverSettings.HWID_LIMIT > 0)
                {
                    if (!string.IsNullOrEmpty(session.ClientIp) && !string.IsNullOrEmpty(session.SessionData.Hwid))
                    {
                        if (RefManager.m_bypassHwidbyIP.ContainsKey(session.ClientIp))
                        {
                            int PC_CNT = await ServerManager.GetHWIDCount(session.SessionData.Hwid);
                            if (PC_CNT > RefManager.m_bypassHwidbyIP[session.ClientIp])
                            {
                                string noticeMessage = RefManager.GetNoticeMessage("HWID_LIMIT_REACHED");
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Disconnect);
                            }
                        }
                        else
                        {
                            int PC_CNT = await ServerManager.GetHWIDCount(session.SessionData.Hwid);
                            if (PC_CNT > _serverSettings.HWID_LIMIT)
                            {
                                string noticeMessage = RefManager.GetNoticeMessage("HWID_LIMIT_REACHED");
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Disconnect);
                            }
                        }
                    }
                    else
                    {
                        return new PacketResult(PacketResultType.Disconnect);
                    }
                }
                if (!session.IsManagedClientless && _serverSettings.HWID_JOB_LIMIT > 0)
                {
                    if (!string.IsNullOrEmpty(session.ClientIp) && !string.IsNullOrEmpty(session.SessionData.Hwid))
                    {
                        int PC_CNT = await ServerManager.GetJobHWIDCount(session.SessionData.Hwid);
                        if (PC_CNT > _serverSettings.HWID_JOB_LIMIT)
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("HWID_LIMIT_REACHED_JOB");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Disconnect);
                        }
                    }
                    else
                    {
                        return new PacketResult(PacketResultType.Disconnect);
                    }
                }
                #region MapSettings
                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                {
                    if ((RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].HideName ||
                        RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableParty))
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


                    if (!PvpChallengeService.IsCapeControlled(session.SessionData.Charid) && RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].AutoCape && session.SessionData.JobType == 4 && RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].RegionType == 0)
                    {
                        var job = new DelayedJobItem(
                             1000, session, null,
                             async (s, p) =>
                             {
                                 Packet pck = new Packet(0x3502);
                                 pck.WriteAscii(session.GameServerPacketKey);
                                 pck.WriteUInt8(5);
                                 await session.SendToServer(pck);
                             });

                        ServerManager.g_DelayedJobMgr.CreateJob(job);
                    }
                    else if (!PvpChallengeService.IsCapeControlled(session.SessionData.Charid) && RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].AutoCape && session.SessionData.JobType == 4 && RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].RegionType == 1)
                    {
                        using (var connection = new SqlConnection(Program.Connectionstring))
                        {
                            await connection.OpenAsync();
                            var data = await connection.QueryFirstOrDefaultAsync<int>(
                                "SELECT Team FROM [dbo].[Event_CurrentTeams] WITH (NOLOCK) WHERE CharID = @CharID",
                                new { CharID = session.SessionData.Charid });
                            if (data > 0 && data < 5)
                            {
                                var job = new DelayedJobItem(
                                 1000, session, null,
                                 async (s, p) =>
                                 {
                                     Packet pck = new Packet(0x3502);
                                     pck.WriteAscii(session.GameServerPacketKey);
                                     pck.WriteUInt8(data);
                                     await session.SendToServer(pck);
                                 });

                                ServerManager.g_DelayedJobMgr.CreateJob(job);
                            }
                        }
                    }
                }
                else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                {
                    if ((RefManager.m_RefEventMapSettings[session.SessionData.WorldID].HideName || RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisableParty))
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
                    if (!PvpChallengeService.IsCapeControlled(session.SessionData.Charid) && RefManager.m_RefEventMapSettings[session.SessionData.WorldID].AutoCape && session.SessionData.JobType == 4 && RefManager.m_RefEventMapSettings[session.SessionData.WorldID].RegionType == 0)
                    {
                        var job = new DelayedJobItem(
                             1000, session, null,
                             async (s, p) =>
                             {
                                 Packet pck = new Packet(0x3502);
                                 pck.WriteAscii(session.GameServerPacketKey);
                                 pck.WriteUInt8(5);
                                 await session.SendToServer(pck);
                             });

                        ServerManager.g_DelayedJobMgr.CreateJob(job);
                    }
                    else if (!PvpChallengeService.IsCapeControlled(session.SessionData.Charid) && RefManager.m_RefEventMapSettings[session.SessionData.WorldID].AutoCape && session.SessionData.JobType == 4 && RefManager.m_RefEventMapSettings[session.SessionData.WorldID].RegionType == 1)
                    {
                        using (var connection = new SqlConnection(Program.Connectionstring))
                        {
                            await connection.OpenAsync();
                            var data = await connection.QueryFirstOrDefaultAsync<int>(
                                "SELECT Team FROM [dbo].[Event_CurrentTeams] WITH (NOLOCK) WHERE CharID = @CharID",
                                new { CharID = session.SessionData.Charid });
                            if (data > 0 && data < 5)
                            {
                                var job = new DelayedJobItem(
                                 1000, session, null,
                                 async (s, p) =>
                                 {
                                     Packet pck = new Packet(0x3502);
                                     pck.WriteAscii(session.GameServerPacketKey);
                                     pck.WriteUInt8(data);
                                     await session.SendToServer(pck);
                                 });

                                ServerManager.g_DelayedJobMgr.CreateJob(job);
                            }
                        }
                    }
                }
                #endregion

                #region Counters and timers
                await SyncPersistentTimerAsync(session, leftTimedRegion);
                if (ActionManager.CreatedKillCounterWorldID.ContainsKey(session.SessionData.WorldID))
                {
                    Packet timer = new Packet(0x207A);
                    timer.WriteUInt8(1);
                    timer.WriteAscii(ActionManager.CreatedKillCounterWorldID[session.SessionData.WorldID]);
                    await session.SendToClient(timer);

                    int WorldID = session.SessionData.WorldID;

                    var orderedKillers = ActionManager.KillCounterKillList
                        .Where(x => x.Value.WorldID == WorldID)
                        .OrderByDescending(x => x.Value.Kill)
                        .ThenBy(x => x.Value.CharName16);
                    var topKillers = ActionManager.CreatedFullKillCounterWorldID.ContainsKey(WorldID)
                        ? orderedKillers.ToList()
                        : orderedKillers.Take(5).ToList();


                    if (topKillers.Count() > 0)
                    {
                        Packet counter = new Packet(0x207C);
                        counter.WriteUInt8(topKillers.Count());
                        foreach (var line in topKillers)
                        {
                            counter.WriteAscii(line.Value.CharName16);
                            counter.WriteInt32(line.Value.Kill);
                        }
                        await session.SendToClient(counter);
                    }
                }

                if (ActionManager.CreatedTeamKillCounterWorldID.ContainsKey(session.SessionData.WorldID))
                {
                    Packet timer = new Packet(0x189A, false, false);
                    timer.WriteUInt8(1);
                    timer.WriteAscii(ActionManager.CreatedTeamKillCounterWorldID[session.SessionData.WorldID]);
                    await session.SendToClient(timer);

                    int WorldID = session.SessionData.WorldID;

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
                        await session.SendToClient(counter);
                    }
                }

                if (ActionManager.CreatedJobKillCounterWorldID.ContainsKey(session.SessionData.WorldID))
                {
                    Packet timer = new Packet(0x189A, false, false);
                    timer.WriteUInt8(1);
                    timer.WriteAscii(ActionManager.CreatedJobKillCounterWorldID[session.SessionData.WorldID]);
                    await session.SendToClient(timer);

                    int WorldID = session.SessionData.WorldID;

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
                        await session.SendToClient(counter);
                    }
                }
                #endregion
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, GET_POS_INFO_FROM_GS", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
            return new PacketResult(packet, PacketResultType.Block);
        }

        #region ITEM LOCK AND PLAYER INFO
        private async Task<PacketResult> TARGET_PLAYER_ITEM_INFO(Packet packet, ISession session, object obj)
        {
            try
            {
                byte ItemSlot = packet.ReadUInt8();

                int Len = packet.ReadInt32();
                List<byte> Bytes = new();
                for (int i = 0; i < Len; i++)
                {
                    byte p = packet.ReadUInt8();
                    Bytes.Add(p);
                }

                int ItemID = packet.ReadInt32();
                string SenderName = packet.ReadAscii();
                var pSession = ServerManager.AgentSessions.FindByCharName(SenderName);
                if (pSession != null)
                {
                    Packet itemlink = new Packet(0x5039);
                    itemlink.WriteUInt8(ItemSlot);
                    itemlink.WriteInt32(Len);
                    foreach (var data in Bytes)
                    {
                        itemlink.WriteUInt8(data);
                    }
                    itemlink.WriteInt32(ItemID);
                    itemlink.WriteAscii(session.SessionData.Charname); /// its my charname to sender
                    await pSession.SendToClient(itemlink);
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, TARGET_PLAYER_ITEM_INFO", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
            return new PacketResult(packet, PacketResultType.Block);
        }

        private async Task<PacketResult> SERVER_ITEM_LOCK_INFO_LOCKED(Packet packet, ISession session, object obj)
        {
            try
            {

                int LockedItemSlot = packet.ReadInt32();
                Int64 id64 = packet.ReadInt64();

                Packet p = new Packet(0xF200);
                p.WriteUInt8(0);
                p.WriteUInt8(LockedItemSlot);
                await session.SendToClient(p);




                string noticeMessage = RefManager.GetNoticeMessage("MSG_LOCK_SUCCESS");
                Packet stMsg = new Packet(0xF201);
                stMsg.WriteUnicode(noticeMessage);
                stMsg.WriteUInt8(LockedItemSlot);
                await session.SendToClient(stMsg);

            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_LOCK_ITEM", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
            return new PacketResult(packet, PacketResultType.Block);
        }
        private async Task<PacketResult> SERVER_ITEM_LOCK_INFO_UNLOCKED(Packet packet, ISession session, object obj)
        {
            try
            {
                int UnlockedItemSlot = packet.ReadInt32();
                Int64 id64 = packet.ReadInt64();


                Packet p = new Packet(0x5028);
                p.WriteUInt8(UnlockedItemSlot);
                await session.SendToClient(p);

                string noticeMessage = RefManager.GetNoticeMessage("MSG_ITEM_UNLOCKED");
                Packet stMsg = new Packet(0xF201);
                stMsg.WriteUnicode(noticeMessage);
                stMsg.WriteUInt8(UnlockedItemSlot);
                await session.SendToClient(stMsg);


            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_ITEM_LOCK_INFO_UNLOCKED", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
            return new PacketResult(packet, PacketResultType.Block);
        }
        #endregion


        #region new work t0p
        private async Task<PacketResult> SERVER_ITEM_MOVE(Packet packet, ISession session, object obj)
        {
            try
            {
                byte succeed = packet.ReadUInt8();
                if (succeed == 1)
                {
                    byte ActionTybe = packet.ReadUInt8();


                    #region trade triggers

                    // buy trade good BUY COMPLET complet  ??? there no complet yet
                    if (ActionTybe == 0x13)
                    {
                        int petuniqueid = packet.ReadInt32();   // pet uniqueid (not used in SQL)
                        byte npctab = packet.ReadUInt8();       // SHOP TAB
                        byte npcslot = packet.ReadUInt8();      // NPC SLOT
                        byte unk = packet.ReadUInt8();          // unknown
                        byte Petslot = packet.ReadUInt8();      // to slot in pet
                        int count = packet.ReadInt16();         // count

                        int npcuniqueid = (int)session.SessionData.SELECTEDUNIQUEID;

                        if (session.SessionData.PetDataDictionary.ContainsKey((uint)petuniqueid))
                        {
                            var petInfo = session.SessionData.PetDataDictionary[(uint)petuniqueid];
                            uint refObjID = petInfo.RefObjID;

                            try
                            {
                                if (RefManager._GSNPCList.TryGetValue(npcuniqueid, out var npcData) && npcData != null)
                                {
                                    int npcid = npcData.RefObjId;
                                    string codeName128 = npcData.CodeName128;

                                    await TriggerService.CompleteTradeGoodsBuying(
                                        session.SessionData.Charid,
                                        session.SessionData.Charname,
                                        (int)refObjID,
                                        npcid,
                                        codeName128,
                                        npctab,
                                        npcslot,
                                        (short)count,
                                        session.SessionData.JobType,
                                        session.SessionData.Hwid);
                                }
                                else
                                {
                                    return new PacketResult(packet, PacketResultType.Nothing);
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error($"Error in buying trade items: {ex.Message}");
                            }
                        }
                    }

                    // sell trade items request
                    if (ActionTybe == 0x14)
                    {
                        int petuniqueid = packet.ReadInt32();  // petuniqueid
                        byte petslot = packet.ReadUInt8();  // SELLING SLOT
                        int count = packet.ReadInt16();  // SELLING COUNT
                        int npcuniqueid = packet.ReadInt32(); // SELL UNIIQUEID NPC
                        byte unk = packet.ReadUInt8();  // SELLING SLOT

                        if (session.SessionData.PetDataDictionary.ContainsKey((uint)petuniqueid))
                        {
                            var petInfo = session.SessionData.PetDataDictionary[(uint)petuniqueid];
                            uint refObjID = petInfo.RefObjID;
                            try
                            {
                                if (RefManager._GSNPCList.TryGetValue(npcuniqueid, out var npcData) && npcData != null)
                                {
                                    int npcid = npcData.RefObjId;
                                    string codeName128 = npcData.CodeName128;

                                    await TriggerService.CompleteTradeGoodsSelling(
                                        session.SessionData.Charid,
                                        session.SessionData.Charname,
                                        (int)petInfo.RefObjID,
                                        npcid,
                                        codeName128,
                                        petslot,
                                        (short)count,
                                        session.SessionData.JobType,
                                        session.SessionData.Hwid);
                                }
                                else
                                {
                                    return new PacketResult(packet, PacketResultType.Nothing);
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error($"Error in selling trade items: {ex.Message}");
                            }
                        }
                    }

                    #endregion

                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_ITEM_MOVE", ConsoleColor.Red);
            }
            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private async Task<PacketResult> CLIENT_ITEM_MOVE(Packet packet, ISession session, object obj)
        {
            try
            {
                byte[] rawPacket = packet.GetBytes();
                if (rawPacket.Length >= 1)
                {
                    byte ActionTybe = packet.ReadUInt8();

                    // Inventory move type 0: [from slot][to slot][quantity].
                    // Equipment slot 8 is the Job suit slot.
                    if (ActionTybe == 0x00 && rawPacket.Length >= 5)
                    {
                        byte fromSlot = packet.ReadUInt8();
                        byte toSlot = packet.ReadUInt8();
                        packet.ReadUInt16();

                        if (toSlot == 8 && fromSlot >= 13)
                        {
                            var suitInfo = new SItemInfoDbRecord();
                            bool itemResolved = await AgentServer.TryGetItemInfoAsync(
                                suitInfo,
                                session.SessionData.Charid,
                                fromSlot);

                            if (!itemResolved ||
                                !await JobControlService.CanEquipSuitAsync(
                                    session,
                                    suitInfo.nRefItemID))
                            {
                                return new PacketResult(packet, PacketResultType.Block);
                            }
                        }
                        else if (fromSlot == 8 && toSlot >= 13)
                        {
                            if (!await JobControlService.CanRemoveSuitAsync(session))
                                return new PacketResult(packet, PacketResultType.Block);
                        }
                    }

                    if (ActionTybe == 0x13 || ActionTybe == 0x14)
                    {
                        var botTradeBlock = await BotProtectionService.BlockBotTradeIfDisabledAsync(
                            session,
                            ActionTybe == 0x13 ? "trade goods purchase" : "trade goods sale");
                        if (botTradeBlock != null)
                            return botTradeBlock;
                    }

                    #region trade triggers

                    // buy trade good request
                    if (ActionTybe == 0x13)
                    {
                        int unk1 = packet.ReadInt32();   // empty
                        byte npctab = packet.ReadUInt8(); // I THINK THIS IS SHOP TAB
                        byte npcslot = packet.ReadUInt8(); // NPC SLOT START FROM 0
                        int count = packet.ReadInt16();
                        int uniqueid = packet.ReadInt32();  // uniqueid used in SQL query

                        if (RefManager._GSNPCList.TryGetValue(uniqueid, out var npcData) && npcData != null)
                        {
                            int npcid = npcData.RefObjId;
                            string codeName128 = npcData.CodeName128;

                            bool isBlocked = await TriggerService.ControlTradeGoodsBuyingRequest(
                                session.SessionData.Charid, session.SessionData.Charname,
                                npcid, codeName128, npctab, npcslot, (short)count,
                                session.SessionData.JobType, session.SessionData.Hwid);

                            if (isBlocked)
                                return new PacketResult(packet, PacketResultType.Block);
                        }
                        else
                        {
                            return new PacketResult(packet, PacketResultType.Nothing);
                        }
                    }

                    // sell trade items request
                    if (ActionTybe == 0x14)
                    {
                        int petuniqueid = packet.ReadInt32();  // ??
                        byte petslot = packet.ReadUInt8();  // SELLING SLOT
                        int count = packet.ReadInt16();  // SELLING COUNT
                        int uniqueid = packet.ReadInt32(); // SELL UNIIQUEID NPC

                        if (session.SessionData.PetDataDictionary.ContainsKey((uint)petuniqueid))
                        {
                            var petInfo = session.SessionData.PetDataDictionary[(uint)petuniqueid];
                            uint refObjID = petInfo.RefObjID;

                            if (RefManager._GSNPCList.TryGetValue(uniqueid, out var npcData) && npcData != null)
                            {
                                int npcid = npcData.RefObjId;
                                string codeName128 = npcData.CodeName128;

                                bool isBlocked = await TriggerService.ControlTradeGoodsSellingRequest(
                                    session.SessionData.Charid, session.SessionData.Charname,
                                    (int)petInfo.RefObjID, npcid, codeName128, petslot, (short)count,
                                    session.SessionData.JobType, session.SessionData.Hwid);

                                if (isBlocked)
                                    return new PacketResult(packet, PacketResultType.Block);

                                if (_serverSettings.EnableTradeSellCaptcha)
                                {
                                    await TradeSellCaptchaService.RequestAsync(
                                        session,
                                        petuniqueid,
                                        petslot,
                                        (short)count,
                                        uniqueid);

                                    return new PacketResult(packet, PacketResultType.Block);
                                }
                            }
                            else
                            {
                                return new PacketResult(packet, PacketResultType.Nothing);
                            }
                        }
                    }

                    #endregion

                }


            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, CLIENT_ITEM_MOVE", ConsoleColor.Red);
            }
            return new PacketResult(packet, PacketResultType.Nothing);
        }
        #endregion

        private Task<PacketResult> SERVER_ENTITY_STATE_UPDATE(Packet packet, ISession session, object obj)
        {
            try
            {
                uint uniqueId = packet.ReadUInt32();
                if (session.SessionData.UniqueCharId != uniqueId)
                    return Task.FromResult(new PacketResult(packet, PacketResultType.Nothing));

                byte updateType = packet.ReadUInt8();
                byte updateState = packet.ReadUInt8();

                if (updateType == 4)
                {
                    session.SessionData.State.BodyState = (BodyState)updateState;
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_ENTITY_STATE_UPDATE", ConsoleColor.Red);
                return Task.FromResult(new PacketResult(packet, PacketResultType.Nothing));
            }
            return Task.FromResult(new PacketResult(packet, PacketResultType.Nothing));
        }
        private async Task<PacketResult> SERVER_ITEM_LOCK_RESULT(Packet packet, ISession session, object obj)
        {
            try
            {
                if (packet.GetBytes().Length < 3)
                    return new PacketResult(packet, PacketResultType.Block);

                _ = packet.ReadUInt8();
                byte result = packet.ReadUInt8();
                _ = packet.ReadUInt8();

                string message = PlayerLanguage.Get(
                    result == 1 ? "ItemLock.OperationFailed" : "ItemLock.TryAgain");
                Packet notice = new Packet(0xF201);
                notice.WriteUnicode(message);
                notice.WriteUInt8(0);
                await session.SendToClient(notice);
            }
            catch (Exception exception)
            {
                Log.Error(exception, "Failed to handle authoritative item-lock result");
            }

            return new PacketResult(packet, PacketResultType.Block);
        }

        #region Mob And Player Kill
        private async Task<PacketResult> SERVER_KILL_LOGGER(Packet packet, ISession session, object obj)
        {
            try
            {
                int WorldID = packet.ReadInt32();
                int SettedWorld = WorldID % 65536;
                int RegionID = packet.ReadInt32();
                int KillerCharID = packet.ReadInt32();
                string KillerCharName = packet.ReadAscii();
                byte KillerPvpCape = packet.ReadUInt8();
                byte KillerJobState = packet.ReadUInt8();
                int KillerGuildID = packet.ReadInt32();
                string KillerGuildName = packet.ReadAscii();
                int DeadCharID = packet.ReadInt32();
                string DeadCharName = packet.ReadAscii();
                byte DeadPvpCape = packet.ReadUInt8();
                byte DeadJobState = packet.ReadUInt8();
                int DeadGuildID = packet.ReadInt32();
                string DeadGuildName = packet.ReadAscii();

                await PvpChallengeService.HandleKillAsync(
                    SettedWorld,
                    RegionID,
                    KillerCharID,
                    KillerCharName,
                    KillerPvpCape,
                    DeadCharID,
                    DeadCharName,
                    DeadPvpCape);

                await SurvivalPartyEventService.HandleCharacterKillAsync(
                    SettedWorld,
                    RegionID,
                    KillerCharID,
                    KillerCharName,
                    DeadCharID);

                await SurvivalSoloEventService.HandleCharacterKillAsync(
                    SettedWorld,
                    RegionID,
                    KillerCharID,
                    KillerCharName,
                    DeadCharID);

                await CompetitiveEventService.HandleCharacterKillAsync(
                    SettedWorld,
                    RegionID,
                    KillerCharID,
                    KillerCharName,
                    DeadCharID);

                await DatabaseJobQueue.RunAsync(() =>
                {
                    try
                    {
                        using var newConnection = new SqlConnection(Program.Connectionstring);
                        newConnection.Open();

                        var parameters = new DynamicParameters();
                        parameters.Add("@SettedWorld", SettedWorld);
                        parameters.Add("@RegionID", RegionID);
                        parameters.Add("@KillerCharID", KillerCharID);
                        parameters.Add("@KillerCharName", KillerCharName);
                        parameters.Add("@KillerPvpCape", KillerPvpCape);
                        parameters.Add("@KillerJobState", KillerJobState);
                        parameters.Add("@KillerGuildID", KillerGuildID);
                        parameters.Add("@KillerGuildName", KillerGuildName);
                        parameters.Add("@DeadCharID", DeadCharID);
                        parameters.Add("@DeadCharName", DeadCharName);
                        parameters.Add("@DeadPvpCape", DeadPvpCape);
                        parameters.Add("@DeadJobState", DeadJobState);
                        parameters.Add("@DeadGuildID", DeadGuildID);
                        parameters.Add("@DeadGuildName", DeadGuildName);

                        string sqlQuery = "EXEC sp_executesql N'[dbo].[Hook_CharacterKill] @SettedWorld, @RegionID, @KillerCharID, @KillerCharName, @KillerPvpCape, " +
                                          "@KillerJobState, @KillerGuildID, @KillerGuildName, @DeadCharID, @DeadCharName, @DeadPvpCape, @DeadJobState, " +
                                          "@DeadGuildID, @DeadGuildName', N'@SettedWorld INT, @RegionID INT, @KillerCharID INT, @KillerCharName NVARCHAR(50), " +
                                          "@KillerPvpCape INT, @KillerJobState INT, @KillerGuildID INT, @KillerGuildName NVARCHAR(50), @DeadCharID INT, " +
                                          "@DeadCharName NVARCHAR(50), @DeadPvpCape INT, @DeadJobState INT, @DeadGuildID INT, @DeadGuildName NVARCHAR(50)', " +
                                          "@SettedWorld, @RegionID, @KillerCharID, @KillerCharName, @KillerPvpCape, @KillerJobState, " +
                                          "@KillerGuildID, @KillerGuildName, @DeadCharID, @DeadCharName, @DeadPvpCape, @DeadJobState, @DeadGuildID, @DeadGuildName";

                        newConnection.Execute(sqlQuery, parameters); // Parametreleri g�venli bir sekilde ge�iriyoruz.

                        //Log.Warning($"Procedure '[hooks].[OnCharacterKilled]' executed successfully.");
                    }
                    catch
                    {
                        //Log.Warning($"[CommandID] Prosed�r �alistirilirken hata: {ex.Message}");
                    }
                });



            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_KILL_LOGGER", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
            return new PacketResult(packet, PacketResultType.Block);
        }
        private static bool TryGetCharacterName(uint uniqueId, out string charName)
        {
            var activeSession = ServerManager.AgentSessions.FindByUniqueCharId(uniqueId);
            charName = activeSession?.SessionData.Charname ?? string.Empty;
            return charName.Length > 0;
        }

        private async Task<PacketResult> UNIQUE_DPS(Packet packet, ISession session, object obj)
        {
            try
            {
                if (packet.RemainingRead() < sizeof(int) * 2)
                    return new PacketResult(packet, PacketResultType.Block);

                var liveDps = new List<(uint PlayerGid, string CharacterName, long Damage)>();
                int MobID = packet.ReadInt32();//mobID
                int cont = packet.ReadInt32();//atack yapan kisi sayisi
                if (MobID <= 0 || cont < 0 || cont > UniqueHistoryService.MaximumDpsEntries ||
                    packet.RemainingRead() != cont * (sizeof(uint) + sizeof(uint)))
                    return new PacketResult(packet, PacketResultType.Block);

                for (int i = 0; i < cont; i++)
                {
                    uint PlayerGameID = packet.ReadUInt32();
                    long Atackvalue = packet.ReadUInt32();//unsigned GameServer damage value

                    if (TryGetCharacterName(PlayerGameID, out var charName))
                        liveDps.Add((PlayerGameID, charName, Atackvalue));
                }
                if (liveDps.Count != 0)
                {
                    Packet packets = new Packet(0x177F);

                    packets.WriteInt32(MobID);
                    var topDps = liveDps.OrderByDescending(o => o.Damage)
                        .ThenBy(o => o.PlayerGid)
                        .Take(UniqueHistoryService.MaximumDpsEntries).ToList();
                    packets.WriteUInt8((byte)topDps.Count);
                    foreach (var line in topDps)
                    {
                        packets.WriteAscii(line.CharacterName);
                        packets.WriteAscii(AgentServer.FormatNumber(line.Damage));
                    }
                    await AgentServer.BroadcastPacket(packets);

                }

                return new PacketResult(packet, PacketResultType.Block);
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, DPSMETER", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
        }
        private async Task<PacketResult> SERVER_MOB_KILL_LOGGER(Packet packet, ISession session, object obj)
        {
            try
            {
                const int fixedMobKillPayloadSize =
                    sizeof(int) + sizeof(int) + sizeof(ushort) +
                    sizeof(float) + sizeof(float) + sizeof(float) +
                    sizeof(int);

                if (packet.RemainingRead() < fixedMobKillPayloadSize)
                {
                    Log.Warning(
                        "Blocked short mob-kill logger packet. Char={CharName} Opcode=0x{Opcode:X4} Remaining={Remaining}",
                        session.SessionData.Charname,
                        packet.Opcode,
                        packet.RemainingRead());
                    return new PacketResult(packet, PacketResultType.Block);
                }

                int MonsterClass = packet.ReadInt32();
                int MobID = packet.ReadInt32();
                ushort RegionID = packet.ReadUInt16();
                float X = packet.ReadFloat();
                float Y = packet.ReadFloat();
                float Z = packet.ReadFloat();
                int WorldID = packet.ReadInt32();
                int SettedWorld = WorldID % 65536;
                await CompetitiveEventService.HandleMobKillAsync(session, MobID, SettedWorld, RegionID);
                //Log.Warning(MobID.ToString());
                if (MonsterClass == 3)
                {
                    Dictionary<string, int> LiveDps = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
                    if (packet.RemainingRead() < sizeof(int))
                    {
                        Log.Warning(
                            "Blocked unique-kill packet without DPS count. Char={CharName} MobID={MobID} Remaining={Remaining}",
                            session.SessionData.Charname,
                            MobID,
                            packet.RemainingRead());
                        return new PacketResult(packet, PacketResultType.Block);
                    }

                    int cont = packet.ReadInt32();//atack yapan kisi sayisi
                    if (cont < 0 ||
                        cont > UniqueHistoryService.MaximumDpsEntries ||
                        packet.RemainingRead() < cont * (sizeof(uint) + sizeof(int)))
                    {
                        Log.Warning(
                            "Blocked malformed unique-kill DPS packet. Char={CharName} MobID={MobID} Count={Count} Remaining={Remaining}",
                            session.SessionData.Charname,
                            MobID,
                            cont,
                            packet.RemainingRead());
                        return new PacketResult(packet, PacketResultType.Block);
                    }

                    for (int i = 1; i <= cont; i++)
                    {
                        uint PlayerGameID = packet.ReadUInt32();
                        int Atackvalue = packet.ReadInt32();//atack value

                        if (TryGetCharacterName(PlayerGameID, out var charName))
                            LiveDps[charName] = Atackvalue;
                    }
                    DateTime now = DateTime.UtcNow;
                    long unixTimestamp = (long)(now - new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc)).TotalSeconds;

                    var history = UniqueHistoryService.SetKilled(
                        UniqueHistoryService.CreateKilled(
                            MobID,
                            session.SessionData.Charname,
                            unixTimestamp,
                            RegionID,
                            X,
                            Y,
                            Z,
                            WorldID,
                            LiveDps));
                    await UniqueHistoryService.PersistAsync(history);

                    Packet pck = UniqueHistoryService.CreateUpdatePacket(history);
                    await ServerManager.BroadcastPacket(pck);
                    await BroadcastKillerAnimationIfActiveAsync(session);
                    if (RefManager.m_RefLoggerMobKill.Contains(MobID)) /// KONTROL
                    {
                        await LogMobKillAsync(session, MobID, SettedWorld, RegionID, X, Y, Z);
                    }
                }
                else
                {
                    if (RefManager.m_RefLoggerMobKill.Contains(MobID)) /// KONTROL
                    {
                        await LogMobKillAsync(session, MobID, SettedWorld, RegionID, X, Y, Z);
                    }
                }


                return new PacketResult(PacketResultType.Block);
            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString());
                return new PacketResult(PacketResultType.Block);
            }
        }
        private static void QueueHwidListUpdate(ISession session)
        {
            var charId = session.SessionData.Charid;
            if (charId <= 0)
                return;

            long nowTicks = Stopwatch.GetTimestamp();
            if (_lastHwidListUpdateTicks.TryGetValue(charId, out long lastTicks) &&
                nowTicks - lastTicks < HwidListUpdateCooldownTicks)
            {
                return;
            }

            _lastHwidListUpdateTicks[charId] = nowTicks;

            var charName = session.SessionData.Charname;
            var clientIp = session.ClientIp;
            var hwid = session.SessionData.Hwid;
            if (string.IsNullOrWhiteSpace(charName) ||
                string.IsNullOrWhiteSpace(clientIp) ||
                string.IsNullOrWhiteSpace(hwid))
            {
                return;
            }

            var jobType = session.SessionData.JobType;
            var regionId = session.SessionData.LatestRegion;
            var worldId = session.SessionData.WorldID;
            var level = session.SessionData.CurLevel;

            DatabaseJobQueue.TryQueueBackground(async cancellationToken =>
            {
                await using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync(cancellationToken);
                await connection.ExecuteAsync(
                    new CommandDefinition(
                    @"EXEC [dbo].[Auth_UpdateHWID]
                      1, @CharID, @CharName, @ClientIP, @Hwid,
                      @JobType, @RegionID, @WorldID, @Level",
                    new
                    {
                        CharID = charId,
                        CharName = charName,
                        ClientIP = clientIp,
                        Hwid = hwid,
                        JobType = jobType,
                        RegionID = regionId,
                        WorldID = worldId,
                        Level = level
                    },
                    commandTimeout: 10,
                    cancellationToken: cancellationToken));
            }, "HandleHwidList");
        }

        private async Task<PacketResult> FORTRESS_DPS(Packet packet, ISession session, object obj)
        {
            try
            {
                const int fixedPayloadSize = sizeof(byte) + sizeof(int) + sizeof(ushort) + sizeof(byte);
                if (packet.RemainingRead() < fixedPayloadSize)
                    return new PacketResult(packet, PacketResultType.Block);

                byte version = packet.ReadUInt8();
                int structureRefObjId = packet.ReadInt32();
                ushort regionId = packet.ReadUInt16();
                byte count = packet.ReadUInt8();
                if (version != 1 || structureRefObjId <= 0 || regionId == 0 ||
                    count == 0 || count > UniqueHistoryService.MaximumDpsEntries)
                    return new PacketResult(packet, PacketResultType.Block);

                var rankings = new List<KeyValuePair<string, ulong>>(count);
                var guildNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                for (int i = 0; i < count; i++)
                {
                    string guildName = packet.ReadAscii();
                    ulong damage = packet.ReadUInt64();
                    if (string.IsNullOrWhiteSpace(guildName) || guildName.Length > 64 ||
                        !guildNames.Add(guildName))
                        return new PacketResult(packet, PacketResultType.Block);
                    rankings.Add(new KeyValuePair<string, ulong>(guildName, damage));
                }
                if (packet.RemainingRead() != 0)
                    return new PacketResult(packet, PacketResultType.Block);

                Packet response = new Packet(0x177F);
                response.WriteInt32(structureRefObjId);
                response.WriteUInt8((byte)rankings.Count);
                foreach (var ranking in rankings.OrderByDescending(x => x.Value)
                             .ThenBy(x => x.Key, StringComparer.OrdinalIgnoreCase))
                {
                    response.WriteAscii(ranking.Key);
                    response.WriteAscii(ranking.Value <= long.MaxValue
                        ? AgentServer.FormatNumber((long)ranking.Value)
                        : ranking.Value.ToString("N0", CultureInfo.InvariantCulture));
                }
                await AgentServer.BroadcastPacket(response);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Rejected malformed Fortress DPS packet.");
            }
            return new PacketResult(packet, PacketResultType.Block);
        }

        private static void QueueEventSuitSnapshot(ISession session)
        {
            ServerManager.g_DelayedJobMgr.CreateJob(new DelayedJobItem(
                1500,
                session,
                null,
                async (s, p) =>
                {
                    if (s is ISession currentSession && currentSession.CharacterGameReady)
                    {
                        await RegionControlService.SendEventSuitSnapshotAsync(currentSession);
                    }
                }));
        }

        private static async Task LogMobKillAsync(
            ISession session, int mobId, int worldId, int regionId, float x, float y, float z)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync(
                @"EXEC [dbo].[Event_MobKilled]
                  @MobID, @WorldID, @RegionID, @X, @Y, @Z, @CharID, @CharName",
                new
                {
                    MobID = mobId,
                    WorldID = worldId,
                    RegionID = regionId,
                    X = x,
                    Y = y,
                    Z = z,
                    CharID = session.SessionData.Charid,
                    CharName = session.SessionData.Charname
                },
                commandTimeout: 60);
        }

        private static async Task BroadcastKillerAnimationIfActiveAsync(ISession session)
        {
            if (session.SessionData.Charid <= 0 || session.SessionData.UniqueCharId == 0)
                return;

            try
            {
                await using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync();
                int animationId = await connection.QueryFirstOrDefaultAsync<int>(
                    @"SELECT TOP (1) ref.AnimationID
                      FROM [dbo].[KillerAnimation_Active] active WITH (NOLOCK)
                      INNER JOIN [dbo].[KillerAnimation_List] ref WITH (NOLOCK)
                          ON ref.ID = active.AnimationRefID
                      INNER JOIN [dbo].[KillerAnimation_Owned] owned WITH (NOLOCK)
                          ON owned.CharID = active.CharID
                         AND owned.AnimationRefID = active.AnimationRefID
                      WHERE active.CharID = @CharID
                        AND ref.Service = 1
                        AND ref.AnimationID > 0
                        AND ref.AnimationID <= 500",
                    new { CharID = session.SessionData.Charid });

                if (animationId <= 0)
                    return;

                Packet animationPacket = new Packet(0x2079);
                animationPacket.WriteUInt32(session.SessionData.UniqueCharId);
                animationPacket.WriteInt32(animationId);
                await ServerManager.BroadcastPacket(animationPacket);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Failed to broadcast killer animation for {CharacterName}", session.SessionData.Charname);
            }
        }

        #endregion
    }
}


