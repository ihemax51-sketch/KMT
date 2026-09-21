using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database;
using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Servers.PacketHandler
{
    public partial class InventoryPackets
    {
        private AgentServer AgentServer { get; set; }

        public InventoryPackets(AgentServer agentServer, IPacketHandler packetHandler) 
        {
            AgentServer = agentServer;
            packetHandler.RegisterClientHandler(0x704C, AGENT_INVENTORY_ITEM_USE);
            packetHandler.RegisterClientHandler(0x7056, CLIENT_REVERSE_RETURN_REQUEST);
            packetHandler.RegisterClientHandler(0x210A, CUSTOM_ITEM_USAGE);
        }

        private async Task<PacketResult> CLIENT_REVERSE_RETURN_REQUEST(Packet packet, ISession session, object obj)
        {
            var payload = packet.GetBytes();
            if (payload.Length < 1)
                return new PacketResult(packet, PacketResultType.Block);

            var option = payload[0];
            if (option is not (2 or 3))
            {
                Log.Warning(
                    "[RegionAdmission] unsupported Reverse destination option blocked. CharID={CharID}, Option={Option}",
                    session.SessionData.Charid,
                    option);
                return new PacketResult(packet, PacketResultType.Block);
            }

            var destination = await RegionControlService.ResolveReverseDestinationAsync(
                session,
                deadLocation: option == 3);
            if (destination == null)
            {
                Log.Debug(
                    "[RegionAdmission] reverse destination unresolved before request; post-arrival enforcement armed. CharID={CharID}, Option={Option}",
                    session.SessionData.Charid,
                    option);
                RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.Reverse);
                return new PacketResult(packet, PacketResultType.Nothing);
            }

            var admission = await RegionControlService.CheckAdmissionAsync(
                session,
                destination.WorldID,
                destination.RegionID,
                RegionTravelMethod.Reverse);
            if (!admission.Allowed)
                return new PacketResult(packet, PacketResultType.Block);

            RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.Reverse);
            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private async Task<PacketResult> LEGACY_REVERSE_RETURN_REQUEST_UNUSED(Packet packet, ISession session, object obj)
        {
            try
            {
                byte option = packet.ReadUInt8();
                if (option == 2)
                {
                    // Move to last teleport location — check TelRegion
                    try
                    {
                        using var telConn = new SqlConnection(Program.Connectionstring);
                        await telConn.OpenAsync();
                        var telRegion = await telConn.ExecuteScalarAsync<short?>(
                            "SELECT TelRegion FROM [SRO_VT_SHARD].[dbo].[_Char] WITH (NOLOCK) WHERE CharID = @CharID",
                            new { CharID = session.SessionData.Charid });
                        if (telRegion.HasValue)
                        {
                            var telRule = await RegionControlService.GetRuleAsync(telRegion.Value);
                            if (telRule != null && !telRule.Enable_Reverse)
                            {
                                await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Region.ReverseDisabled"));
                                return new PacketResult(packet, PacketResultType.Block);
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "[ReverseScroll] TelRegion check failed CharID={CharID}", session.SessionData.Charid);
                    }
                }
                else if (option == 3)
                {
                    // Move to dead location — check DiedRegion
                    try
                    {
                        using var diedConn = new SqlConnection(Program.Connectionstring);
                        await diedConn.OpenAsync();
                        var diedRegion = await diedConn.ExecuteScalarAsync<short?>(
                            "SELECT DiedRegion FROM [SRO_VT_SHARD].[dbo].[_Char] WITH (NOLOCK) WHERE CharID = @CharID",
                            new { CharID = session.SessionData.Charid });
                        if (diedRegion.HasValue)
                        {
                            var diedRule = await RegionControlService.GetRuleAsync(diedRegion.Value);
                            if (diedRule != null && !diedRule.Enable_Reverse)
                            {
                                await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Region.ReverseDisabled"));
                                return new PacketResult(packet, PacketResultType.Block);
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "[ReverseScroll] DiedRegion check failed CharID={CharID}", session.SessionData.Charid);
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString() + " CLIENT_REVERSE_RETURN_REQUEST");
            }
            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private async Task<PacketResult> CUSTOM_ITEM_USAGE(Packet packet, ISession session, object obj)
        {
            try
            {
                int itemId = packet.ReadInt32();
                int itemSlotId = packet.ReadInt32();
                ushort itemTypeId = packet.ReadUInt16();

                if (itemTypeId == 0xC6ED)
                {
                    if (itemSlotId >= 13 && itemSlotId <= 109)
                    {
                        Packet pck = new Packet(0x3530, false, false);
                        pck.WriteAscii(session.GameServerPacketKey);
                        pck.WriteInt32(itemId);
                        pck.WriteInt32(itemSlotId);
                        pck.WriteUInt16(itemTypeId);
                        await session.SendToServer(pck);
                    }
                }
                else if (itemTypeId == 0xCEED)
                { // is item locker
                    int targetitemslot = packet.ReadInt32();
                    string Code = packet.ReadUnicode();
                    if (itemSlotId >= 13 && itemSlotId <= 109)
                    {
                        int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_LOCK_MAIL_TIME).TotalSeconds);
                        if (gecensaniye < 180)
                        {
                            if (int.TryParse(Code, out int Codeint))
                            {
                                if (Codeint == session.SessionData.LockCode)
                                {
                                    Packet pck = new Packet(0x3531, false, false);
                                    pck.WriteAscii(session.GameServerPacketKey);
                                    pck.WriteInt32(itemId);
                                    pck.WriteInt32(itemSlotId);
                                    pck.WriteUInt16(itemTypeId);
                                    pck.WriteInt32(targetitemslot);
                                    await session.SendToServer(pck);
                                    session.SessionData.LAST_LOCK_MAIL_TIME = new DateTime(2020, 12, 31);
                                    session.SessionData.LockCode = 0;
                                }
                                else
                                {
                                    string noticeMessage = RefManager.GetNoticeMessage("MSG_LOCK_CODE_WRONG");
                                    Packet stMsg = new Packet(0x168A);
                                    stMsg.WriteUInt8(NoticeType.WARNING);
                                    stMsg.WriteUnicode(noticeMessage);
                                    await session.SendToClient(stMsg);


                                    Packet stMsgx = new Packet(0x5015);
                                    stMsgx.WriteUInt8(4);
                                    await session.SendToClient(stMsgx);
                                    return new PacketResult(PacketResultType.Block);
                                }
                            }
                        }
                        else
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("MSG_LOCK_CODE_EXPIRED");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                }
                else if (itemTypeId == 0xD6ED)
                { // is item unlocker
                    int targetitemslot = packet.ReadInt32();

                    string Code = packet.ReadUnicode();
                    if (itemSlotId >= 13 && itemSlotId <= 109)
                    {
                        int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_UNLOCK_MAIL_TIME).TotalSeconds);
                        if (gecensaniye < 180)
                        {
                            if (int.TryParse(Code, out int Codeint))
                            {
                                if (Codeint == session.SessionData.UnLockCode)
                                {
                                    Packet pck = new Packet(0x3532, false, false);
                                    pck.WriteAscii(session.GameServerPacketKey);
                                    pck.WriteInt32(itemId);
                                    pck.WriteInt32(itemSlotId);
                                    pck.WriteUInt16(itemTypeId);
                                    pck.WriteInt32(targetitemslot);
                                    await session.SendToServer(pck);
                                    session.SessionData.LAST_UNLOCK_MAIL_TIME = new DateTime(2020, 12, 31);
                                    session.SessionData.UnLockCode = 0;
                                }
                                else
                                {
                                    string noticeMessage = RefManager.GetNoticeMessage("MSG_UNLOCK_CODE_WRONG");
                                    Packet stMsg = new Packet(0x168A);
                                    stMsg.WriteUInt8(NoticeType.WARNING);
                                    stMsg.WriteUnicode(noticeMessage);
                                    await session.SendToClient(stMsg);


                                    Packet stMsgx = new Packet(0x5015);
                                    stMsgx.WriteUInt8(4);
                                    await session.SendToClient(stMsgx);
                                    return new PacketResult(PacketResultType.Block);
                                }
                            }
                        }
                        else
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("MSG_UNLOCK_CODE_EXPIRED");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "UPDATE_MACRO_SETTING");
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> AGENT_INVENTORY_ITEM_USE(Packet packet, ISession session, object obj)
        {
            try
            {
                byte num = packet.ReadUInt8();
                uint value = packet.ReadUInt16();
                bool isReverseTravelItem = value is 0x19ED or 0x19EC or 0x09ED or 0x09EC;
                if (isReverseTravelItem)
                {
                    var challengeBlock = await PvpChallengeService.BlockTravelIfLockedAsync(session);
                    if (challengeBlock != null)
                        return challengeBlock;
                }

                STypeID stTypeID = new STypeID((ushort)value);
                SItemInfoDbRecord itemInfo = new SItemInfoDbRecord();
                await AgentServer.TryGetItemInfoAsync(itemInfo, session.SessionData.Charid, num);

                if (value is 0x19ED or 0x19EC)
                {
                    RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.Reverse);
                }

                var regionBlock = await GetRegionItemBlockAsync(session, stTypeID, value, itemInfo.szCodeName);
                if (regionBlock != null)
                    return regionBlock;

                // ===== Item Region Restriction Check =====
                if (!isReverseTravelItem)
                {
                    bool isAllowed = await ItemRegionRestrictionService.IsItemAllowedForDestinationAsync(session.SessionData.WorldID, session.SessionData.LatestRegion, itemInfo.nRefItemID);
                    if (!isAllowed)
                    {
                        await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Region.ItemDisabled"));
                        return new PacketResult(packet, PacketResultType.Block);
                    }
                }
                // =========================================


                if (stTypeID.IsCOS)
                {
                    if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                    {
                        if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisablePetSpawn)
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("MSG_PET_SPAWN_IS_DISABLED");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                    else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                    {
                        if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisablePetSpawn)
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("MSG_PET_SPAWN_IS_DISABLED");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                }

                //Log.Warning(value.ToString());
                switch (value)
                {
                    #region itemmm
                    case 0xEED:
                    case 0xEEC:
                        {
                            //if (Dellay(session, session.SessionData.LAST_BOX_TIME, 2, "You're must be wait [{time}] for reuse scroll."))
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //session.SessionData.LAST_BOX_TIME = DateTime.Now;
                        }

                        break;
                    #endregion
                    #region BERSECK_POTION
                    case 0x40EC:
                        {
                            #region Disable berserk pot in fortress
                            //if (MainForm.ANTI_FORTRESS_ZERK_POTION)
                            //{
                            //    if (this.isFortress)
                            //    {
                            //     this.SendNotice(MainForm.FORTRESS_ZERK_POTION_NOTICE);
                            //     continue;
                            //    }
                            //}
                            #endregion

                            #region Disable berserk pot in job
                            //if (MainForm.ANTI_JOB_ZERK_POTION)
                            //{
                            //    if (this.isCharJob)
                            //    {
                            //     this.SendNotice(MainForm.JOB_ZERK_POTION_NOTICE);
                            //     continue;
                            //    }
                            //}
                            #endregion
                        }
                        break;
                    #endregion

                    #region FIX BUG TRADE PET2
                    case 0x08CD:
                    case 4300:
                        {
                            //if (HideNameRegions.ContainsKey(session.SessionData.LatestRegionId))
                            //{
                            //    SendMessageViaText(session, 3, $"LEXA_BLOCK_PET_SPAWN_NOTICE");
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                            //if (session.SessionData.WorldID > 1 && session.SessionData.WorldID < 10)
                            //{
                            //    SendMessageViaText(session, 3, $"LEXA_BLOCK_PET_SPAWN_NOTICE_FORTRESS");
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                            //if (___EventConfig[1].WorldID == session.SessionData.WorldID)
                            //{
                            //    SendMessageViaText(session, 3, $"LEXA_BLOCK_PET_SPAWN_NOTICE");
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                            //if (___EventConfig[2].WorldID == session.SessionData.WorldID)
                            //{
                            //    SendMessageViaText(session, 3, $"LEXA_BLOCK_PET_SPAWN_NOTICE");
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                            //if (___EventConfig[3].WorldID == session.SessionData.WorldID)
                            //{
                            //    SendMessageViaText(session, 3, $"LEXA_BLOCK_PET_SPAWN_NOTICE");
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                            //if (___EventConfig[4].WorldID == session.SessionData.WorldID)
                            //{
                            //    SendMessageViaText(session, 3, $"LEXA_BLOCK_PET_SPAWN_NOTICE");
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                            //foreach(var line in HideNameRegions)
                            //{
                            //    if(line.Value.RegionID == session.SessionData.LatestRegionId)
                            //    return new PacketResult(packet, PacketResultType.Block);
                            //}
                        }
                        break;
                    #endregion

                    #region REVERSE SCROLL
                    case 0x19ED:
                    case 0x19EC:
                        {
                            uint num2 = packet.ReadUInt8();

                            #region REVERSE DISABLE IN JOBBING
                            if (_serverSettings.DisableReverseInJob)
                            {
                                if (session.SessionData.JobType != 4)
                                {
                                    string noticeMessage = RefManager.GetNoticeMessage("MSG_REVERSE_DISABLED_JOB_MODE");
                                    Packet stMsg = new Packet(0x168A);
                                    stMsg.WriteUInt8(NoticeType.WARNING);
                                    stMsg.WriteUnicode(noticeMessage);
                                    await session.SendToClient(stMsg);
                                    return new PacketResult(PacketResultType.Block);
                                }
                            }
                            #endregion

                            #region REVERSE DELAY
                            int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_REVERSE_TIME).TotalSeconds);
                            if (gecensaniye < _serverSettings.ReverseDelay)
                            {
                                int kalanSaniye = _serverSettings.ReverseDelay - gecensaniye;
                                string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_REVERSE_DELAY"), kalanSaniye);
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Block);
                            }
                            session.SessionData.LAST_REVERSE_TIME = DateTime.Now;
                            #endregion


                        }
                        break;
                    #endregion

                    #region TELEPORT TO TOWN SCROLL
                    case 0x09ED:
                    case 0x09EC:
                        {
                            //session.SessionData.CharacterInReturn = true;
                        }
                        break;
                    #endregion

                    #region GLOBAL SCROLL
                    case 0x29ED:
                    case 0x29EC:
                        {
                            //string message = packet.ReadAscii();
                            #region GLOBAL DELAY
                            try
                            {
                                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                                {
                                    if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableChat)
                                    {
                                        string noticeMessage = RefManager.GetNoticeMessage("MSG_EVENT_ARENA_CHAT_DISABLED");
                                        Packet stMsg = new Packet(0x168A);
                                        stMsg.WriteUInt8(NoticeType.WARNING);
                                        stMsg.WriteUnicode(noticeMessage);
                                        await session.SendToClient(stMsg);
                                        return new PacketResult(PacketResultType.Block);
                                    }
                                }
                                int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_GLOBAL_TIME).TotalSeconds);
                                if (gecensaniye < _serverSettings.GlobalDelay)
                                {
                                    int kalanSaniye = _serverSettings.GlobalDelay - gecensaniye;
                                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_GLOBAL_CHAT_DELAY"), kalanSaniye);
                                    Packet stMsg = new Packet(0x168A);
                                    stMsg.WriteUInt8(NoticeType.WARNING);
                                    stMsg.WriteUnicode(noticeMessage);
                                    await session.SendToClient(stMsg);
                                    return new PacketResult(PacketResultType.Block);
                                }

                                if (session.SessionData.CurLevel < _serverSettings.GlobalLevel)
                                {
                                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_GLOBAL_CHAT_LEVEL"), _serverSettings.GlobalLevel);
                                    Packet stMsg = new Packet(0x168A);
                                    stMsg.WriteUInt8(NoticeType.WARNING);
                                    stMsg.WriteUnicode(noticeMessage);
                                    await session.SendToClient(stMsg);
                                    return new PacketResult(PacketResultType.Block);
                                }
                                session.SessionData.LAST_GLOBAL_TIME = DateTime.Now; 
                            }
                            catch { }
                            #endregion
                        }
                        break;
                    #endregion

                    #region RESCURRENT SCROLL
                    case 0x36ED:
                    case 0x36EC:
                        {
                        }
                        break;
                    #endregion

                    #region BUG TT
                    case 0x11ED:
                    case 0x11EC:
                        {
                            try
                            {
                                var botTradeBlock = await BotProtectionService.BlockBotTradeIfDisabledAsync(
                                    session,
                                    "trade transport summon");
                                if (botTradeBlock != null)
                                    return botTradeBlock;

                                int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_SPAWN_TIME).TotalSeconds);
                                if (gecensaniye < _serverSettings.TradePetSpawnDelay)
                                {
                                    int kalanSaniye = _serverSettings.TradePetSpawnDelay - gecensaniye;
                                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_TRADE_PET_DELAY"), kalanSaniye);
                                    Packet stMsg = new Packet(0x168A);
                                    stMsg.WriteUInt8(NoticeType.WARNING);
                                    stMsg.WriteUnicode(noticeMessage);
                                    await session.SendToClient(stMsg);
                                    return new PacketResult(PacketResultType.Block);
                                }
                                session.SessionData.LAST_SPAWN_TIME = DateTime.Now;
                            }
                            catch { }
                        }
                        break;
                    #endregion

                    case 0x5EED:
                    case 0x5EEC:
                        {

                            byte slot = packet.ReadUInt8();
                            //if (slot >= 13 && slot <= 109)
                            //{
                            //    Packet pck = new Packet(0x3517);
                            //    pck.WriteUInt8(num);
                            //    pck.WriteUInt8(slot);
                            //    await session.SendToServer(pck);
                            //}
                            int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_LIVE_ITEM_DELAY).TotalSeconds);
                            if (gecensaniye < _serverSettings.LiveItemDelay)
                            {
                                int kalanSaniye = _serverSettings.LiveItemDelay - gecensaniye;
                                string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_LIVE_ITEM_DELAY"), kalanSaniye);
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Block);
                            }
                            session.SessionData.LAST_LIVE_ITEM_DELAY = DateTime.Now;

                            using (var connection = new SqlConnection(Program.Connectionstring)) // KONTROL
                            {
                                await connection.OpenAsync();
                                await connection.ExecuteAsync(
                                    "EXEC [dbo].[Hook_SelectScroll] @CharName, @CharID, @Num, @Slot",
                                    new
                                    {
                                        CharName = session.SessionData.Charname,
                                        CharID = session.SessionData.Charid,
                                        Num = num,
                                        Slot = slot
                                    });
                            }

                            //return new PacketResult(packet, PacketResultType.Block);
                        }
                        break;

                }

            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "AGENT_INVENTORY_ITEM_USE");
            }
            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private static async Task<PacketResult?> GetRegionItemBlockAsync(
            ISession session,
            STypeID typeId,
            uint rawTypeId,
            string codeName)
        {
            if (typeId.IsReverseReturnScroll)
            {
                return await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Reverse,
                    "Region.ReverseScrollDisabled");
            }

            if (typeId.IsGlobalChatting)
            {
                return await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Global,
                    "Region.GlobalChatDisabled");
            }

            if (rawTypeId == 0x36ED || rawTypeId == 0x36EC)
            {
                return await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_ResurrectionScroll,
                    "Region.ResurrectionScrollDisabled");
            }

            if (typeId.IsScroll && IsFellowScroll(codeName))
            {
                return await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_FellowScroll,
                    "Region.FellowScrollDisabled");
            }

            return null;
        }

        private static bool IsFellowScroll(string codeName)
        {
            return !string.IsNullOrWhiteSpace(codeName) &&
                   codeName.Contains("FELLOW", StringComparison.OrdinalIgnoreCase) &&
                   codeName.Contains("SCROLL", StringComparison.OrdinalIgnoreCase);
        }

    }
}
