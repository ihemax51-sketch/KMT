using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Features.UniqueHistory;
using KMTGuard.Database.ModelsEvents;
using KMTGuard.Features.Attendance;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Features.ItemChest;
using KMTGuard.Features.WebViewer;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;
using static System.Collections.Specialized.BitVector32;
using static System.Net.Mime.MediaTypeNames;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class CustomUIPackets
    {
        private const int AutoEquipCooldownSeconds = 60;
        private static readonly long AutoEquipCooldownTicks = Stopwatch.Frequency * AutoEquipCooldownSeconds;
        private static readonly ConcurrentDictionary<int, byte> AutoEquipRequestsInFlight = new();
        private static readonly ConcurrentDictionary<int, long> AutoEquipCooldowns = new();
        private AgentServer AgentServer { get; set; }
        private static string BrandHwidNotice(string noticeMessage)
        {
            if (string.IsNullOrWhiteSpace(noticeMessage))
                return PlayerLanguage.Get("SystemNotices.HWID_SUCCES");

            return noticeMessage
                .Replace("JTGuard", "KMTGuard", StringComparison.OrdinalIgnoreCase)
                .Replace("jtguard", "KMTGuard", StringComparison.OrdinalIgnoreCase);
        }

        private static async Task SendSecurityMessageAsync(ISession session, string languageKey)
        {
            Packet packet = new Packet(0xA340);
            packet.WriteUnicode(PlayerLanguage.Get(languageKey));
            await session.SendToClient(packet);
        }

        private static bool RequiresSummonedFellow(int actionWndID) =>
            actionWndID is >= 10001 and <= 10008;

        private static async Task SendFellowRequiredMessageAsync(ISession session)
        {
            Packet packet = new Packet(0x168A);
            packet.WriteUInt8(NoticeType.WARNING);
            packet.WriteUnicode(PlayerLanguage.Get("FellowBuff.FellowRequired"));
            await session.SendToClient(packet);
        }

        private static async Task SendAutoEquipCooldownMessageAsync(ISession session, int remainingSeconds)
        {
            Packet packet = new Packet(0x168A);
            packet.WriteUInt8(NoticeType.WARNING);
            packet.WriteUnicode(PlayerLanguage.Get("AutoEquip.Cooldown", remainingSeconds));
            await session.SendToClient(packet);
        }

        private static bool TryStartAutoEquipCooldown(int charId, out int remainingSeconds, out long startedAt)
        {
            long now = Stopwatch.GetTimestamp();

            while (true)
            {
                if (AutoEquipCooldowns.TryAdd(charId, now))
                {
                    remainingSeconds = 0;
                    startedAt = now;
                    return true;
                }

                if (!AutoEquipCooldowns.TryGetValue(charId, out long previousStart))
                    continue;

                long remainingTicks = AutoEquipCooldownTicks - (now - previousStart);
                if (remainingTicks > 0)
                {
                    remainingSeconds = Math.Max(1, (int)Math.Ceiling(remainingTicks / (double)Stopwatch.Frequency));
                    startedAt = 0;
                    return false;
                }

                if (AutoEquipCooldowns.TryUpdate(charId, now, previousStart))
                {
                    remainingSeconds = 0;
                    startedAt = now;
                    return true;
                }
            }
        }

        private static void ScheduleAutoEquipCooldownExpiry(int charId, long startedAt)
        {
            _ = ExpireAutoEquipCooldownAsync(charId, startedAt);
        }

        private static async Task ExpireAutoEquipCooldownAsync(int charId, long startedAt)
        {
            await Task.Delay(TimeSpan.FromSeconds(AutoEquipCooldownSeconds));
            ((ICollection<KeyValuePair<int, long>>)AutoEquipCooldowns)
                .Remove(new KeyValuePair<int, long>(charId, startedAt));
        }

        public CustomUIPackets(AgentServer agentServer, IPacketHandler packetHandler)
        {
            AgentServer = agentServer;
            var newrev = new NewReverse(agentServer, packetHandler);
            var titleiconmgrs = new Title_IconManagers(agentServer, packetHandler);
            //packetHandler.RegisterClientHandler(0x3560, CLIENT_INFO_REQUEST);

            packetHandler.RegisterClientHandler(0x189B, USE_FELLOW_SKILL);
            packetHandler.RegisterClientHandler(0x189A, SAVE_FELLOW_SKILL);

            packetHandler.RegisterClientHandler(0x185A, PARTY_MEMBER_VIEWER);
            packetHandler.RegisterClientHandler(0x185C, PARTY_MEMBER_VIEWER2);


            packetHandler.RegisterClientHandler(0x187E, UPDATE_MACRO_SETTING);




            packetHandler.RegisterClientHandler(0x169A, HandleGuiPackets);
            packetHandler.RegisterClientHandler(0xA400, GInterfaceIsReady);
            packetHandler.RegisterClientHandler(0x207B, CLIENT_GRANT_NAME_REQUEST);
            packetHandler.RegisterClientHandler(0x165B, CLIENT_HWID_REQUEST);

            packetHandler.RegisterClientHandler(0x180A, SERVER_RANKS);



            packetHandler.RegisterClientHandler(0xB301, CLIENT_HANDLE_EVENT_REGISTER_REQUEST);
            packetHandler.RegisterClientHandler(0x400B, ExecuteAutoEquipEditAsync);

            packetHandler.RegisterClientHandler(0xB299, CLIENT_PARTY_PING_REQUEST);
            packetHandler.RegisterClientHandler(0xB298, CLIENT_ITEM_TRANS_REQUEST);
            packetHandler.RegisterClientHandler(0xB297, CLIENT_NEW_ITEM_MALL_BUY_REQUEST);

            packetHandler.RegisterClientHandler(0xB296, CLIENT_ITEM_CHEST_REQUEST);
            packetHandler.RegisterClientHandler(0xCC1E, CLIENT_ACTION_USE);
            packetHandler.RegisterModuleHandler(0xA405, SERVER_ITEM_CHEST_RESPONSE);

        }

        private async Task ExecutePetActionAsync(
            ISession session,
            int actionWndID)
        {
            try
            {
                await using var conn = new SqlConnection(Program.Connectionstring);
                await conn.OpenAsync();

                await using var cmd = new SqlCommand(
                    "_OnActionWndCommandPet",
                    conn)
                {
                    CommandType = CommandType.StoredProcedure
                };

                cmd.Parameters.Add("@CharID", SqlDbType.Int).Value =
                    session.SessionData.Charid;
                cmd.Parameters.Add("@ActionWndID", SqlDbType.Int).Value =
                    actionWndID;

                await cmd.ExecuteNonQueryAsync();
            }
            catch (SqlException ex)
            {
                Log.Error(
                    ex,
                    "ActionWnd SQL execution failed. Procedure={Procedure}, CharID={CharID}, ActionWndID={ActionWndID}",
                    "_OnActionWndCommandPet",
                    session.SessionData.Charid,
                    actionWndID);
            }
            catch (Exception ex)
            {
                Log.Error(
                    ex,
                    "ActionWnd execution failed. CharID={CharID}, ActionWndID={ActionWndID}",
                    session.SessionData.Charid,
                    actionWndID);
            }
        }

        private async Task<PacketResult> CLIENT_ACTION_USE(
            Packet packet,
            ISession session,
            object obj)
        {
            try
            {
                if (packet.RemainingRead() != sizeof(int))
                {
                    Log.Warning(
                        "Rejected malformed ActionWnd request. CharID={CharID}, Remaining={Remaining}",
                        session.SessionData.Charid,
                        packet.RemainingRead());
                    return new PacketResult(PacketResultType.Block);
                }

                int actionWndID = packet.ReadInt32();

                if (RequiresSummonedFellow(actionWndID) && !session.SessionData.IsFellowSummoned)
                {
                    await SendFellowRequiredMessageAsync(session);
                    return new PacketResult(PacketResultType.Block);
                }

                if (actionWndID >= 9000)
                {
                    Log.Information(
                        "ActionWnd request received. CharID={CharID}, ActionWndID={ActionWndID}",
                        session.SessionData.Charid,
                        actionWndID);
                    await ExecutePetActionAsync(session, actionWndID);
                }
                else
                {
                    Log.Warning(
                        "Rejected invalid ActionWnd ID. CharID={CharID}, ActionWndID={ActionWndID}",
                        session.SessionData.Charid,
                        actionWndID);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "ActionWnd client request handling failed");
            }

            return new PacketResult(PacketResultType.Block);
        }

        private async Task<PacketResult> SERVER_ITEM_CHEST_RESPONSE(Packet packet, ISession session, object obj)
        {
            try
            {
                if (packet.RemainingRead() != sizeof(byte) + sizeof(int))
                {
                    Log.Warning(
                        "Rejected malformed Item Chest GameServer response. CharID={CharID}, Remaining={Remaining}",
                        session.SessionData.Charid,
                        packet.RemainingRead());
                    return new PacketResult(PacketResultType.Block);
                }

                byte result = packet.ReadUInt8();
                int DBID = packet.ReadInt32();
                await ItemChestService.CompleteClaimAsync(session, DBID, result == 0);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Unhandled Item Chest GameServer response failure");
            }
            return new PacketResult(PacketResultType.Block);

        }
        private async Task<PacketResult> CLIENT_ITEM_CHEST_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                if (packet.RemainingRead() != sizeof(int))
                {
                    Log.Warning(
                        "Rejected malformed Item Chest client request. CharID={CharID}, Remaining={Remaining}",
                        session.SessionData.Charid,
                        packet.RemainingRead());
                    return new PacketResult(PacketResultType.Block);
                }

                int DBID = packet.ReadInt32();
                await ItemChestService.BeginClaimAsync(session, DBID);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Unhandled Item Chest client request failure");
            }
            return new PacketResult(PacketResultType.Block);

        }
        private async Task<PacketResult> CLIENT_NEW_ITEM_MALL_BUY_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                if (_serverSettings.NewItemMall)
                {
                    byte type = packet.ReadUInt8();
                    if (type == 11)
                    {
                        int ItemMallDbID = packet.ReadInt32();
                        int Quantity = packet.ReadInt32();
                        if (Quantity < 1 || Quantity > 100)
                            return new PacketResult(PacketResultType.Block);

                        if (RefManager.m_RefNewItemMall.ContainsKey(ItemMallDbID))
                        {
                            int Price = RefManager.m_RefNewItemMall[ItemMallDbID].Silk;
                            int totalPrice;
                            try { totalPrice = checked(Price * Quantity); }
                            catch (OverflowException) { return new PacketResult(PacketResultType.Block); }

                            var purchase = await PurchaseItemMallAsync(
                                session,
                                RefManager.m_RefNewItemMall[ItemMallDbID].ItemID,
                                RefManager.m_RefNewItemMall[ItemMallDbID].CodeName128,
                                RefManager.m_RefNewItemMall[ItemMallDbID].ItemCount,
                                Quantity,
                                totalPrice,
                                "New Item Mall");

                            if (purchase != null)
                            {
                                Packet packeta = new Packet(0x3527);
                                packeta.WriteAscii(session.GameServerPacketKey);
                                packeta.WriteInt32(purchase.silk_own);
                                packeta.WriteInt32(purchase.silk_gift);
                                packeta.WriteInt32(purchase.silk_point);
                                await session.SendToServer(packeta);

                                for (int i = 0; i < Quantity; i++)
                                {
                                    Packet stMsg = new Packet(0x169C);
                                    stMsg.WriteUInt8(12);
                                    stMsg.WriteUnicode(PlayerLanguage.Get("ItemMall.AddedToChest"));
                                    stMsg.WriteInt32(RefManager.m_RefNewItemMall[ItemMallDbID].ItemID);
                                    await session.SendToClient(stMsg);
                                }
                            }
                            else
                            {
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(PlayerLanguage.Get("Purchase.InsufficientSilk"));
                                await session.SendToClient(stMsg);
                            }

                        }
                    }
                    else if (type == 12)
                    {
                        int ItemMallDbID = packet.ReadInt32();

                        if (RefManager.m_RefNewAvatarMall.ContainsKey(ItemMallDbID))
                        {
                            int Price = RefManager.m_RefNewAvatarMall[ItemMallDbID].Silk;

                            var purchase = await PurchaseItemMallAsync(
                                session,
                                RefManager.m_RefNewAvatarMall[ItemMallDbID].ItemID,
                                RefManager.m_RefNewAvatarMall[ItemMallDbID].CodeName128,
                                1,
                                1,
                                Price,
                                "New Avatar Mall");

                            if (purchase != null)
                            {
                                // Paket g�nderme islemi
                                Packet packeta = new Packet(0x3527);
                                packeta.WriteAscii(session.GameServerPacketKey);
                                packeta.WriteInt32(purchase.silk_own);
                                packeta.WriteInt32(purchase.silk_gift);
                                packeta.WriteInt32(purchase.silk_point);
                                await session.SendToServer(packeta);



                                Packet stMsg = new Packet(0x169C);
                                stMsg.WriteUInt8(12);
                                stMsg.WriteUnicode(PlayerLanguage.Get("ItemMall.AddedToChest"));
                                stMsg.WriteInt32(RefManager.m_RefNewAvatarMall[ItemMallDbID].ItemID);
                                await session.SendToClient(stMsg);
                            }
                            else
                            {
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(PlayerLanguage.Get("Purchase.InsufficientSilk"));
                                await session.SendToClient(stMsg);
                            }

                        }
                    }
                }

            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + " CLIENT_NEW_ITEM_MALL_BUY_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult(PacketResultType.Block);

        }
        private async Task<PacketResult> CLIENT_ITEM_TRANS_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                if (_serverSettings.EnableItemTranslation)
                {
                    byte SlotIndex = packet.ReadUInt8();
                    string ItemCodeName = packet.ReadAscii();
                    string TargetItemCodeName = packet.ReadAscii();

                    bool isvalid = false;
                    //string query = $"SELECT 1 FROM {_serverSettings.ShardDB}.._RefObjCommon with (nolock) WHERE CodeName128 = '{TargetItemCodeName}' and Service = 1";
                    //using (var connection = new SqlConnection(Program.Connectionstring))
                    //{
                    //    using (var command = new SqlCommand(query, connection))
                    //    {
                    //        await connection.OpenAsync();
                    //        var result = await command.ExecuteScalarAsync();
                    //        if (result != null)
                    //        {
                    //            isvalid = true;
                    //        }
                    //    }
                    //}
                    isvalid = await RefManager.GetRefObjCommonValidate(TargetItemCodeName);
                    if (!isvalid)
                    {
                        Packet invalidItemNotice = new Packet(0x168A);
                        invalidItemNotice.WriteUInt8(NoticeType.WARNING);
                        invalidItemNotice.WriteUnicode(PlayerLanguage.Get("ItemTranslation.InvalidTarget"));
                        await session.SendToClient(invalidItemNotice);
                        return new PacketResult(PacketResultType.Block);
                    }
                    if (_serverSettings.ItemTranslationPayment == 0) /// silk
                    {
                        var silkInfo = await ChargeSilkAsync(
                            session.SessionData.JID,
                            _serverSettings.ItemTranslationPrice);
                        if (silkInfo != null)
                        {
                            Packet packeta = new Packet(0x3527);
                            packeta.WriteAscii(session.GameServerPacketKey);
                            packeta.WriteInt32(silkInfo.silk_own);
                            packeta.WriteInt32(silkInfo.silk_gift);
                            packeta.WriteInt32(silkInfo.silk_point);
                            await session.SendToServer(packeta);

                            Packet pck = new Packet(0x3505, false, false);
                            pck.WriteAscii(session.GameServerPacketKey);
                            pck.WriteUInt8(_serverSettings.ItemTranslationPayment);
                            pck.WriteUInt8(SlotIndex);
                            pck.WriteAscii(ItemCodeName);
                            pck.WriteAscii(TargetItemCodeName);
                            await session.SendToServer(pck);
                        }
                        else
                        {
                            Packet stMsgx = new Packet(0x5015);
                            stMsgx.WriteUInt8(6);
                            await session.SendToClient(stMsgx);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                    else
                    {
                        Packet pck = new Packet(0x3505, false, false);
                        pck.WriteAscii(session.GameServerPacketKey);
                        pck.WriteUInt8(_serverSettings.ItemTranslationPayment);
                        pck.WriteUInt8(SlotIndex);
                        pck.WriteAscii(ItemCodeName);
                        pck.WriteAscii(TargetItemCodeName);
                        pck.WriteInt32(_serverSettings.ItemTranslationPrice);
                        await session.SendToServer(pck);
                    }
                }
                else
                    return new PacketResult(PacketResultType.Disconnect);

            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + "CLIENT_ITEM_TRANS_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult(PacketResultType.Block);

        }
        private async Task<PacketResult> CLIENT_PARTY_PING_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                int pingedregionID = packet.ReadInt32();
                int pingedPosX = packet.ReadInt32();
                int pingedPosY = packet.ReadInt32();
                int pingedPosZ = packet.ReadInt32();
                int Is0OutSide2IsInside = packet.ReadInt32();
                int Normal0Town1Dungeon2 = packet.ReadInt32();
                byte PtMemberscount = packet.ReadUInt8();
                for (byte iss = 0; iss < PtMemberscount; iss++)
                {
                    //string PartyMemberName = _pck.ReadAscii();
                    string PtMemberName = packet.ReadAscii();


                    var Agents = ServerManager.AgentSessions.FindByCharName(PtMemberName);
                    if (Agents != null)
                    {
                        Packet packeta = new Packet(0x193E);
                        packeta.WriteUInt8(1);
                        packeta.WriteInt32(pingedregionID);
                        packeta.WriteInt32(pingedPosX);
                        packeta.WriteInt32(pingedPosY);
                        packeta.WriteInt32(pingedPosZ);
                        packeta.WriteInt32(Is0OutSide2IsInside);
                        packeta.WriteInt32(Normal0Town1Dungeon2);
                        await Agents.SendToClient(packeta);
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + "CLIENT_PARTY_PING_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult(PacketResultType.Block);

        }

        private async Task<PacketResult> ExecuteAutoEquipEditAsync(Packet packet, ISession session, object obj)
        {
            int charId = session.SessionData.Charid;
            int charLevel = session.SessionData.CurLevel;
            int maxLevel = Math.Max(0, _serverSettings.AutoEquipMaxLevel);

            if (packet.RemainingRead() != 0 || charId <= 0 || charLevel <= 0 ||
                !_serverSettings.ShowGuideAutoEquip ||
                (maxLevel > 0 && charLevel > maxLevel))
            {
                return new PacketResult(PacketResultType.Block);
            }

            if (!TryStartAutoEquipCooldown(charId, out int remainingSeconds, out long cooldownStartedAt))
            {
                await SendAutoEquipCooldownMessageAsync(session, remainingSeconds);
                return new PacketResult(PacketResultType.Block);
            }

            ScheduleAutoEquipCooldownExpiry(charId, cooldownStartedAt);

            if (!AutoEquipRequestsInFlight.TryAdd(charId, 0))
            {
                await SendAutoEquipCooldownMessageAsync(session, AutoEquipCooldownSeconds);
                return new PacketResult(PacketResultType.Block);
            }

            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    using (var command = new SqlCommand("dbo.Hook_AutoEquip", connection))
                    {
                        command.CommandType = CommandType.StoredProcedure;
                        command.Parameters.Add("@CharID", SqlDbType.Int).Value = charId;
                        command.Parameters.Add("@CharLevel", SqlDbType.Int).Value = charLevel;
                        command.CommandTimeout = 60;
                        await command.ExecuteNonQueryAsync();
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex,
                    "Auto Equip execution failed. CharID={CharID}, CharLevel={CharLevel}",
                    charId,
                    charLevel);
            }
            finally
            {
                AutoEquipRequestsInFlight.TryRemove(charId, out _);
            }
            return new PacketResult(PacketResultType.Block);
        }

        private async Task<PacketResult> CLIENT_HANDLE_EVENT_REGISTER_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                byte type = packet.ReadUInt8();
                if ((eGuiRequestType)type == eGuiRequestType.EventRegisterRequest)
                {
                    byte RefEventID = packet.ReadUInt8();

                    // Block registration while wearing a Job suit (Trader=1, Thief=2, Hunter=3).
                    byte jobType = session.SessionData.JobType;
                    if (jobType >= 1 && jobType <= 3)
                    {
                        var notice = new Packet(0x168A);
                        notice.WriteUInt8(NoticeType.WARNING);
                        notice.WriteUnicode(PlayerLanguage.Get("Event.JobSuitBlocked"));
                        await session.SendToClient(notice);
                        return new PacketResult(PacketResultType.Block);
                    }

                    if (SurvivalPartyEventService.IsRegistrationEventId(RefEventID))
                    {
                        await SurvivalPartyEventService.HandleRegistrationAsync(session);
                        return new PacketResult(PacketResultType.Block);
                    }
                    if (SurvivalSoloEventService.IsRegistrationEventId(RefEventID))
                    {
                        await SurvivalSoloEventService.HandleRegistrationAsync(session);
                        return new PacketResult(PacketResultType.Block);
                    }
                    if (CompetitiveEventService.IsRegistrationEventId(RefEventID))
                    {
                        await CompetitiveEventService.HandleRegistrationAsync(session, RefEventID);
                        return new PacketResult(PacketResultType.Block);
                    }

                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                using (var command = new SqlCommand(
                                    "EXEC [dbo].[Hook_EventRegister] @CharID, @CharName, @RefEventID, @LatestRegion, @WorldID",
                                    connection))
                                {
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname ?? string.Empty);
                                    command.Parameters.AddWithValue("@RefEventID", RefEventID);
                                    command.Parameters.AddWithValue("@LatestRegion", session.SessionData.LatestRegion);
                                    command.Parameters.AddWithValue("@WorldID", session.SessionData.WorldID);
                                    command.CommandTimeout = 60;
                                    command.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"HandleHwidList hata: {ex.Message}");
                        }
                    });
                }
                else if ((eGuiRequestType)type == eGuiRequestType.EventRegisterCancelRequest)
                {
                    byte RefEventID = packet.ReadUInt8();
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                using (var command = new SqlCommand(
                                    "EXEC [dbo].[Hook_EventCancel] @CharID, @CharName, @RefEventID, @LatestRegion, @WorldID",
                                    connection))
                                {
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname ?? string.Empty);
                                    command.Parameters.AddWithValue("@RefEventID", RefEventID);
                                    command.Parameters.AddWithValue("@LatestRegion", session.SessionData.LatestRegion);
                                    command.Parameters.AddWithValue("@WorldID", session.SessionData.WorldID);
                                    command.CommandTimeout = 60;
                                    command.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"HandleHwidList hata: {ex.Message}");
                        }
                    });


                }
            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + "CLIENT_HANDLE_EVENT_REGISTER_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult(PacketResultType.Block);

        }

        private async Task SendCustomRankAsync(ISession session, IReadOnlyDictionary<int, _Rank_Custom1> dict)
        {
            var ordered = (dict ?? new Dictionary<int, _Rank_Custom1>())
                .Values
                .OrderByDescending(entry => entry.Point)
                .ThenBy(entry => entry.CharID)
                .ToList();
            var top = ordered.Take(50).ToList();

            var packList = new Packet(0x170E);
            packList.WriteUInt8((byte)top.Count);
            foreach (var entry in top)
            {
                packList.WriteAscii(entry.CharName16);
                packList.WriteAscii(entry.GuildName);
                packList.WriteInt32(entry.Point);
            }
            await session.SendToClient(packList);

            var selfIndex = ordered.FindIndex(entry =>
                entry.CharID == session.SessionData.Charid &&
                string.Equals(
                    entry.CharName16,
                    session.SessionData.Charname,
                    StringComparison.OrdinalIgnoreCase));
            if (selfIndex >= 0)
            {
                var self = ordered[selfIndex];
                var packMe = new Packet(0x171A);
                packMe.WriteAscii(session.SessionData.Charname);
                packMe.WriteInt32(selfIndex + 1);
                packMe.WriteInt32(self.Point);
                await session.SendToClient(packMe);
            }
        }

        private async Task<PacketResult> SERVER_RANKS(Packet packet, ISession session, object obj)
        {
            try
            {
                byte type = packet.ReadUInt8();

                if (type == 0)
                {
                    var categories = RefManager.RankCategories
                        .OrderBy(entry => entry.Key)
                        .Take(byte.MaxValue)
                        .ToList();
                    var stAck = new Packet(0x170F);
                    stAck.WriteUInt8((byte)categories.Count);
                    foreach (var line in categories)
                    {
                        stAck.WriteInt32(line.Key);
                        stAck.WriteAscii(line.Value.Category);
                    }
                    await session.SendToClient(stAck);
                }
                else if (RefManager.TryGetRank(type, out var rank))
                {
                    await SendCustomRankAsync(session, rank);
                }
                else
                {
                    // A well-formed empty response clears stale client-side data
                    // for an inactive or unsupported category.
                    await SendCustomRankAsync(
                        session,
                        new Dictionary<int, _Rank_Custom1>());
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message);
            }

            return new PacketResult(packet, PacketResultType.Block);
        }

        private Task<PacketResult> CLIENT_INFO_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                session.SessionData.JobType = packet.ReadUInt8();
                session.SessionData.LatestRegion = packet.ReadInt32();
                session.SessionData.WorldID = packet.ReadInt32();

                if (session.SessionData.JobType != 4)
                {
                    session.SessionData.JobName = packet.ReadAscii();
                }


            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + "CLIENT_INF_REQ", ConsoleColor.Red);
            }
            return Task.FromResult(new PacketResult(PacketResultType.Block));

        }

        #region FELLOW
        private async Task<PacketResult> USE_FELLOW_SKILL(Packet packet, ISession session, object obj)
        {
            try
            {
                if (!session.CharScreen)
                {
                    Int64 ID64 = packet.ReadInt64();
                    string NameStrID = packet.ReadAscii();
                    byte SkillSlot = packet.ReadUInt8();
                    uint UniqueID = packet.ReadUInt32();

                    if (RefManager.m_RefFellowData.ContainsKey(NameStrID))
                    {
                        if (session.SessionData.FellowPetID64 == ID64)
                        {
                            if (SkillSlot == 1 && RefManager.m_RefFellowData[NameStrID].SkillType_1 == 1)
                            {
                                Packet LiveTitle = new Packet(0x3511, true, false);
                                LiveTitle.WriteAscii(session.GameServerPacketKey);
                                LiveTitle.WriteInt32(RefManager.m_RefFellowData[NameStrID].SkillID_1);
                                LiveTitle.WriteUInt32(UniqueID);
                                //LiveTitle.WriteAscii(NameStrID);
                                LiveTitle.WriteUInt8(RefManager.m_RefFellowData[NameStrID].SkillAnimationID);
                                await session.SendToServer(LiveTitle);
                            }
                            else if (SkillSlot == 1 && RefManager.m_RefFellowData[NameStrID].SkillType_1 == 0)
                            {
                                Packet LiveTitle = new Packet(0x3511, true, false);
                                LiveTitle.WriteAscii(session.GameServerPacketKey);
                                LiveTitle.WriteInt32(RefManager.m_RefFellowData[NameStrID].SkillID_1);
                                LiveTitle.WriteUInt32(UniqueID);
                                //LiveTitle.WriteAscii(NameStrID);
                                LiveTitle.WriteUInt8(RefManager.m_RefFellowData[NameStrID].SkillAnimationID);
                                await session.SendToServer(LiveTitle);
                            }
                            else if (SkillSlot == 2)
                            {
                                Packet LiveTitle = new Packet(0x3511, true, false);
                                LiveTitle.WriteAscii(session.GameServerPacketKey);
                                LiveTitle.WriteInt32(RefManager.m_RefFellowData[NameStrID].SkillID_2);
                                LiveTitle.WriteUInt32(UniqueID);
                                //LiveTitle.WriteAscii(NameStrID);
                                LiveTitle.WriteUInt8(RefManager.m_RefFellowData[NameStrID].SkillAnimationID);
                                await session.SendToServer(LiveTitle);
                            }
                            else if (SkillSlot == 3)
                            {
                                Packet LiveTitle = new Packet(0x3511, true, false);
                                LiveTitle.WriteAscii(session.GameServerPacketKey);
                                LiveTitle.WriteInt32(RefManager.m_RefFellowData[NameStrID].SkillID_3);
                                LiveTitle.WriteUInt32(UniqueID);
                                //LiveTitle.WriteAscii(NameStrID);
                                LiveTitle.WriteUInt8(RefManager.m_RefFellowData[NameStrID].SkillAnimationID);
                                await session.SendToServer(LiveTitle);
                            }
                            else if (SkillSlot == 4)
                            {
                                Packet LiveTitle = new Packet(0x3511, true, false);
                                LiveTitle.WriteAscii(session.GameServerPacketKey);
                                LiveTitle.WriteInt32(RefManager.m_RefFellowData[NameStrID].SkillID_4);
                                LiveTitle.WriteUInt32(UniqueID);
                                //LiveTitle.WriteAscii(NameStrID);
                                LiveTitle.WriteUInt8(RefManager.m_RefFellowData[NameStrID].SkillAnimationID);
                                await session.SendToServer(LiveTitle);
                            }
                            else if (SkillSlot == 5)
                            {
                                Packet LiveTitle = new Packet(0x3511, true, false);
                                LiveTitle.WriteAscii(session.GameServerPacketKey);
                                LiveTitle.WriteInt32(RefManager.m_RefFellowData[NameStrID].SkillID_5);
                                LiveTitle.WriteUInt32(UniqueID);
                                //LiveTitle.WriteAscii(NameStrID);
                                LiveTitle.WriteUInt8(RefManager.m_RefFellowData[NameStrID].SkillAnimationID);
                                await session.SendToServer(LiveTitle);
                            }
                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "USE_FELLOW_SKILL");
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> SAVE_FELLOW_SKILL(Packet packet, ISession session, object obj)
        {
            try
            {
                if (!session.CharScreen)
                {
                    Int64 ID64 = packet.ReadInt64();
                    string NameStrID = packet.ReadUnicode();
                    byte LastLevel = packet.ReadUInt8();
                    byte SkillSlot = packet.ReadUInt8();
                    byte Active = packet.ReadUInt8();


                    if (RefManager.m_RefFellowData.ContainsKey(NameStrID))
                    {
                        if (session.SessionData.FellowPetID64 == ID64)
                        {
                            await DatabaseJobQueue.RunAsync(() =>
                            {
                                try
                                {
                                    using (var connection = new SqlConnection(Program.Connectionstring))
                                    {
                                        connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                        string columnName = SkillSlot switch
                                        {
                                            1 => "Enable_Skill_1",
                                            2 => "Enable_Skill_2",
                                            3 => "Enable_Skill_3",
                                            4 => "Enable_Skill_4",
                                            5 => "Enable_Skill_5",
                                            _ => string.Empty
                                        };

                                        if (string.IsNullOrEmpty(columnName))
                                            return;

                                        string query = $"UPDATE [dbo].[Fellow_Skills] SET {columnName} = @Active WHERE ID64 = @ID64";
                                        using (var command = new SqlCommand(query, connection))
                                        {
                                            command.Parameters.AddWithValue("@Active", Active);
                                            command.Parameters.AddWithValue("@ID64", ID64);
                                            command.CommandTimeout = 60;
                                            command.ExecuteNonQuery();
                                        }
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Log.Error($"HandleHwidList hata: {ex.Message}");
                                    throw;
                                }
                            });
                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "SAVE_FELLOW_SKILL");
            }
            return new PacketResult(PacketResultType.Block);
        }

        #endregion
        private async Task<PacketResult> UPDATE_MACRO_SETTING(Packet packet, ISession session, object obj)
        {
            try
            {
                byte AutoPotion = packet.ReadUInt8();
                byte AutoSkill = packet.ReadUInt8();
                byte AutoHunt = packet.ReadUInt8();
                byte AutoPickup = packet.ReadUInt8();
                byte AutoScroll = packet.ReadUInt8();


                await DatabaseJobQueue.RunAsync(() =>
                {
                    try
                    {
                        using (var connection = new SqlConnection(Program.Connectionstring))
                        {
                            connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                            using (var command = new SqlCommand(
                                "EXEC [dbo].[UI_SaveMacro] @CharID, @AutoPotion, @AutoSkill, @AutoHunt, @AutoPickup, @AutoScroll",
                                connection))
                            {
                                command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                command.Parameters.AddWithValue("@AutoPotion", AutoPotion);
                                command.Parameters.AddWithValue("@AutoSkill", AutoSkill);
                                command.Parameters.AddWithValue("@AutoHunt", AutoHunt);
                                command.Parameters.AddWithValue("@AutoPickup", AutoPickup);
                                command.Parameters.AddWithValue("@AutoScroll", AutoScroll);
                                command.CommandTimeout = 60;
                                command.ExecuteNonQuery();
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Error($"HandleHwidList hata: {ex.Message}");
                        throw;
                    }
                });
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "UPDATE_MACRO_SETTING");
            }
            return new PacketResult(PacketResultType.Block);
        }

        #region PARTY MEMBER VIEWER
        public class PartyMemberData
        {
            public byte CharLevel { get; set; }
            public int RefObjID { get; set; }
            public int Mastery1 { get; set; }
            public int Mastery2 { get; set; }
        };
        private async Task<PacketResult> PARTY_MEMBER_VIEWER2(Packet packet, ISession session, object obj)
        {
            try
            {
                byte Type = packet.ReadUInt8(); // FromSlot
                string CharNameSendCommand = packet.ReadAscii(); // party Master
                if (Type == 0)
                {
                    byte partymembercount = packet.ReadUInt8();
                    if (partymembercount > 0)
                    {
                        Dictionary<int, PartyMemberData>? PartyMember = new();
                        //PartyMember.Clear();
                        for (byte iss = 0; iss < partymembercount; iss++)
                        {
                            //string PartyMemberName = _pck.ReadAscii();
                            byte CurLevel = packet.ReadUInt8();
                            int RefObjID = packet.ReadInt32();
                            int Mastery1 = packet.ReadInt32();
                            int Mastery2 = packet.ReadInt32();
                            var Memember = new PartyMemberData();

                            Memember.RefObjID = RefObjID;
                            //  CharName = PartyMemberName,
                            Memember.CharLevel = CurLevel;
                            Memember.Mastery1 = Mastery1;
                            Memember.Mastery2 = Mastery2;

                            PartyMember.TryAdd(iss, Memember);

                        }
                        Packet test = new Packet(0x178E);
                        test.WriteUInt8(1);
                        test.WriteUInt8(PartyMember.Count());

                        foreach (var line in PartyMember)
                        {
                            //test.WriteUnicode(line.Value.CharName);
                            test.WriteInt32(line.Value.RefObjID);
                            test.WriteUInt8(line.Value.CharLevel);
                            test.WriteInt32(line.Value.Mastery1);
                            test.WriteInt32(line.Value.Mastery2);
                        }
                        var Agents = ServerManager.AgentSessions.FindByCharName(CharNameSendCommand);
                        if (Agents != null)
                        {
                            await Agents.SendToClient(test);
                        }
                    }
                }
                else if (Type == 1)
                {
                    int RefObjID = packet.ReadInt32();
                    byte CurLevel = packet.ReadUInt8();
                    int mastery1 = packet.ReadInt32();
                    int mastery2 = packet.ReadInt32();

                    Packet GetPartyInfo = new Packet(0x178E);
                    GetPartyInfo.WriteUInt8(3);
                    GetPartyInfo.WriteInt32(RefObjID);
                    GetPartyInfo.WriteUInt8(CurLevel);
                    GetPartyInfo.WriteInt32(mastery1);
                    GetPartyInfo.WriteInt32(mastery2);
                    var Agents = ServerManager.AgentSessions.FindByCharName(CharNameSendCommand);
                    if (Agents != null)
                    {
                        await Agents.SendToClient(GetPartyInfo);
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "HandleTitleManager");
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> PARTY_MEMBER_VIEWER(Packet packet, ISession session, object obj)
        {
            try
            {
                byte Type = packet.ReadUInt8(); // FromSlot
                string CharName = packet.ReadAscii(); // party Master

                var Agents = ServerManager.AgentSessions.FindByCharName(CharName);
                var Agentsx = ServerManager.AgentSessions.FirstOrDefault(x => x.SessionData.JobName == CharName);

                if (Agents != null)
                {
                    if (Type == 0) // Call Party Info From Pt Master
                    {
                        Packet GetPartyInfo = new Packet(0x178E);
                        GetPartyInfo.WriteUInt8(0); // type
                        GetPartyInfo.WriteAscii(session.SessionData.Charname);// Message
                        await Agents.SendToClient(GetPartyInfo);
                    }
                    else if (Type == 1)
                    {
                        Packet GetPartyInfo = new Packet(0x178E);
                        GetPartyInfo.WriteUInt8(2); // typee
                        GetPartyInfo.WriteAscii(session.SessionData.Charname);// Message
                        await Agents.SendToClient(GetPartyInfo);
                    }
                }
                else if (Agentsx != null)
                {
                    if (Type == 0) // Call Party Info From Pt Master
                    {
                        Packet GetPartyInfo = new Packet(0x178E);
                        GetPartyInfo.WriteUInt8(0); // type
                        GetPartyInfo.WriteAscii(session.SessionData.Charname);// Message
                        await Agentsx.SendToClient(GetPartyInfo);
                    }
                    else if (Type == 1)
                    {
                        Packet GetPartyInfo = new Packet(0x178E);
                        GetPartyInfo.WriteUInt8(2); // type
                        GetPartyInfo.WriteAscii(session.SessionData.Charname);// Message
                        await Agentsx.SendToClient(GetPartyInfo);
                    }
                }

            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "PARTY_MEMBER_VIEWER");
            }
            return new PacketResult(PacketResultType.Block);
        }
        #endregion

        enum eGuiRequestType : byte
        {
            EventRegisterRequest = 16,
            EventRegisterCancelRequest = 17
        }
        private async Task<PacketResult> HandleGuiPackets(Packet packet, ISession session, object obj)
        {
            try
            {
                byte type = packet.ReadUInt8();
                if (type == 0)
                {
                    if (!session.SessionData.TitleManagerPacket)
                    {
                        session.SessionData.TitleManagerPacket = true;

                        if (session.SessionData.PlayerTitles.Count() > 0)
                        {
                            Packet stAckMsg = new Packet(0x200A);
                            stAckMsg.WriteUInt8(session.SessionData.PlayerTitles.Count());

                            foreach (var stRecord in session.SessionData.PlayerTitles.OrderBy(x => x))
                            {
                                stAckMsg.WriteUInt8(stRecord);
                                if (session.SessionData.EUChar)
                                {
                                    if (RefManager.RefHwan.ContainsKey(Convert.ToByte(stRecord)))
                                    {
                                        stAckMsg.WriteAscii(RefManager.RefHwan[(Convert.ToByte(stRecord))].Title_EU70);
                                    }
                                    else
                                    {
                                        stAckMsg.WriteAscii("dummy");
                                    }
                                }
                                else if (session.SessionData.CHChar)
                                {
                                    if (RefManager.RefHwan.ContainsKey(Convert.ToByte(stRecord)))
                                    {
                                        stAckMsg.WriteAscii(RefManager.RefHwan[(Convert.ToByte(stRecord))].Title_CH70);
                                    }
                                    else
                                    {
                                        stAckMsg.WriteAscii("dummy");
                                    }
                                }
                            }
                            await session.SendToClient(stAckMsg);
                        }

                        if (session.SessionData.PlayerTitleColors.Count() > 0)
                        {
                            Packet stAckMsgColor = new Packet(0x200B);
                            stAckMsgColor.WriteUInt8(session.SessionData.PlayerTitleColors.Count());

                            foreach (var stRecord in session.SessionData.PlayerTitleColors.OrderBy(x => x.Value.ID))
                            {
                                stAckMsgColor.WriteInt32(stRecord.Value.ID);
                                stAckMsgColor.WriteAscii(stRecord.Value.ColorName);

                                int argbInputColor = Int32.Parse(stRecord.Value.ColorCode.Replace("#", ""), NumberStyles.HexNumber);
                                stAckMsgColor.WriteUInt32(argbInputColor);
                            }

                            await session.SendToClient(stAckMsgColor);
                        }

                    }
                }
                else if (type == 1)
                {
                    if (!session.SessionData.IconMgrPacket)
                    {
                        session.SessionData.IconMgrPacket = true;

                        int nCharDBID = session.SessionData.Charid;

                        if (session.SessionData.PlayerIcons.Count() > 0 && RefManager.Icons.Count() > 0)
                        {
                            Packet stAckMsg = new Packet(0x201B);
                            stAckMsg.WriteUInt8(session.SessionData.PlayerIcons.Count());
                            foreach (var stRecord in session.SessionData.PlayerIcons)
                            {
                                stAckMsg.WriteInt32(stRecord.Value.IconID);
                                if (RefManager.Icons.ContainsKey(stRecord.Key))
                                {
                                    stAckMsg.WriteAscii(RefManager.Icons[stRecord.Key]);
                                }
                                else
                                {
                                    stAckMsg.WriteAscii("dummy");

                                }
                                stAckMsg.WriteUInt8(stRecord.Value.Side);
                            }
                            await session.SendToClient(stAckMsg);
                        }
                    }
                }
                else if (type == 2)
                {
                    var uniqueHistory = RefManager.UniqueLog
                        .OrderByDescending(x => x.Value.State)
                        .ThenByDescending(x => x.Value.Time)
                        .Take(byte.MaxValue)
                        .ToList();

                    Packet stAckMsg = new Packet(0x171B);
                    stAckMsg.WriteUInt8((byte)uniqueHistory.Count);

                    foreach (var it in uniqueHistory)
                        UniqueHistoryService.WriteEntry(stAckMsg, it.Value);

                    await session.SendToClient(stAckMsg);
                }
                else if (type == 3)
                {
                    if (!session.SessionData.EventRegisterPacket)
                    {
                        session.SessionData.EventRegisterPacket = true;
                        if (RefManager.m_RefEventRegister.Count() > 0)
                        {
                            Packet stAckMsg = new Packet(0x171C);
                            stAckMsg.WriteUInt8(RefManager.m_RefEventRegister.Count());

                            foreach (var it in RefManager.m_RefEventRegister)
                            {
                                stAckMsg.WriteUInt8(it.Key);
                                stAckMsg.WriteUnicode(it.Value.Name);
                                stAckMsg.WriteUnicode(it.Value.Description);
                            }

                            await session.SendToClient(stAckMsg);
                        }
                    }
                }
                else if (type == 4)
                {
                    if (!session.SessionData.EventSchedulePacket)
                    {
                        session.SessionData.EventSchedulePacket = true;
                        if (RefManager.m_RefEventSchedule.Count() > 0)
                        {
                            Packet stAckMsg = new Packet(0x171E);
                            stAckMsg.WriteUInt8(RefManager.m_RefEventSchedule.Count());

                            foreach (var it in RefManager.m_RefEventSchedule)
                            {
                                stAckMsg.WriteInt32(it.Value.ID);
                                stAckMsg.WriteUnicode(it.Value.EventName);
                                stAckMsg.WriteUInt8(it.Value.Day);
                                stAckMsg.WriteUnicode(it.Value.Time);
                            }

                            await session.SendToClient(stAckMsg);
                        }
                    }
                }
                else if (type == 5)
                {
                    await ItemChestService.SendSnapshotAsync(session);
                }
                else if (type == 6)
                {
                    if (!_serverSettings.Achievements)
                        return new PacketResult();

                    if ((DateTime.UtcNow - session.SessionData.LastAchievementTitleActionUtc).TotalMilliseconds < 750)
                        return new PacketResult();

                    session.SessionData.LastAchievementTitleActionUtc = DateTime.UtcNow;
                    if (RefManager.ActiveTags.ContainsKey(session.SessionData.Charname))
                    {
                        bool removedFromDatabase = false;
                        await DatabaseJobQueue.RunAsync(() =>
                        {
                            try
                            {
                                using (var connection = new SqlConnection(Program.Connectionstring))
                                {
                                    connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                    connection.Execute(
                                        "EXEC [dbo].[Tag_Deactivate] @CharName, 0",
                                        new { CharName = session.SessionData.Charname },
                                        commandTimeout: 60);
                                    removedFromDatabase = true;
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error(ex, "Failed to remove active achievement title for {CharacterName}",
                                    session.SessionData.Charname);
                                throw;
                            }
                        });

                        if (removedFromDatabase)
                        {
                            RefManager.ActiveTags.TryRemove(session.SessionData.Charname, out _);
                            Packet stAckMsg = new Packet(0x202C);
                            stAckMsg.WriteAscii(session.SessionData.Charname);
                            await ServerManager.BroadcastPacket(stAckMsg);
                        }
                    }
                }
                else if (type == 8)
                {
                    if (!_serverSettings.Achievements)
                        return new PacketResult();

                    if ((DateTime.UtcNow - session.SessionData.LastAchievementTitleActionUtc).TotalMilliseconds < 750)
                        return new PacketResult();

                    int AchievementID = packet.ReadInt32();
                    if (!session.SessionData.CharacterAchievement.TryGetValue(AchievementID, out var playerAchievement) ||
                        playerAchievement.State != 1 ||
                        !RefManager.m_RefAchievements.TryGetValue(AchievementID, out var refAchievement) ||
                        !refAchievement.Service ||
                        refAchievement.RewardType != 0 ||
                        !RefManager.Tags.TryGetValue(refAchievement.RewardTagID, out var tagName))
                    {
                        return new PacketResult();
                    }

                    session.SessionData.LastAchievementTitleActionUtc = DateTime.UtcNow;
                    byte TagID = refAchievement.RewardTagID;
                    if (RefManager.ActiveTags.TryGetValue(session.SessionData.Charname, out var activeTagId) &&
                        activeTagId == TagID)
                    {
                        return new PacketResult();
                    }

                    bool persisted = false;
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open();
                                connection.Execute(
                                    @"EXEC [dbo].[Tag_Add] @CharID, @TagID;
                                      EXEC [dbo].[Tag_Activate] @CharID, @CharName, @TagID, 0;",
                                    new
                                    {
                                        CharID = session.SessionData.Charid,
                                        CharName = session.SessionData.Charname,
                                        TagID
                                    },
                                    commandTimeout: 60);
                                persisted = true;
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error(ex, "Failed to activate achievement tag {TagID} for {CharacterName}",
                                TagID, session.SessionData.Charname);
                            throw;
                        }
                    });

                    if (persisted)
                    {
                        RefManager.ActiveTags[session.SessionData.Charname] = TagID;
                        Packet stAckMsg = new Packet(0x202B);
                        stAckMsg.WriteAscii(session.SessionData.Charname);
                        stAckMsg.WriteAscii(tagName);
                        await ServerManager.BroadcastPacket(stAckMsg);
                    }
                }
                else if (type == 10)
                {
                    await HandleAttendanceClaimAsync(packet, session);
                }
                else if (type == 13)
                {
                    await HandleAttendanceStateRequestAsync(packet, session);
                }
                else if (type == 14)
                {
                    await HandleAttendanceRecordAsync(packet, session);
                }
                else if (type == 15)
                {
                    byte datasize = packet.ReadUInt8();
                    for (int i = 0; i < datasize; i++)
                    {
                        byte Slot = packet.ReadUInt8();
                        byte Activate = packet.ReadUInt8();
                        byte Value = packet.ReadUInt8();

                        await DatabaseJobQueue.RunAsync(() =>
                        {
                            try
                            {
                                using (var connection = new SqlConnection(Program.Connectionstring))
                                {
                                    connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                    using (var command = new SqlCommand(
                                        "EXEC [dbo].[UI_SavePotion] @CharID, @Slot, @Activate, @Value",
                                        connection))
                                    {
                                        command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                        command.Parameters.AddWithValue("@Slot", Slot);
                                        command.Parameters.AddWithValue("@Activate", Activate);
                                        command.Parameters.AddWithValue("@Value", Value);
                                        command.CommandTimeout = 60;
                                        command.ExecuteNonQuery();
                                    }
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error($"HandleHwidList hata: {ex.Message}");
                                throw;
                            }
                        });
                    }
                }
                else if (type == 18)
                {
                    uint FellowUniqueID = packet.ReadUInt32();
                    byte FellowLevel = packet.ReadUInt8();
                    if (session.SessionData.FellowPetUniqueID == FellowUniqueID && session.SessionData.FellowItemID != 0)
                    {
                        foreach (var data in RefManager.m_RefFellowData)
                        {
                            if (data.Value.ItemID == session.SessionData.FellowItemID && data.Value.SelfSkill_1 > 0 && FellowLevel >= data.Value.SelfSkill_Active_Level_1)
                            {
                                //CFilterDbSet::LivePetSkill(pState->GetFellowUniqueID(), data.second.SelfSkill_1);
                                Packet stMsg = new Packet(0x3528, false, false);
                                stMsg.WriteAscii(session.GameServerPacketKey);
                                stMsg.WriteUInt32(session.SessionData.FellowPetUniqueID);
                                stMsg.WriteInt32(data.Value.SelfSkill_1);
                                await session.SendToServer(stMsg);
                                break;

                            }
                        }
                    }
                }
                else if (type == 22)
                {
                    byte safetytype = packet.ReadUInt8();
                    if (safetytype == 0)
                    {
                        int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_LOCK_MAIL_TIME).TotalSeconds);
                        if (gecensaniye < 180)
                        {
                            int kalanSaniye = 180 - gecensaniye;
                            string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_LOCK_NOT_COMPLETED"), kalanSaniye);
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }

                        session.SessionData.LAST_LOCK_MAIL_TIME = DateTime.Now;
                        Random random = new Random();
                        int randomNumber = random.Next(10000000, 100000000);

                        string recipientEmail = session.SessionData.MailAddress;
                        string subject = "KMTGuard - Item Lock Code";
                        string body = $"<p>Merhaba {session.SessionData.Charname},</p><p>Kilitlemek istedigin itemin dogrulama kodu : {randomNumber}</p><p>Kodu girmek icin 3 dakikaniz var.</p>";

                        // E-posta g�nderimi arka planda ger�eklestirme
                        await SendEmailAsync(recipientEmail, subject, body);

                        session.SessionData.LockCode = randomNumber;

                        Packet Mailpck = new Packet(0x1209, false, false);
                        Mailpck.WriteUInt8(1); /// START TIMER
                        await session.SendToClient(Mailpck);
                    }
                    else if (safetytype == 1)
                    {
                        int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_UNLOCK_MAIL_TIME).TotalSeconds);
                        if (gecensaniye < 180)
                        {
                            int kalanSaniye = 180 - gecensaniye;
                            string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_UNLOCK_NOT_COMPLETED"), kalanSaniye);
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }

                        session.SessionData.LAST_UNLOCK_MAIL_TIME = DateTime.Now;
                        Random random = new Random();
                        int randomNumber = random.Next(10000000, 100000000);

                        string recipientEmail = session.SessionData.MailAddress;
                        string subject = "KMTGuard - Item Unlock Code";
                        string body = $"<p>Merhaba {session.SessionData.Charname},</p><p>Kilidini acmak istedigin itemin dogrulama kodu : {randomNumber}</p><p>Kodu girmek icin 3 dakikaniz var.</p>";

                        // E-posta g�nderimi arka planda ger�eklestirme
                        await SendEmailAsync(recipientEmail, subject, body);

                        session.SessionData.UnLockCode = randomNumber;

                        Packet Mailpck = new Packet(0x1209, false, false);
                        Mailpck.WriteUInt8(2); /// START TIMER // UUNLOCKER
                        await session.SendToClient(Mailpck);
                    }
                }
                else if (type == 23)
                {
                    if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                    {
                        return new PacketResult(PacketResultType.Block);
                    }
                    else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                    {
                        return new PacketResult(PacketResultType.Block);
                    }
                    string TargetName = packet.ReadUnicode();
                    int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_CHAR_INFO_DELAY).TotalSeconds);
                    if (gecensaniye < _serverSettings.SHOW_CHAR_INFO_DELAY)
                    {
                        int kalanSaniye = _serverSettings.SHOW_CHAR_INFO_DELAY - gecensaniye;
                        string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_CHAR_INFO_DELAY"), kalanSaniye);
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                    var pSession = ServerManager.AgentSessions.FindByCharName(TargetName);
                    if (pSession != null)
                    {
                        if (pSession.SessionData.HideCharInformation)
                        {
                            Packet stMsgx = new Packet(0x5015);
                            stMsgx.WriteUInt8(7);
                            await session.SendToClient(stMsgx);
                            return new PacketResult(PacketResultType.Block);
                        }
                        else
                        {
                            Packet stMsgx = new Packet(0x5015);
                            stMsgx.WriteUInt8(8);
                            await session.SendToClient(stMsgx);

                            session.SessionData.LAST_CHAR_INFO_DELAY = DateTime.Now;
                            Packet xpacket = new Packet(0x3537);
                            xpacket.WriteAscii(pSession.GameServerPacketKey);
                            xpacket.WriteAscii(session.SessionData.Charname); // its sender
                            await pSession.SendToServer(xpacket);

                        }
                    }
                }
                else if (type == 24)
                {
                    bool Value = packet.ReadBool();
                    bool SecondaryPWRememberPC = packet.ReadBool();
                    if (session.SessionData.HideCharInformation != Value)
                    {
                        await DatabaseJobQueue.RunAsync(() =>
                        {
                            try
                            {
                                using (var connection = new SqlConnection(Program.Connectionstring))
                                {
                                    connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                    connection.Execute(
                                        "EXEC [dbo].[UI_SavePlayer] @CharName, @Value",
                                        new { CharName = session.SessionData.Charname, Value },
                                        commandTimeout: 60);
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error($"HandleHwidList hata: {ex.Message}");
                                throw;
                            }
                        });
                        session.SessionData.HideCharInformation = Value;
                    }


                    if (session.SessionData.SecondPwRememberPC != SecondaryPWRememberPC)
                    {
                        var secondaryPasswordData = await sqlQueryHelper.GetSecondaryPasswordData(session.PlayerUserID);
                        if (secondaryPasswordData != null)
                        {
                            if (await sqlQueryHelper.UpdateSecondaryPasswordRememberPcAsync(
                                    session.PlayerUserID, SecondaryPWRememberPC, session.SessionData.Hwid,
                                    session.DeviceKeyThumbprint))
                                session.SessionData.SecondPwRememberPC = SecondaryPWRememberPC;
                        }
                    }


                }
                else if (type == 25)
                {
                    byte UniqueType = packet.ReadUInt8();
                    string column = UniqueType switch
                    {
                        0 => "TGCalled",
                        1 => "CerberusCalled",
                        2 => "IvyCalled",
                        3 => "UruchiCalled",
                        4 => "IsyCalled",
                        _ => string.Empty
                    };

                    if (!string.IsNullOrEmpty(column))
                    {
                        using (var connection = new SqlConnection(Program.Connectionstring))
                        {
                            await connection.OpenAsync();

                            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                            var line = await connection.QueryFirstOrDefaultAsync<_CharInstanceWorldData>(
                                $"SELECT * FROM {shardDb}.._CharInstanceWorldData WITH (NOLOCK) WHERE CharID = @CharID AND WorldID = 99",
                                new { CharID = session.SessionData.Charid });
                            if (line != null)
                            {
                                await using var transaction = await connection.BeginTransactionAsync(System.Data.IsolationLevel.Serializable);
                                await connection.ExecuteAsync(
                                    """
                                    IF NOT EXISTS (
                                        SELECT 1 FROM [dbo].[Event_ShadowDungeon] WITH (UPDLOCK, HOLDLOCK)
                                        WHERE WorldID = @WorldID AND LayerID = @LayerID
                                    )
                                    BEGIN
                                        INSERT INTO [dbo].[Event_ShadowDungeon]
                                            (CharID, WorldID, LayerID, TGCalled, CerberusCalled, IvyCalled, UruchiCalled, IsyCalled)
                                        VALUES
                                            (@CharID, @WorldID, @LayerID, 0, 0, 0, 0, 0)
                                    END
                                    """,
                                    new
                                    {
                                        CharID = session.SessionData.Charid,
                                        line.WorldID,
                                        line.LayerID
                                    },
                                    transaction);

                                int updated = await connection.ExecuteAsync(
                                    $"UPDATE [dbo].[Event_ShadowDungeon] SET [{column}] = 1 " +
                                    $"WHERE WorldID = @WorldID AND LayerID = @LayerID AND [{column}] = 0",
                                    new { line.WorldID, line.LayerID },
                                    transaction);

                                if (updated != 1)
                                {
                                    await transaction.RollbackAsync();
                                    return new PacketResult(PacketResultType.Block);
                                }

                                await transaction.CommitAsync();

                                Packet spawnPacket = new Packet(0x3538);
                                spawnPacket.WriteAscii(session.GameServerPacketKey);
                                spawnPacket.WriteUInt8(UniqueType);
                                await session.SendToServer(spawnPacket);

                                // Sorgulanan uniquelerin sayisini almak i�in
                                int uniqueSpawnCount = await connection.QueryFirstOrDefaultAsync<int>(
                                    """
                                    SELECT SUM(
                                        (CASE WHEN TGCalled = 1 THEN 1 ELSE 0 END) +
                                        (CASE WHEN CerberusCalled = 1 THEN 1 ELSE 0 END) +
                                        (CASE WHEN IvyCalled = 1 THEN 1 ELSE 0 END) +
                                        (CASE WHEN UruchiCalled = 1 THEN 1 ELSE 0 END) +
                                        (CASE WHEN IsyCalled = 1 THEN 1 ELSE 0 END)
                                    )
                                    FROM [dbo].[Event_ShadowDungeon] WITH (NOLOCK)
                                    WHERE WorldID = @WorldID AND LayerID = @LayerID
                                    """,
                                    new { line.WorldID, line.LayerID });

                                Packet uniquecountpck = new Packet(0x5040);
                                uniquecountpck.WriteInt32(uniqueSpawnCount);
                                uniquecountpck.WriteInt32(5);
                                await AgentServer.BroadcastPacketbyWorldIDAndLayerID(line.WorldID, line.LayerID, uniquecountpck);

                                //Log.Warning($"{uniqueSpawnCount}");
                                var TimerWorldandPlayer = new TimerWorldandPlayer();
                                TimerWorldandPlayer.WorldID = line.WorldID;
                                TimerWorldandPlayer.LayerID = line.LayerID;

                            }
                        }
                    }
                }
                else if (type == 27)
                {
                    await HandleLuckySpinAsync(session);
                }
                else if (type == 29)
                {
                    await SendLuckySpinRewardsAsync(session);
                }
                else if (type == 30)
                {
                    await TradeSellCaptchaService.HandleResponseAsync(packet, session);
                }
                else if (type == 31)
                {
                    await SendSpecialOffersAsync(session);
                }
                else if (type == 32)
                {
                    await HandleSpecialOfferPurchaseAsync(packet, session);
                }
                else if (type == 33)
                {
                    await HandleDropLogsRequestAsync(packet, session);
                }
                else if (type == 34)
                {
                    await HandleMonsterPossibleDropsRequestAsync(packet, session);
                }
                else if (type == 35)
                {
                    string targetName = packet.ReadAscii();
                    long wagerGold = packet.ReadInt64();
                    await PvpChallengeService.HandleChallengeRequestAsync(session, targetName, wagerGold);
                }
                else if (type == 36)
                {
                    long matchId = packet.ReadInt64();
                    bool accepted = packet.ReadUInt8() != 0;
                    await PvpChallengeService.HandleChallengeAnswerAsync(session, matchId, accepted);
                }
                else if (type == 37)
                {
                    await SendKillerAnimationsAsync(session);
                }
                else if (type == 38)
                {
                    int animationId = packet.ReadInt32();
                    await HandleKillerAnimationPurchaseAsync(session, animationId);
                }
                else if (type == 39)
                {
                    int animationId = packet.ReadInt32();
                    await HandleKillerAnimationActivateAsync(session, animationId);
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "HandleGuiPackets");
            }
            return new PacketResult(PacketResultType.Block);
        }

        private static async Task HandleAttendanceClaimAsync(Packet packet, ISession session)
        {
            if (packet.RemainingRead() != sizeof(int))
            {
                Log.Warning(
                    "Rejected malformed Attendance claim. CharID={CharID}, Remaining={Remaining}",
                    session.SessionData.Charid,
                    packet.RemainingRead());
                return;
            }

            var now = DateTime.UtcNow;
            if ((now - session.SessionData.LastAttendanceClaimRequestUtc).TotalMilliseconds < 500)
                return;

            session.SessionData.LastAttendanceClaimRequestUtc = now;
            int refRewardId = packet.ReadInt32();

            try
            {
                var result = await AttendanceService.ClaimRewardAsync(
                    session.SessionData.Charid,
                    refRewardId);

                if (result.Result == 1)
                {
                    Packet success = new Packet(0x169C);
                    success.WriteUInt8(12);
                    success.WriteUnicode(PlayerLanguage.Get("ItemMall.AddedToChest"));
                    success.WriteInt32(result.ItemID);
                    await session.SendToClient(success);
                }
                else
                {
                    await SendAttendanceNoticeAsync(
                        session,
                        result.Result == -1
                            ? PlayerLanguage.Get("Attendance.RewardNotConfigured")
                            : PlayerLanguage.Get("Attendance.RewardUnavailable"),
                        NoticeType.WARNING);

                    if (result.Result == -1)
                        await SendAttendanceRewardDefinitionsAsync(session);
                }

                await SendAttendanceStateAsync(session);
            }
            catch (Exception ex)
            {
                Log.Error(
                    ex,
                    "Attendance reward claim failed. CharID={CharID}, RefRewardID={RefRewardID}",
                    session.SessionData.Charid,
                    refRewardId);
                await SendAttendanceNoticeAsync(
                    session,
                    PlayerLanguage.Get("Attendance.RewardAddFailed"),
                    NoticeType.WARNING);

                try
                {
                    await SendAttendanceStateAsync(session);
                }
                catch (Exception stateException)
                {
                    Log.Error(
                        stateException,
                        "Attendance state recovery failed after a claim error. CharID={CharID}",
                        session.SessionData.Charid);
                }
            }
        }

        private static async Task HandleAttendanceStateRequestAsync(Packet packet, ISession session)
        {
            if (packet.RemainingRead() != 0)
            {
                Log.Warning(
                    "Rejected malformed Attendance state request. CharID={CharID}, Remaining={Remaining}",
                    session.SessionData.Charid,
                    packet.RemainingRead());
                return;
            }

            var now = DateTime.UtcNow;
            if ((now - session.SessionData.LastAttendanceStateRequestUtc).TotalMilliseconds < 250)
                return;

            session.SessionData.LastAttendanceStateRequestUtc = now;

            try
            {
                await SendAttendanceRewardDefinitionsAsync(session);
                await SendAttendanceStateAsync(session);
            }
            catch (Exception ex)
            {
                Log.Error(
                    ex,
                    "Attendance state load failed. CharID={CharID}",
                    session.SessionData.Charid);
                await SendAttendanceNoticeAsync(
                    session,
                    PlayerLanguage.Get("Attendance.Unavailable"),
                    NoticeType.WARNING);
            }
        }

        private static async Task HandleAttendanceRecordAsync(Packet packet, ISession session)
        {
            if (packet.RemainingRead() != 0)
            {
                Log.Warning(
                    "Rejected malformed Attendance record request. CharID={CharID}, Remaining={Remaining}",
                    session.SessionData.Charid,
                    packet.RemainingRead());
                return;
            }

            var now = DateTime.UtcNow;
            if ((now - session.SessionData.LastAttendanceRecordRequestUtc).TotalMilliseconds < 750)
                return;

            session.SessionData.LastAttendanceRecordRequestUtc = now;

            try
            {
                var result = await AttendanceService.RecordAsync(session.SessionData.Charid);
                session.SessionData.AttendanceDayCount = result.DayCount;

                switch (result.Result)
                {
                    case 1:
                        await SendAttendanceNoticeAsync(
                            session,
                            PlayerLanguage.Get("Attendance.Recorded"),
                            NoticeType.YELLOW_RIGHT);
                        break;

                    case 0:
                        await SendAttendanceNoticeAsync(
                            session,
                            PlayerLanguage.Get("Attendance.AlreadyRecorded"),
                            NoticeType.WARNING);
                        break;

                    case -1:
                        Log.Error(
                            "Attendance database date is behind stored player progress. CharID={CharID}, AttendanceDate={AttendanceDate:yyyy-MM-dd}",
                            session.SessionData.Charid,
                            result.AttendanceDate);
                        await SendAttendanceNoticeAsync(
                            session,
                            PlayerLanguage.Get("Attendance.DateReviewRequired"),
                            NoticeType.WARNING);
                        break;

                    default:
                        throw new InvalidOperationException(
                            $"Attendance returned unsupported result {result.Result}.");
                }

                await SendAttendanceStateAsync(session);
            }
            catch (Exception ex)
            {
                Log.Error(
                    ex,
                    "Attendance record failed. CharID={CharID}",
                    session.SessionData.Charid);
                await SendAttendanceNoticeAsync(
                    session,
                    PlayerLanguage.Get("Attendance.RecordFailed"),
                    NoticeType.WARNING);
            }
        }

        private static async Task SendAttendanceStateAsync(ISession session)
        {
            var state = await AttendanceService.GetStateAsync(session.SessionData.Charid);
            session.SessionData.AttendanceDayCount = state.DayCount;

            Packet progress = new Packet(0x208C);
            progress.WriteUInt8((byte)state.DayCount);
            await session.SendToClient(progress);

            Packet rewards = new Packet(0x209A);
            rewards.WriteUInt8((byte)state.EligibleRewardIds.Count);
            foreach (int refRewardId in state.EligibleRewardIds)
                rewards.WriteInt32(refRewardId);
            await session.SendToClient(rewards);
        }

        private static async Task SendAttendanceRewardDefinitionsAsync(ISession session)
        {
            var rewards = await AttendanceService.LoadRewardsAsync();

            Packet definitions = new Packet(0x208B);
            definitions.WriteUInt8((byte)rewards.Count);
            foreach (var reward in rewards)
            {
                definitions.WriteInt32(reward.ID);
                definitions.WriteInt32(reward.ItemID);
                definitions.WriteInt32(reward.ItemCount);
                definitions.WriteInt32(reward.DayCount);
            }
            await session.SendToClient(definitions);
        }

        private static async Task SendAttendanceNoticeAsync(
            ISession session,
            string message,
            NoticeType noticeType)
        {
            Packet notice = new Packet(0x168A);
            notice.WriteUInt8(noticeType);
            notice.WriteUnicode(message);
            await session.SendToClient(notice);
        }

        private sealed class DropLogRow
        {
            public int TotalCount { get; set; }
            public string Player { get; set; } = string.Empty;
            public string ItemCode { get; set; } = string.Empty;
            public int RefObjId { get; set; }
            public int PlusAmount { get; set; }
            public string Monster { get; set; } = string.Empty;
            public string Info { get; set; } = string.Empty;
            public string Date { get; set; } = string.Empty;
            public string Location { get; set; } = string.Empty;
        }

        private sealed class DropLogRefObjRow
        {
            public int ID { get; set; }
            public string CodeName128 { get; set; } = string.Empty;
            public string OrgObjCodeName128 { get; set; } = string.Empty;
        }

        private sealed class MonsterPossibleDropRow
        {
            public int MonsterID { get; set; }
            public string MonsterCodeName { get; set; } = string.Empty;
            public int ItemID { get; set; }
            public string ItemCodeName { get; set; } = string.Empty;
            public string ItemNameStrID { get; set; } = string.Empty;
            public int DropAmountMin { get; set; }
            public int DropAmountMax { get; set; }
            public decimal DropRatio { get; set; }
        }

        private static string BuildDropLogsQuery()
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            var logDb = SqlIdentifier.Quote(_serverSettings.LogDB);

            return $@"
;WITH LatestLogs AS
(
    SELECT TOP (500)
        CAST(ch.CharName16 AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT AS Player,
        CAST(ref.CodeName128 AS NVARCHAR(128)) COLLATE DATABASE_DEFAULT AS ItemCode,
        ref.ID AS RefObjId,
        COALESCE
        (
            TRY_CONVERT
            (
                INT,
                CASE
                    WHEN plusData.PlusPos > 0
                        THEN SUBSTRING
                        (
                            descData.DescText,
                            plusData.PlusPos + 1,
                            CASE
                                WHEN digitData.NonDigitPos > 1 THEN digitData.NonDigitPos - 1
                                ELSE 1
                            END
                        )
                    ELSE N'0'
                END
            ),
            0
        ) AS PlusAmount,
        CAST
        (
            CASE
                WHEN mobData.MobPos > 0
                    THEN SUBSTRING
                    (
                        descData.DescText,
                        mobData.MobPos,
                        CASE
                            WHEN mobEndData.CommaAfterMob > mobData.MobPos
                                THEN mobEndData.CommaAfterMob - mobData.MobPos
                            ELSE 128
                        END
                    )
                ELSE N'-'
            END
            AS NVARCHAR(128)
        ) COLLATE DATABASE_DEFAULT AS Monster,
        CAST
        (
            CASE
                WHEN varData.VarPos = 0
                     OR bracketStartData.BracketStart = 0
                     OR bracketEndData.BracketEnd = 0
                    THEN N'-'
                WHEN CAST(ch.CharName16 AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT
                     LIKE N'%' + infoData.RawInfo COLLATE DATABASE_DEFAULT + N'%'
                    THEN N'-'
                ELSE UPPER(infoData.RawInfo)
            END
            AS NVARCHAR(64)
        ) COLLATE DATABASE_DEFAULT AS Info,
        CONVERT(NVARCHAR(32), elog.EventTime, 120) COLLATE DATABASE_DEFAULT AS [Date],
        CAST(N'-' AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT AS [Location],
        elog.EventTime AS SortDate
    FROM {shardDb}.dbo._Items AS items WITH (NOLOCK)
    INNER JOIN {logDb}.dbo._LogEventItem AS elog WITH (NOLOCK)
        ON items.Serial64 = elog.Serial64
    INNER JOIN {shardDb}.dbo._Char AS ch WITH (NOLOCK)
        ON elog.CharID = ch.CharID
    INNER JOIN {shardDb}.dbo._RefObjCommon AS ref WITH (NOLOCK)
        ON elog.ItemRefID = ref.ID
    CROSS APPLY
    (
        SELECT CAST(ISNULL(elog.strDesc, N'') AS NVARCHAR(MAX)) AS DescText
    ) AS descData
    CROSS APPLY
    (
        SELECT PATINDEX(N'%+[0-9]%', descData.DescText) AS PlusPos
    ) AS plusData
    CROSS APPLY
    (
        SELECT CASE
                   WHEN plusData.PlusPos > 0
                       THEN PATINDEX
                       (
                           N'%[^0-9]%',
                           SUBSTRING(descData.DescText, plusData.PlusPos + 1, 10) + N'X'
                       )
                   ELSE 0
               END AS NonDigitPos
    ) AS digitData
    CROSS APPLY
    (
        SELECT CHARINDEX(N'MOB', descData.DescText) AS MobPos
    ) AS mobData
    CROSS APPLY
    (
        SELECT CASE
                   WHEN mobData.MobPos > 0
                       THEN CHARINDEX(N',', descData.DescText, mobData.MobPos)
                   ELSE 0
               END AS CommaAfterMob
    ) AS mobEndData
    CROSS APPLY
    (
        SELECT CHARINDEX(N'Var', descData.DescText) AS VarPos
    ) AS varData
    CROSS APPLY
    (
        SELECT CASE
                   WHEN varData.VarPos > 0
                       THEN CHARINDEX(N'[', descData.DescText, varData.VarPos)
                   ELSE 0
               END AS BracketStart
    ) AS bracketStartData
    CROSS APPLY
    (
        SELECT CASE
                   WHEN bracketStartData.BracketStart > 0
                       THEN CHARINDEX(N']', descData.DescText, bracketStartData.BracketStart + 1)
                   ELSE 0
               END AS BracketEnd
    ) AS bracketEndData
    CROSS APPLY
    (
        SELECT CASE
                   WHEN bracketStartData.BracketStart > 0
                        AND bracketEndData.BracketEnd > bracketStartData.BracketStart
                       THEN SUBSTRING
                       (
                           descData.DescText,
                           bracketStartData.BracketStart + 1,
                           bracketEndData.BracketEnd - bracketStartData.BracketStart - 1
                       )
                   ELSE N''
               END AS RawInfo
    ) AS infoData
    WHERE descData.DescText LIKE N'%MOB%'
      AND ref.CodeName128 NOT LIKE '%ARCHEMY%'
      AND ref.CodeName128 NOT LIKE '%ALCHEMY%'
      AND ref.CodeName128 NOT LIKE '%MAGICSTONE%'
      AND ref.CodeName128 NOT LIKE '%STONE%'
      AND ref.CodeName128 NOT LIKE '%ELIXIR%'
      AND ref.CodeName128 NOT LIKE '%ETC%'
      AND ref.CodeName128 NOT LIKE '%MALL%'
      AND
      (
          ref.CodeName128 LIKE 'ITEM_CH_%'
          OR ref.CodeName128 LIKE 'ITEM_EU_%'
      )
    ORDER BY elog.EventTime DESC
),
Filtered AS
(
    SELECT *
    FROM LatestLogs
    WHERE
    (
        @SearchText = N''
        OR Player LIKE N'%' + @SearchText + N'%'
        OR ItemCode LIKE N'%' + @SearchText + N'%'
        OR Monster LIKE N'%' + @SearchText + N'%'
        OR Info LIKE N'%' + @SearchText + N'%'
        OR CONVERT(NVARCHAR(16), PlusAmount) LIKE N'%' + @SearchText + N'%'
    )
    AND
    (
        @Filter = 0
        OR (@Filter = 1 AND ItemCode LIKE N'ITEM_CH_%')
        OR (@Filter = 2 AND ItemCode LIKE N'ITEM_EU_%')
    )
),
Numbered AS
(
    SELECT
        ROW_NUMBER() OVER (ORDER BY SortDate DESC) AS RowNum,
        COUNT(*) OVER () AS TotalCount,
        Player,
        ItemCode,
        RefObjId,
        PlusAmount,
        Monster,
        Info,
        [Date],
        [Location]
    FROM Filtered
)
SELECT
    TotalCount,
    Player,
    ItemCode,
    RefObjId,
    PlusAmount,
    Monster,
    Info,
    [Date],
    [Location]
FROM Numbered
WHERE RowNum BETWEEN ((@Page - 1) * @PageSize + 1) AND (@Page * @PageSize)
ORDER BY RowNum;";
        }

        private static int ResolveDropLogRefObjIdFromCache(string itemCode)
        {
            if (string.IsNullOrWhiteSpace(itemCode))
                return 0;

            foreach (var refObj in RefManager.RefObjCommons.Values)
            {
                if (string.Equals(refObj.CodeName128, itemCode, StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(refObj.OrgObjCodeName128, itemCode, StringComparison.OrdinalIgnoreCase))
                    return refObj.ID;
            }

            return 0;
        }

        private async Task ResolveDropLogRefObjIdsAsync(SqlConnection connection, IReadOnlyList<DropLogRow> rows)
        {
            foreach (var row in rows)
            {
                row.RefObjId = ResolveDropLogRefObjIdFromCache(row.ItemCode);
            }

            var missingCodes = rows
                .Where(row => row.RefObjId <= 0 && !string.IsNullOrWhiteSpace(row.ItemCode))
                .Select(row => row.ItemCode)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();

            if (missingCodes.Length == 0)
                return;

            try
            {
                var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                var refRows = await connection.QueryAsync<DropLogRefObjRow>(
                    $"SELECT ID, CodeName128, OrgObjCodeName128 FROM {shardDb}.._RefObjCommon WITH (NOLOCK) WHERE CodeName128 IN @Codes OR OrgObjCodeName128 IN @Codes",
                    new { Codes = missingCodes },
                    commandTimeout: 30);

                var refMap = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
                foreach (var refRow in refRows)
                {
                    if (!string.IsNullOrWhiteSpace(refRow.CodeName128) && !refMap.ContainsKey(refRow.CodeName128))
                        refMap.Add(refRow.CodeName128, refRow.ID);
                    if (!string.IsNullOrWhiteSpace(refRow.OrgObjCodeName128) && !refMap.ContainsKey(refRow.OrgObjCodeName128))
                        refMap.Add(refRow.OrgObjCodeName128, refRow.ID);
                }

                foreach (var row in rows)
                {
                    if (row.RefObjId <= 0 && refMap.TryGetValue(row.ItemCode, out var refObjId))
                        row.RefObjId = refObjId;
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Drop Logs RefObjId resolve failed");
            }
        }

        private async Task HandleDropLogsRequestAsync(Packet packet, ISession session)
        {
            int page = Math.Clamp(packet.ReadInt32(), 1, 100000);
            int pageSize = Math.Clamp(packet.ReadInt32(), 1, 10);
            string searchText = packet.ReadAscii() ?? string.Empty;
            if (searchText.Length > 64)
                searchText = searchText[..64];

            byte filter = packet.ReadUInt8();
            if (filter > 2)
                filter = 0;

            List<DropLogRow> rows = new();
            bool success = true;
            int totalCount = 0;

            try
            {
                using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync();
                var result = await connection.QueryAsync<DropLogRow>(
                    BuildDropLogsQuery(),
                    new
                    {
                        Page = page,
                        PageSize = pageSize,
                        SearchText = searchText,
                        Filter = filter
                    },
                    commandTimeout: 60);

                rows = result.Take(pageSize).ToList();
                totalCount = rows.Count > 0 ? rows[0].TotalCount : 0;
                await ResolveDropLogRefObjIdsAsync(connection, rows);
            }
            catch (Exception ex)
            {
                success = false;
                rows.Clear();
                totalCount = 0;
                Log.Warning(
                    ex,
                    "Drop Logs query failed for {CharName}. ShardDB={ShardDB} LogDB={LogDB}",
                    session.SessionData.Charname,
                    _serverSettings.ShardDB,
                    _serverSettings.LogDB);
            }

            Packet response = new Packet(0x2074);
            response.WriteUInt8(success ? (byte)1 : (byte)0);
            response.WriteInt32(totalCount);
            response.WriteInt32(page);
            response.WriteInt32(pageSize);
            response.WriteInt32(rows.Count);
            foreach (var row in rows)
            {
                response.WriteUnicode(row.Player ?? string.Empty);
                response.WriteUnicode(row.ItemCode ?? string.Empty);
                response.WriteInt32(row.RefObjId);
                response.WriteInt32(row.PlusAmount);
                response.WriteUnicode(row.Monster ?? string.Empty);
                response.WriteUnicode(string.IsNullOrWhiteSpace(row.Info) ? "-" : row.Info);
                response.WriteUnicode(row.Date ?? string.Empty);
                response.WriteUnicode(string.IsNullOrWhiteSpace(row.Location) ? "-" : row.Location);
            }

            await session.SendToClient(response);
        }

        private async Task HandleMonsterPossibleDropsRequestAsync(Packet packet, ISession session)
        {
            int monsterId = Math.Clamp(packet.ReadInt32(), 1, int.MaxValue);
            List<MonsterPossibleDropRow> rows = new();
            bool success = true;

            try
            {
                using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync();
                var result = await connection.QueryAsync<MonsterPossibleDropRow>(
                    "KMTGuard.dbo.DropMonster_Window",
                    new { MonsterID = monsterId },
                    commandType: CommandType.StoredProcedure,
                    commandTimeout: 60);

                rows = result
                    .Where(row => row.ItemID > 0)
                    .GroupBy(row => row.ItemID)
                    .Select(group => group.First())
                    .Take(900)
                    .ToList();
            }
            catch (Exception ex)
            {
                success = false;
                rows.Clear();
                Log.Warning(ex, "Monster possible drops query failed for MonsterID={MonsterID} CharName={CharName}", monsterId, session.SessionData.Charname);
            }

            Packet response = new Packet(0x2075);
            response.WriteUInt8(success ? (byte)1 : (byte)0);
            response.WriteInt32(monsterId);
            response.WriteInt32(rows.Count);
            foreach (var row in rows)
            {
                response.WriteInt32(row.ItemID);
            }

            await session.SendToClient(response);
        }
        private async Task HandleLuckySpinAsync(ISession session)
        {
            if (!_serverSettings.EnableLuckySpin)
            {
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.Disabled"));
                return;
            }

            if (_serverSettings.LuckySpinPrice <= 0)
            {
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.PriceNotConfigured"));
                return;
            }

            var rewards = RefManager.m_LuckySpin.Values
                .Where(reward => reward.ItemID > 0 && reward.Amount > 0 && reward.Rate > 0)
                .OrderBy(reward => reward.ID)
                .ToList();
            if (rewards.Count == 0)
            {
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.NoRewards"));
                return;
            }

            long totalRate = rewards.Sum(reward => (long)reward.Rate);
            if (totalRate <= 0 || totalRate > int.MaxValue)
            {
                Log.Error("Lucky Spin rewards have an invalid total rate.");
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.InvalidConfiguration"));
                return;
            }

            int roll = Random.Shared.Next(1, (int)totalRate + 1);
            int accumulatedRate = 0;
            _LuckySpinRewards? selectedReward = null;
            foreach (var reward in rewards)
            {
                accumulatedRate += reward.Rate;
                if (roll <= accumulatedRate)
                {
                    selectedReward = reward;
                    break;
                }
            }

            if (selectedReward == null)
            {
                Log.Error("Lucky Spin could not select a reward although rewards were loaded.");
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.SelectionFailed"));
                return;
            }

            var rewardItem = await RefManager.GetRefObjCommonAsync(selectedReward.ItemID);
            if (rewardItem == null || string.IsNullOrWhiteSpace(rewardItem.CodeName128))
            {
                Log.Error("Lucky Spin reward {RewardId} references missing item {ItemId}.", selectedReward.ID, selectedReward.ItemID);
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.InvalidItem"));
                return;
            }

            bool charged;
            if (_serverSettings.EnableLuckySpinSilk)
            {
                SilkInfo? silk = await ChargeSilkAsync(session.SessionData.JID, _serverSettings.LuckySpinPrice);
                charged = silk != null;
                if (silk != null)
                {
                    Packet silkUpdate = new Packet(0x3527);
                    silkUpdate.WriteAscii(session.GameServerPacketKey);
                    silkUpdate.WriteInt32(silk.silk_own);
                    silkUpdate.WriteInt32(silk.silk_gift);
                    silkUpdate.WriteInt32(silk.silk_point);
                    await session.SendToServer(silkUpdate);
                }
            }
            else
            {
                charged = await ChargeLuckySpinGoldAsync(session.SessionData.Charid, _serverSettings.LuckySpinPrice);
            }

            if (!charged)
            {
                await SendLuckySpinWarningAsync(
                    session,
                    _serverSettings.EnableLuckySpinSilk
                        ? PlayerLanguage.Get("Purchase.NotEnoughSilk")
                        : PlayerLanguage.Get("Purchase.NotEnoughGold"));
                return;
            }

            try
            {
                bool rewardAdded = await sqlQueryHelper.AddItemToChest2(
                    session.SessionData.Charid,
                    selectedReward.ItemID,
                    selectedReward.Amount,
                    "Lucky Spin",
                    0);
                if (!rewardAdded)
                    throw new InvalidOperationException("Item Chest rejected the Lucky Spin reward.");

                await DatabaseJobQueue.RunAsync(() =>
                {
                    using var connection = new SqlConnection(Program.Connectionstring);
                    connection.Open();
                    connection.Execute(
                        "INSERT INTO [dbo].[LuckySpin_Log] (CharID, JID, RewardID, ItemID, Amount, Price, PaymentType) VALUES (@CharID, @JID, @RewardID, @ItemID, @Amount, @Price, @PaymentType)",
                        new
                        {
                            CharID = session.SessionData.Charid,
                            JID = session.SessionData.JID,
                            RewardID = selectedReward.ID,
                            ItemID = selectedReward.ItemID,
                            Amount = selectedReward.Amount,
                            Price = _serverSettings.LuckySpinPrice,
                            PaymentType = _serverSettings.EnableLuckySpinSilk ? "Silk" : "Gold"
                        },
                        commandTimeout: 60);
                });

                Packet spinResult = new Packet(0x2070);
                spinResult.WriteInt32(rewards.IndexOf(selectedReward));
                await session.SendToClient(spinResult);

                Packet rewardNotice = new Packet(0x169C);
                rewardNotice.WriteUInt8(12);
                rewardNotice.WriteUnicode(PlayerLanguage.Get("LuckySpin.RewardAddedToChest"));
                rewardNotice.WriteInt32(selectedReward.ItemID);
                await session.SendToClient(rewardNotice);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Lucky Spin reward delivery failed for {CharacterName}", session.SessionData.Charname);
                await SendLuckySpinWarningAsync(session, PlayerLanguage.Get("LuckySpin.DeliveryFailed"));
            }
        }

        private async Task<bool> ChargeLuckySpinGoldAsync(int charId, int price)
        {
            if (charId <= 0 || price <= 0)
                return false;

            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            int affected = await connection.ExecuteAsync(
                $@"UPDATE {shardDb}.._Char
                   SET RemainGold = RemainGold - @Price
                   WHERE CharID = @CharID AND RemainGold >= @Price",
                new { CharID = charId, Price = price });
            return affected == 1;
        }

        private static async Task SendLuckySpinRewardsAsync(ISession session)
        {
            var rewards = RefManager.m_LuckySpin.Values
                .Where(reward => reward.ItemID > 0 && reward.Amount > 0 && reward.Rate > 0)
                .OrderBy(reward => reward.ID)
                .ToList();

            Packet response = new Packet(0x206F);
            response.WriteInt32(rewards.Count);
            foreach (var reward in rewards)
            {
                response.WriteInt32(reward.ItemID);
                response.WriteInt32(reward.Amount);
            }
            await session.SendToClient(response);
            Log.Verbose("Lucky Spin reward catalog sent to {CharacterName}: {RewardCount}", session.SessionData.Charname, rewards.Count);
        }

        private static async Task SendLuckySpinWarningAsync(ISession session, string message)
        {
            Packet notice = new Packet(0x168A);
            notice.WriteUInt8(NoticeType.WARNING);
            notice.WriteUnicode(message);
            await session.SendToClient(notice);
        }

        private static List<_SpecialOffer> GetActiveSpecialOffers()
        {
            DateTime now = DateTime.Now;
            return RefManager.m_SpecialOffers.Values
                .Where(offer =>
                    offer.Service &&
                    offer.ItemID > 0 &&
                    offer.ItemCount > 0 &&
                    offer.SalePrice > 0 &&
                    offer.PaymentType <= 1 &&
                    (!offer.StartDate.HasValue || offer.StartDate.Value <= now) &&
                    (!offer.EndDate.HasValue || offer.EndDate.Value >= now))
                .OrderBy(offer => offer.SortOrder)
                .ThenBy(offer => offer.ID)
                .ToList();
        }

        private static async Task SendSpecialOffersAsync(ISession session)
        {
            var offers = GetActiveSpecialOffers();

            Packet response = new Packet(0x2072);
            response.WriteInt32(offers.Count);
            foreach (var offer in offers)
            {
                response.WriteInt32(offer.ID);
                response.WriteInt32(offer.ItemID);
                response.WriteInt32(offer.ItemCount);
                response.WriteInt32(offer.MainPrice);
                response.WriteInt32(offer.SalePrice);
                response.WriteUInt8(offer.PaymentType);
                response.WriteUInt8(offer.PreviewMode);
                response.WriteInt32(offer.PreviewRefObjID);
                response.WriteInt32(offer.SortOrder);
                response.WriteUnicode(string.IsNullOrWhiteSpace(offer.Title)
                    ? PlayerLanguage.Get("SpecialOffer.DefaultTitle", offer.ID)
                    : offer.Title);
                response.WriteAscii(offer.PreviewImagePath ?? string.Empty);
            }

            await session.SendToClient(response);
            Log.Verbose("Special Offers catalog sent to {CharacterName}: {OfferCount}", session.SessionData.Charname, offers.Count);
        }

        private async Task HandleSpecialOfferPurchaseAsync(Packet packet, ISession session)
        {
            int offerId = packet.ReadInt32();

            if (!_serverSettings.EnableSpecialOffers)
            {
                await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("SpecialOffer.Disabled"));
                return;
            }

            var offer = GetActiveSpecialOffers().FirstOrDefault(x => x.ID == offerId);
            if (offer == null)
            {
                await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("SpecialOffer.Unavailable"));
                return;
            }

            if (offer.SalePrice <= 0 || offer.ItemCount <= 0 || offer.ItemID <= 0)
            {
                await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("SpecialOffer.InvalidConfiguration"));
                return;
            }

            string codeName = offer.CodeName128?.Trim() ?? string.Empty;
            if (string.IsNullOrWhiteSpace(codeName))
            {
                var refObj = await RefManager.GetRefObjCommonAsync(offer.ItemID);
                codeName = refObj?.CodeName128 ?? string.Empty;
            }

            if (string.IsNullOrWhiteSpace(codeName))
            {
                Log.Error("Special Offer {OfferId} references missing item {ItemId}.", offer.ID, offer.ItemID);
                await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("SpecialOffer.InvalidItem"));
                return;
            }

            try
            {
                if (offer.PaymentType == 0)
                {
                    SilkInfo? silk = await PurchaseSpecialOfferSilkAsync(session, offer);
                    if (silk == null)
                    {
                        await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("Purchase.NotEnoughSilk"));
                        return;
                    }

                    Packet silkUpdate = new Packet(0x3527);
                    silkUpdate.WriteAscii(session.GameServerPacketKey);
                    silkUpdate.WriteInt32(silk.silk_own);
                    silkUpdate.WriteInt32(silk.silk_gift);
                    silkUpdate.WriteInt32(silk.silk_point);
                    await session.SendToServer(silkUpdate);

                }
                else if (offer.PaymentType == 1)
                {
                    bool purchased = await PurchaseSpecialOfferGoldAsync(session, offer);
                    if (!purchased)
                    {
                        await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("Purchase.NotEnoughGold"));
                        return;
                    }
                }
                else
                {
                    await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("SpecialOffer.InvalidPaymentType"));
                    return;
                }

                Packet rewardNotice = new Packet(0x169C);
                rewardNotice.WriteUInt8(12);
                rewardNotice.WriteUnicode(PlayerLanguage.Get("SpecialOffer.AddedToChest"));
                rewardNotice.WriteInt32(offer.ItemID);
                await session.SendToClient(rewardNotice);

                await SendSpecialOfferResultAsync(session, true, offerId, PlayerLanguage.Get("SpecialOffer.PurchaseCompleted"));
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Special Offer purchase failed for {CharacterName}, offer {OfferId}", session.SessionData.Charname, offer.ID);
                await SendSpecialOfferResultAsync(session, false, offerId, PlayerLanguage.Get("SpecialOffer.PurchaseFailed"));
            }
        }

        private static async Task AddSpecialOfferItemToChestAsync(SqlConnection connection, SqlTransaction transaction, ISession session, _SpecialOffer offer)
        {
            await connection.ExecuteAsync(
                @"EXEC [dbo].[Item_AddChest]
                    @CharID = @CharID,
                    @ItemRefObjID = @ItemID,
                    @Quantity = @ItemCount,
                    @From = @Source,
                    @Plus = @Plus",
                new
                {
                    CharID = session.SessionData.Charid,
                    ItemID = offer.ItemID,
                    ItemCount = offer.ItemCount,
                    Source = "Special Offers",
                    Plus = 0
                },
                transaction,
                commandTimeout: 60);
        }

        private async Task<SilkInfo?> PurchaseSpecialOfferSilkAsync(ISession session, _SpecialOffer offer)
        {
            var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

            try
            {
                var silk = await connection.QuerySingleOrDefaultAsync<SilkInfo>(
                    $@"SELECT TOP (1) silk_own, silk_gift, silk_point
                       FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK)
                       WHERE JID = @JID",
                    new { JID = session.SessionData.JID },
                    transaction);

                if (silk == null || silk.silk_own < offer.SalePrice)
                {
                    await transaction.RollbackAsync();
                    return null;
                }

                int affected = await connection.ExecuteAsync(
                    $@"UPDATE {accountDb}..SK_Silk
                       SET silk_own = silk_own - @Price
                       WHERE JID = @JID AND silk_own >= @Price",
                    new
                    {
                        JID = session.SessionData.JID,
                        Price = offer.SalePrice
                    },
                    transaction);

                if (affected != 1)
                {
                    await transaction.RollbackAsync();
                    return null;
                }

                await AddSpecialOfferItemToChestAsync(connection, transaction, session, offer);

                await connection.ExecuteAsync(
                    @"INSERT INTO [dbo].[Offer_PurchaseLog]
                      (OfferID, CharID, JID, ItemID, ItemCount, Price, PaymentType)
                      VALUES (@OfferID, @CharID, @JID, @ItemID, @ItemCount, @Price, @PaymentType)",
                    new
                    {
                        OfferID = offer.ID,
                        CharID = session.SessionData.Charid,
                        JID = session.SessionData.JID,
                        ItemID = offer.ItemID,
                        ItemCount = offer.ItemCount,
                        Price = offer.SalePrice,
                        PaymentType = "Silk"
                    },
                    transaction,
                    commandTimeout: 60);

                await transaction.CommitAsync();
                silk.silk_own -= offer.SalePrice;
                return silk;
            }
            catch
            {
                await transaction.RollbackAsync();
                throw;
            }
        }

        private async Task<bool> PurchaseSpecialOfferGoldAsync(ISession session, _SpecialOffer offer)
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

            try
            {
                int affected = await connection.ExecuteAsync(
                    $@"UPDATE {shardDb}.._Char
                       SET RemainGold = RemainGold - @Price
                       WHERE CharID = @CharID AND RemainGold >= @Price",
                    new
                    {
                        CharID = session.SessionData.Charid,
                        Price = offer.SalePrice
                    },
                    transaction);

                if (affected != 1)
                {
                    await transaction.RollbackAsync();
                    return false;
                }

                await AddSpecialOfferItemToChestAsync(connection, transaction, session, offer);

                await connection.ExecuteAsync(
                    @"INSERT INTO [dbo].[Offer_PurchaseLog]
                      (OfferID, CharID, JID, ItemID, ItemCount, Price, PaymentType)
                      VALUES (@OfferID, @CharID, @JID, @ItemID, @ItemCount, @Price, @PaymentType)",
                    new
                    {
                        OfferID = offer.ID,
                        CharID = session.SessionData.Charid,
                        JID = session.SessionData.JID,
                        ItemID = offer.ItemID,
                        ItemCount = offer.ItemCount,
                        Price = offer.SalePrice,
                        PaymentType = "Gold"
                    },
                    transaction,
                    commandTimeout: 60);

                await transaction.CommitAsync();
                return true;
            }
            catch
            {
                await transaction.RollbackAsync();
                throw;
            }
        }

        private static async Task SendSpecialOfferResultAsync(ISession session, bool success, int offerId, string message)
        {
            Packet response = new Packet(0x2073);
            response.WriteUInt8(success ? (byte)1 : (byte)0);
            response.WriteInt32(offerId);
            response.WriteUnicode(message);
            await session.SendToClient(response);

            if (!success)
            {
                Packet notice = new Packet(0x168A);
                notice.WriteUInt8(NoticeType.WARNING);
                notice.WriteUnicode(message);
                await session.SendToClient(notice);
            }
        }

        private static List<_KillerAnimation> GetActiveKillerAnimations()
        {
            return RefManager.m_KillerAnimations.Values
                .Where(animation =>
                    animation.Service &&
                    animation.ID > 0 &&
                    animation.AnimationID > 0 &&
                    animation.AnimationID <= 500 &&
                    animation.Price >= 0 &&
                    animation.PaymentType <= 1)
                .OrderBy(animation => animation.SortOrder)
                .ThenBy(animation => animation.ID)
                .ToList();
        }

        private static async Task SendKillerAnimationsAsync(ISession session)
        {
            var animations = GetActiveKillerAnimations();
            HashSet<int> owned = new();
            int activeId = 0;

            await using (var connection = new SqlConnection(Program.Connectionstring))
            {
                await connection.OpenAsync();
                var rows = await connection.QueryAsync<int>(
                    "SELECT AnimationRefID FROM [dbo].[KillerAnimation_Owned] WITH (NOLOCK) WHERE CharID = @CharID",
                    new { CharID = session.SessionData.Charid });
                owned = rows.ToHashSet();

                activeId = await connection.QueryFirstOrDefaultAsync<int>(
                    "SELECT AnimationRefID FROM [dbo].[KillerAnimation_Active] WITH (NOLOCK) WHERE CharID = @CharID",
                    new { CharID = session.SessionData.Charid });
            }

            Packet response = new Packet(0x2077);
            response.WriteInt32(animations.Count);
            response.WriteInt32(activeId);
            foreach (var animation in animations)
            {
                bool isOwned = owned.Contains(animation.ID);
                response.WriteInt32(animation.ID);
                response.WriteInt32(animation.AnimationID);
                response.WriteInt32(animation.Price);
                response.WriteUInt8(animation.PaymentType);
                response.WriteInt32(animation.SortOrder);
                response.WriteUInt8(isOwned ? (byte)1 : (byte)0);
                response.WriteUInt8(activeId == animation.ID ? (byte)1 : (byte)0);
                response.WriteUnicode(string.IsNullOrWhiteSpace(animation.DisplayName)
                    ? $"Animation #{animation.ID}"
                    : animation.DisplayName);
            }

            await session.SendToClient(response);
        }

        private async Task HandleKillerAnimationPurchaseAsync(ISession session, int animationRefId)
        {
            var animation = GetActiveKillerAnimations().FirstOrDefault(x => x.ID == animationRefId);
            if (animation == null)
            {
                await SendKillerAnimationResultAsync(session, false, 1, animationRefId, PlayerLanguage.Get("KillerAnimation.Unavailable"));
                return;
            }

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();

            bool alreadyOwned = await connection.QueryFirstOrDefaultAsync<int>(
                "SELECT 1 FROM [dbo].[KillerAnimation_Owned] WITH (NOLOCK) WHERE CharID = @CharID AND AnimationRefID = @AnimationRefID",
                new { CharID = session.SessionData.Charid, AnimationRefID = animation.ID }) == 1;
            if (alreadyOwned)
            {
                await SendKillerAnimationResultAsync(session, false, 1, animation.ID, PlayerLanguage.Get("KillerAnimation.AlreadyOwned"));
                return;
            }

            await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);
            SilkInfo? silk = null;
            try
            {
                if (animation.Price > 0 && animation.PaymentType == 0)
                {
                    var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
                    silk = await connection.QuerySingleOrDefaultAsync<SilkInfo>(
                        $@"SELECT TOP (1) silk_own, silk_gift, silk_point
                           FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK)
                           WHERE JID = @JID",
                        new { JID = session.SessionData.JID },
                        transaction);

                    if (silk == null || silk.silk_own < animation.Price)
                    {
                        await transaction.RollbackAsync();
                        await SendKillerAnimationResultAsync(session, false, 1, animation.ID, PlayerLanguage.Get("Purchase.NotEnoughSilk"));
                        return;
                    }

                    int affected = await connection.ExecuteAsync(
                        $@"UPDATE {accountDb}..SK_Silk
                           SET silk_own = silk_own - @Price
                           WHERE JID = @JID AND silk_own >= @Price",
                        new { Price = animation.Price, JID = session.SessionData.JID },
                        transaction);
                    if (affected != 1)
                    {
                        await transaction.RollbackAsync();
                        await SendKillerAnimationResultAsync(session, false, 1, animation.ID, PlayerLanguage.Get("Purchase.NotEnoughSilk"));
                        return;
                    }

                    silk.silk_own -= animation.Price;
                }
                else if (animation.Price > 0 && animation.PaymentType == 1)
                {
                    var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                    int affected = await connection.ExecuteAsync(
                        $@"UPDATE {shardDb}.._Char
                           SET RemainGold = RemainGold - @Price
                           WHERE CharID = @CharID AND RemainGold >= @Price",
                        new { CharID = session.SessionData.Charid, Price = animation.Price },
                        transaction);
                    if (affected != 1)
                    {
                        await transaction.RollbackAsync();
                        await SendKillerAnimationResultAsync(session, false, 1, animation.ID, PlayerLanguage.Get("Purchase.NotEnoughGold"));
                        return;
                    }
                }

                await connection.ExecuteAsync(
                    @"INSERT INTO [dbo].[KillerAnimation_Owned] (CharID, JID, AnimationRefID)
                      VALUES (@CharID, @JID, @AnimationRefID)",
                    new
                    {
                        CharID = session.SessionData.Charid,
                        JID = session.SessionData.JID,
                        AnimationRefID = animation.ID
                    },
                    transaction);

                await connection.ExecuteAsync(
                    @"INSERT INTO [dbo].[KillerAnimation_PurchaseLog]
                      (CharID, JID, AnimationRefID, Price, PaymentType)
                      VALUES (@CharID, @JID, @AnimationRefID, @Price, @PaymentType)",
                    new
                    {
                        CharID = session.SessionData.Charid,
                        JID = session.SessionData.JID,
                        AnimationRefID = animation.ID,
                        Price = animation.Price,
                        PaymentType = animation.PaymentType == 1 ? "Gold" : "Silk"
                    },
                    transaction);

                await transaction.CommitAsync();

                if (silk != null)
                {
                    Packet silkUpdate = new Packet(0x3527);
                    silkUpdate.WriteAscii(session.GameServerPacketKey);
                    silkUpdate.WriteInt32(silk.silk_own);
                    silkUpdate.WriteInt32(silk.silk_gift);
                    silkUpdate.WriteInt32(silk.silk_point);
                    await session.SendToServer(silkUpdate);
                }

                await SendKillerAnimationResultAsync(session, true, 1, animation.ID, PlayerLanguage.Get("KillerAnimation.Purchased"));
                await SendKillerAnimationsAsync(session);
            }
            catch (Exception ex)
            {
                await transaction.RollbackAsync();
                Log.Error(ex, "Killer animation purchase failed for {CharacterName}, animation {AnimationId}", session.SessionData.Charname, animation.ID);
                await SendKillerAnimationResultAsync(session, false, 1, animation.ID, PlayerLanguage.Get("KillerAnimation.PurchaseFailed"));
            }
        }

        private static async Task HandleKillerAnimationActivateAsync(ISession session, int animationRefId)
        {
            var animation = GetActiveKillerAnimations().FirstOrDefault(x => x.ID == animationRefId);
            if (animation == null)
            {
                await SendKillerAnimationResultAsync(session, false, 2, animationRefId, PlayerLanguage.Get("KillerAnimation.Unavailable"));
                return;
            }

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();

            bool owned = await connection.QueryFirstOrDefaultAsync<int>(
                "SELECT 1 FROM [dbo].[KillerAnimation_Owned] WITH (NOLOCK) WHERE CharID = @CharID AND AnimationRefID = @AnimationRefID",
                new { CharID = session.SessionData.Charid, AnimationRefID = animation.ID }) == 1;
            if (!owned)
            {
                await SendKillerAnimationResultAsync(session, false, 2, animation.ID, PlayerLanguage.Get("KillerAnimation.BuyBeforeActivating"));
                return;
            }

            await connection.ExecuteAsync(
                @"MERGE [dbo].[KillerAnimation_Active] AS target
                  USING (SELECT @CharID AS CharID, @AnimationRefID AS AnimationRefID) AS source
                     ON target.CharID = source.CharID
                  WHEN MATCHED THEN
                      UPDATE SET AnimationRefID = source.AnimationRefID, UpdatedAt = SYSUTCDATETIME()
                  WHEN NOT MATCHED THEN
                      INSERT (CharID, AnimationRefID) VALUES (source.CharID, source.AnimationRefID);",
                new { CharID = session.SessionData.Charid, AnimationRefID = animation.ID });

            await SendKillerAnimationResultAsync(session, true, 2, animation.ID, PlayerLanguage.Get("KillerAnimation.Activated"));
            await SendKillerAnimationsAsync(session);
        }

        private static async Task SendKillerAnimationResultAsync(ISession session, bool success, byte action, int animationId, string message)
        {
            Packet response = new Packet(0x2078);
            response.WriteUInt8(success ? (byte)1 : (byte)0);
            response.WriteUInt8(action);
            response.WriteInt32(animationId);
            response.WriteUnicode(message);
            await session.SendToClient(response);

            if (!success)
            {
                Packet notice = new Packet(0x168A);
                notice.WriteUInt8(NoticeType.WARNING);
                notice.WriteUnicode(message);
                await session.SendToClient(notice);
            }
        }

        private static Task SendEmailAsync(string recipient, string subject, string body)
        {
            EmailService emailService = new EmailService();
            return emailService.SendEmailAsync(recipient, subject, body);
        }

        private async Task<PacketResult> GInterfaceIsReady(Packet packet, ISession session, object obj)
        {
            try
            {
                if (session.SessionData.FirstSpawn)
                {
                    Packet stMsgx = new Packet(0x5015);
                    stMsgx.WriteUInt8(9);
                    stMsgx.WriteBool(session.SessionData.HideCharInformation);
                    stMsgx.WriteBool(session.SessionData.SecondPwRememberPC);
                    await session.SendToClient(stMsgx);

                    Packet Mailpck = new Packet(0x1209);
                    Mailpck.WriteUInt8(0);
                    if (session.SessionData.MailAddress != null)
                        Mailpck.WriteAscii(session.SessionData.MailAddress);
                    else
                        Mailpck.WriteAscii("");
                    await session.SendToClient(Mailpck);

                    int nCharDBID = session.SessionData.Charid;
                    IEnumerable<_MacroAutoPotion> MacroAutoPotion;
                    IEnumerable<_MacroSetting> MacroSettings;
                    using (var connection = new SqlConnection(Program.Connectionstring))
                    {
                        await connection.OpenAsync();

                        if (_serverSettings.Achievements)
                        {
                            await connection.ExecuteAsync(
                                "EXEC [dbo].[Achievement_AddPlayer] @CharID",
                                new { CharID = nCharDBID },
                                commandTimeout: 60);
                        }

                        // Ilk sorgu: _ActiveTitleColors tablosu
                        var titles = await connection.QueryAsync<int>(
                            "SELECT TitleID FROM [dbo].[PlayerTitles] WHERE CharID = @CharID",
                            new { CharID = nCharDBID });
                        foreach (var item in titles)
                        {
                            if (!session.SessionData.PlayerTitles.Contains(item))
                            {
                                session.SessionData.PlayerTitles.Add(item);
                            }
                        }

                        var colors = await connection.QueryAsync<PlayerTitleColor>(
                            "SELECT * FROM [dbo].[PlayerTitleColors] WITH (NOLOCK) WHERE CharID = @CharID",
                            new { CharID = nCharDBID });
                        foreach (var item in colors)
                        {
                            if (!session.SessionData.PlayerTitleColors.ContainsKey(item.ID))
                            {
                                session.SessionData.PlayerTitleColors.TryAdd(item.ID, item);
                            }
                        }

                        var icons = await connection.QueryAsync<PlayerIcon>(
                            @"SELECT ID, CharID, IconID, CAST(0 AS TINYINT) AS Side
                              FROM [dbo].[PlayerLeftIcons] WITH (NOLOCK)
                              WHERE CharID = @CharID
                              UNION ALL
                              SELECT ID, CharID, IconID, CAST(1 AS TINYINT) AS Side
                              FROM [dbo].[PlayerRightIcons] WITH (NOLOCK)
                              WHERE CharID = @CharID",
                            new { CharID = nCharDBID });
                        foreach (var item in icons)
                        {
                            if (!session.SessionData.PlayerIcons.ContainsKey(item.ID))
                            {
                                session.SessionData.PlayerIcons.TryAdd(item.ID, item);
                            }
                        }

                        session.SessionData.CharacterChest.Clear();
                        session.SessionData.PendingChestClaims.Clear();
                        var chest = await connection.QueryAsync<_ItemChest>(
                            @"SELECT
                                  C.ID,
                                  C.CharID,
                                  C.ItemCodeName,
                                  C.ItemID,
                                  C.Quantity,
                                  C.[Date],
                                  C.[Type],
                                  CONVERT(TINYINT, C.Plus) AS Plus
                              FROM dbo.Item_Chest AS C WITH (READCOMMITTEDLOCK)
                              WHERE C.CharID = @CharID
                                AND NOT EXISTS
                                (
                                    SELECT 1
                                    FROM dbo.Item_ChestClaim AS ClaimRow WITH (READCOMMITTEDLOCK)
                                    WHERE ClaimRow.ChestID = C.ID
                                )
                              ORDER BY C.ID",
                            new { CharID = nCharDBID });
                        foreach (var item in chest)
                        {
                            session.SessionData.CharacterChest.TryAdd(item.ID, item);
                        }

                        session.SessionData.CharacterAchievement.Clear();
                        session.SessionData.CharacterAchievementCondition.Clear();

                        var achievement = await connection.QueryAsync<_Achievement>(
                            @"SELECT player.ID, player.CharID, player.RefAchievementID, player.State
                              FROM [dbo].[Achievement_Players] AS player
                              INNER JOIN [dbo].[Achievement_List] AS reference
                                  ON reference.ID = player.RefAchievementID
                              WHERE player.CharID = @CharID
                                AND reference.Service = 1",
                            new { CharID = nCharDBID });
                        foreach (var item in achievement)
                        {
                            if (!session.SessionData.CharacterAchievement.ContainsKey(item.RefAchievementID))
                            {
                                session.SessionData.CharacterAchievement.TryAdd(item.RefAchievementID, item);
                            }
                        }

                        var achievementcon = await connection.QueryAsync<_AchievementCondition>(
                            @"SELECT playerCondition.ID,
                                     playerCondition.CharID,
                                     playerCondition.AchievementID AS PlayerAchievementID,
                                     referenceCondition.RefAchievementID,
                                     playerCondition.RefAchievementConditionID,
                                     playerCondition.ProgressCount
                              FROM [dbo].[Achievement_PlayerConditions] AS playerCondition
                              INNER JOIN [dbo].[Achievement_Conditions] AS referenceCondition
                                  ON referenceCondition.ID = playerCondition.RefAchievementConditionID
                              INNER JOIN [dbo].[Achievement_List] AS reference
                                  ON reference.ID = referenceCondition.RefAchievementID
                              WHERE playerCondition.CharID = @CharID
                                AND reference.Service = 1",
                            new { CharID = nCharDBID });
                        foreach (var item in achievementcon)
                        {
                            if (!session.SessionData.CharacterAchievementCondition.ContainsKey(item.RefAchievementConditionID))
                            {
                                session.SessionData.CharacterAchievementCondition.TryAdd(item.RefAchievementConditionID, item);
                            }
                        }

                        var newrev = await connection.QueryAsync<_NewReverseSavedLocations>(
                            "SELECT * FROM [dbo].[Teleport_SavedLocations] WITH (NOLOCK) WHERE CharID = @CharID",
                            new { CharID = nCharDBID });
                        foreach (var item in newrev)
                        {
                            if (!session.SessionData.CharacterNewReverseSavedLocations.ContainsKey(item.LocationID))
                            {
                                session.SessionData.CharacterNewReverseSavedLocations.TryAdd(item.LocationID, item);
                            }
                        }



                        MacroAutoPotion = await connection.QueryAsync<_MacroAutoPotion>(
                            "SELECT * FROM [dbo].[Macro_AutoPotion] WITH (NOLOCK) WHERE CharID = @CharID",
                            new { CharID = nCharDBID });
                        MacroSettings = await connection.QueryAsync<_MacroSetting>(
                            "SELECT * FROM [dbo].[Macro_Settings] WITH (NOLOCK) WHERE CharID = @CharID",
                            new { CharID = nCharDBID });

                    }


                    if (_serverSettings.Achievements)
                    {
                        Packet stAckMsg = new Packet(0x177A);
                        stAckMsg.WriteInt32(session.SessionData.CharacterAchievement.Count());
                        foreach (var stRecord in session.SessionData.CharacterAchievement)
                        {
                            stAckMsg.WriteInt32(stRecord.Value.RefAchievementID);
                            stAckMsg.WriteUInt8(stRecord.Value.State);

                        }
                        await session.SendToClient(stAckMsg);

                        Packet stAchConPck = new Packet(0x177B);
                        stAchConPck.WriteInt32(session.SessionData.CharacterAchievementCondition.Count());
                        foreach (var stRecord in session.SessionData.CharacterAchievementCondition)
                        {
                            stAchConPck.WriteInt32(stRecord.Value.RefAchievementConditionID);
                            stAchConPck.WriteInt32(stRecord.Value.RefAchievementID);
                            stAchConPck.WriteInt64(stRecord.Value.ProgressCount);
                        }
                        await session.SendToClient(stAchConPck);
                    }

                    if (session.SessionData.CharacterNewReverseSavedLocations.Count() > 0)
                    {
                        Packet stAckMsg = new Packet(0x206C);
                        stAckMsg.WriteUInt8(session.SessionData.CharacterNewReverseSavedLocations.Count());
                        foreach (var stRecord in session.SessionData.CharacterNewReverseSavedLocations)
                        {
                            stAckMsg.WriteUInt8(stRecord.Value.LocationID);
                            stAckMsg.WriteInt32(stRecord.Value.RegionID);
                            stAckMsg.WriteInt32(stRecord.Value.WorldID);
                        }
                        await session.SendToClient(stAckMsg);
                    }

                    if (RankManager.m_SilkRank.ContainsKey(nCharDBID))
                    {
                        Packet packetx = new Packet(0x209C);
                        packetx.WriteInt32(RankManager.m_SilkRank[nCharDBID].SilkHistory);
                        packetx.WriteInt32(RankManager.m_SilkRank[nCharDBID].SilkRank);
                        await session.SendToClient(packetx);
                    }


                    await SendAttendanceRewardDefinitionsAsync(session);

                    if (MacroAutoPotion.Count() > 0)
                    {
                        Packet stAckMsg = new Packet(0x204A);
                        stAckMsg.WriteUInt8(MacroAutoPotion.Count());
                        foreach (var data in MacroAutoPotion)
                        {
                            stAckMsg.WriteUInt8(data.Slot);
                            stAckMsg.WriteUInt8(data.Active);
                            stAckMsg.WriteUInt8(data.Value);
                        }
                        await session.SendToClient(stAckMsg);
                    }

                    if (MacroSettings.Count() > 0)
                    {
                        Packet stAckMsg = new Packet(0x204B);
                        foreach (var data in MacroSettings)
                        {
                            stAckMsg.WriteUInt8(data.AutoPotion);
                            stAckMsg.WriteUInt8(data.AutoSkill);
                            stAckMsg.WriteUInt8(data.AutoHunt);
                            stAckMsg.WriteUInt8(data.AutoPickup);
                            stAckMsg.WriteUInt8(data.AutoScroll);
                        }
                        await session.SendToClient(stAckMsg);
                    }

                    await WebViewerManager.SendConfigAsync(session);

                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "GINTERFACE READY");
            }
            return new PacketResult(PacketResultType.Block);
        }

        private async Task<PacketResult> CLIENT_GRANT_NAME_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                string NewGrantName = packet.ReadAscii().Trim('\0');
                bool isValidGrantName =
                    NewGrantName.Length is >= 1 and <= 12 &&
                    NewGrantName.All(ch => char.IsAsciiLetterOrDigit(ch) || ch == '_');
                // Belirli �zel karakterleri tanimla
                char[] specialChars = new[] { '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '+', '[', ']', '{', '}', '\\', '|', ';', ':', '\'', '"', ',', '<', '>', '/', '?', 'ğ', 'ç', 'ş', 'ü', 'ö', 'ı', 'Ğ', 'Ç', 'Ş', 'Ü', 'Ö', 'İ', ' ' };

                // '\0' karakterini temizle
                NewGrantName = NewGrantName.Replace("\0", "");

                // �zel karakter kontrol�
                bool containsSpecialChar =
                    !isValidGrantName ||
                    NewGrantName.Any(ch => specialChars.Contains(ch) && ch != '_');

                // '_' hari� diger �zel karakterler, bosluk veya T�rk�e harfler varsa
                if (containsSpecialChar)
                {
                    return new PacketResult(PacketResultType.Block);
                }
                Packet LiveTitle = new Packet(0x3500);
                LiveTitle.WriteAscii(session.GameServerPacketKey);
                LiveTitle.WriteAscii(NewGrantName);
                await session.SendToServer(LiveTitle);
                return new PacketResult(PacketResultType.Block);
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "CLIENT_GRANT_NAME_REQUEST");
                return new PacketResult(PacketResultType.Block);
            }
        }

        private async Task<SilkInfo?> ChargeSilkAsync(int jid, int price)
        {
            if (price < 0)
                return null;

            var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

            var silk = await connection.QuerySingleOrDefaultAsync<SilkInfo>(
                $@"SELECT TOP (1) silk_own, silk_gift, silk_point
                   FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK)
                   WHERE JID = @JID",
                new { JID = jid },
                transaction);
            if (silk == null || silk.silk_own < price)
            {
                await transaction.RollbackAsync();
                return null;
            }

            var affected = await connection.ExecuteAsync(
                $@"UPDATE {accountDb}..SK_Silk
                   SET silk_own = silk_own - @Price
                   WHERE JID = @JID AND silk_own >= @Price",
                new { Price = price, JID = jid },
                transaction);
            if (affected != 1)
            {
                await transaction.RollbackAsync();
                return null;
            }

            await transaction.CommitAsync();
            silk.silk_own -= price;
            return silk;
        }

        private async Task<SilkInfo?> PurchaseItemMallAsync(
            ISession session,
            int itemId,
            string codeName,
            int itemCount,
            int quantity,
            int totalPrice,
            string source)
        {
            var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

            try
            {
                var silk = await connection.QuerySingleOrDefaultAsync<SilkInfo>(
                    $@"SELECT TOP (1) silk_own, silk_gift, silk_point
                       FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK)
                       WHERE JID = @JID",
                    new { JID = session.SessionData.JID },
                    transaction);

                if (silk == null || silk.silk_own < totalPrice || totalPrice < 0)
                {
                    await transaction.RollbackAsync();
                    return null;
                }

                var affected = await connection.ExecuteAsync(
                    $@"UPDATE {accountDb}..SK_Silk
                       SET silk_own = silk_own - @Price
                       WHERE JID = @JID AND silk_own >= @Price",
                    new { Price = totalPrice, JID = session.SessionData.JID },
                    transaction);
                if (affected != 1)
                {
                    await transaction.RollbackAsync();
                    return null;
                }

                for (var i = 0; i < quantity; i++)
                {
                    await connection.ExecuteAsync(
                        @"EXEC [dbo].[Item_AddChestByCodeName]
                            @CharID = @CharID,
                            @ItemCodeName = @CodeName,
                            @Quantity = @Count,
                            @From = @Source,
                            @Plus = @Plus",
                        new
                        {
                            CharID = session.SessionData.Charid,
                            CodeName = codeName,
                            Count = itemCount,
                            Source = source,
                            Plus = 0
                        },
                        transaction,
                        commandTimeout: 60);
                }

                await transaction.CommitAsync();
                silk.silk_own -= totalPrice;
                return silk;
            }
            catch
            {
                await transaction.RollbackAsync();
                throw;
            }
        }

        private async Task<PacketResult> CLIENT_HWID_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                string response = packet.ReadAscii();
                if (!HwidSecurity.TryValidateResponse(session, response, out string Hwid))
                {
                    await SendSecurityMessageAsync(session, "Security.ClientUpdateRequired");
                    return new PacketResult(PacketResultType.Disconnect);
                }
                else
                {
                    string noticeMessage = BrandHwidNotice(RefManager.GetNoticeMessage("HWID_SUCCES"));
                    session.SessionData.Hwid = Hwid;
                    Packet pck = new Packet(0xA340);
                    pck.WriteUnicode(noticeMessage);
                    await session.SendToClient(pck);

                    return new PacketResult(PacketResultType.Block);
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX, "Agent HWID v2 verification failed for {ClientIp}", session.ClientIp);
                await SendSecurityMessageAsync(session, "Security.HwidVerificationFailed");
                return new PacketResult(PacketResultType.Disconnect);
            }
        }

    }
}

