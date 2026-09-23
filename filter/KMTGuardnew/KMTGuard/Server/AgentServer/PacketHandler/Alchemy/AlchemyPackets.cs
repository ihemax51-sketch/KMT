using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;
using System.Data;
using Dapper;
using KMTGuard.Features.AutoEvents;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class AlchemyPackets
    {
        private AgentServer AgentServer { get; set; }
        public AlchemyPackets(AgentServer agentServer, IPacketHandler packetHandler)
        {
            AgentServer = agentServer;
            packetHandler.RegisterClientHandler(0x7150, CLIENT_REINFORCE_REQUEST);
            packetHandler.RegisterClientHandler(0x7151, CLIENT_ENCHANT_REQUEST);
            //packetHandler.RegisterClientHandler(0x7157, CLIENT_DISMANTLE_REQUEST);
            //packetHandler.RegisterClientHandler(0x7155, CLIENT_DISJOIN_REQUEST);
            packetHandler.RegisterModuleHandler(0xB150, SERVER_REINFORCE_RESPONSE);

            packetHandler.RegisterClientHandler(0xb300, CLIENT_NEW_ALCHEMY_REQUEST);
            packetHandler.RegisterModuleHandler(0x5017, SERVER_NEW_ALCHEMY_RESULT);
            packetHandler.RegisterModuleHandler(0x5034, SERVER_ALCHEMY_LINK);

           

        }
        private async Task<PacketResult> CLIENT_NEW_ALCHEMY_REQUEST(Packet packet, ISession session, object obj)
        {
            // Keep the opcode registered so modified or outdated clients cannot
            // forward the retired custom request to GameServer.
            if (!_serverSettings.IsNewAlchemyAvailable)
                return new PacketResult(PacketResultType.Block);

            try
            {
                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Alchemy,
                    "Region.AlchemyDisabled");
                if (regionBlock != null)
                    return regionBlock;

                byte plustype = packet.ReadUInt8();
                if (plustype == 0)
                {
                    byte TargetItemSlot = packet.ReadUInt8();
                    byte EncSlot = packet.ReadUInt8();
                    byte ProofSlot = packet.ReadUInt8();

                    SItemInfoDbRecord stItemInfoRecord = new SItemInfoDbRecord();
                    bool bHasItemInfo = await AgentServer.TryGetItemInfoAsync(stItemInfoRecord, session.SessionData.Charid, TargetItemSlot);

                    if (bHasItemInfo)
                    {
                        if (stItemInfoRecord.btOptLevel >= _serverSettings.MaxPlus)
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("ALCHEMY_MAX_ITEM_PLUS_NOADV");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                    // ??? ??? ?????? ?? RefObjCommons
                    bool isNasrun = false;
                    if (RefManager.RefObjCommons.TryGetValue(stItemInfoRecord.nRefItemID, out var refObj))
                    {
                        if (refObj.CodeName128.Contains("NASRUN", StringComparison.OrdinalIgnoreCase))
                            isNasrun = true;
                    }

                    // ??? ??? ????? ??? NASRUN >= +5
                    if (isNasrun && stItemInfoRecord.btOptLevel >= _serverSettings.MaxPlusDevil)
                    {
                        string noticeMessage = $"Maximum Plus +{_serverSettings.MaxPlusDevil} Reached For Devil Plus";
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }



                    Packet xxx = new Packet(0x3534, false, false);
                    xxx.WriteAscii(session.GameServerPacketKey);
                    xxx.WriteUInt8(plustype);
                    xxx.WriteUInt8(TargetItemSlot);
                    xxx.WriteUInt8(EncSlot);
                    xxx.WriteUInt8(ProofSlot);
                    await session.SendToServer(xxx);

                }
                else if (plustype == 1)
                {
                    byte TargetItemSlot = packet.ReadUInt8();
                    byte EncSlot = packet.ReadUInt8();

                    SItemInfoDbRecord stItemInfoRecord = new SItemInfoDbRecord();
                    bool bHasItemInfo = await AgentServer.TryGetItemInfoAsync(stItemInfoRecord, session.SessionData.Charid, TargetItemSlot);

                    if (bHasItemInfo)
                    {
                        if (stItemInfoRecord.btOptLevel >= _serverSettings.MaxPlus)
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("ALCHEMY_MAX_ITEM_PLUS_NOADV");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }

                    Packet xxx = new Packet(0x3534, false, false);
                    xxx.WriteAscii(session.GameServerPacketKey);
                    xxx.WriteUInt8(plustype);
                    xxx.WriteUInt8(TargetItemSlot);
                    xxx.WriteUInt8(EncSlot);
                    await session.SendToServer(xxx);
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, CLIENT_REINFORCE_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> CLIENT_REINFORCE_REQUEST(Packet packet, ISession session, object obj) 
        {
            try
            {
                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Alchemy,
                    "Region.AlchemyDisabled");
                if (regionBlock != null)
                    return regionBlock;

                byte eBaseAction = packet.ReadUInt8();

                if (eBaseAction == 2)
                {
                    byte eSubAction = packet.ReadUInt8();
                    if (eSubAction == 3)
                    {
                        // The third byte is the component count for normal reinforce
                        // requests (2 without powder, 3 with powder).  It is not an
                        // advanced-elixir flag and must not be used to block normal
                        // alchemy in regions that only disable advanced elixirs.
                        byte btComponentCount = packet.ReadUInt8();
                        byte btItemSlot = packet.ReadUInt8();

                        SItemInfoDbRecord stItemInfoRecord = new SItemInfoDbRecord(); 
                        bool bHasItemInfo = await AgentServer.TryGetItemInfoAsync(stItemInfoRecord, session.SessionData.Charid, btItemSlot);

                        if (bHasItemInfo)
                        {
                            if (stItemInfoRecord.btOptLevel >= _serverSettings.MaxPlus)
                            {
                                string noticeMessage = RefManager.GetNoticeMessage("ALCHEMY_MAX_ITEM_PLUS_NOADV");
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING);
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Block);
                            }
                            // ??? ??? ?????? ?? RefObjCommons
                            bool isNasrun = false;
                            if (RefManager.RefObjCommons.TryGetValue(stItemInfoRecord.nRefItemID, out var refObj))
                            {
                                if (refObj.CodeName128.Contains("NASRUN", StringComparison.OrdinalIgnoreCase))
                                    isNasrun = true;
                            }

                            // ??? ??? ????? ??? NASRUN >= +5
                            if (isNasrun && stItemInfoRecord.btOptLevel >= _serverSettings.MaxPlusDevil)
                            {
                                string noticeMessage = $"Maximum Plus +{_serverSettings.MaxPlusDevil} Reached For Devil Plus";
                                Packet stMsg = new Packet(0x168A);
                                stMsg.WriteUInt8(NoticeType.WARNING); // ?? ERROR ?? ?????? ?????
                                stMsg.WriteUnicode(noticeMessage);
                                await session.SendToClient(stMsg);
                                return new PacketResult(PacketResultType.Block);
                            }


                            //using (var connection = new SqlConnection(Program.Connectionstring))
                            //{
                            //    await connection.OpenAsync();
                            //    bool CanTeleport = true;

                            //    using (var command = new SqlCommand("[dbo].[_OnAlchemyRequest_EDIT]", connection))
                            //    {
                            //        command.CommandType = CommandType.StoredProcedure;

                            //        command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                            //        command.Parameters.AddWithValue("@CurrentPlusNotAdv", Convert.ToInt32(stItemInfoRecord.btOptLevel));
                            //        command.Parameters.AddWithValue("@TargetItemID", stItemInfoRecord.nRefItemID);

                            //        var returnValueParam = new SqlParameter("@CanPlus", SqlDbType.Bit)
                            //        {
                            //            Direction = ParameterDirection.Output
                            //        };
                            //        command.Parameters.Add(returnValueParam);

                            //        await command.ExecuteNonQueryAsync();

                            //        CanTeleport = (bool)returnValueParam.Value;
                            //    }

                            //    if (CanTeleport)
                            //    {
                            //        return new PacketResult(packet, PacketResultType.Nothing);
                            //    }
                            //    else
                            //    {
                            //        return new PacketResult(packet, PacketResultType.Block);
                            //    }
                            //}


                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, CLIENT_REINFORCE_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult();
        }

        private async Task<PacketResult> CLIENT_ENCHANT_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Alchemy,
                    "Region.AlchemyDisabled");
                if (regionBlock != null)
                    return regionBlock;
            }
            catch (Exception ex)
            {
                Log.Error($"{ex.Message}, CLIENT_ENCHANT_REQUEST", ConsoleColor.Red);
            }

            return new PacketResult();
        }
        private async Task<PacketResult> SERVER_REINFORCE_RESPONSE(Packet packet, ISession session, object obj) 
        {
            try
            {
                //1
                byte btResult = packet.ReadUInt8();
                if (btResult != 1)
                    return new PacketResult();

                //2
                byte btResult2 = packet.ReadUInt8();
                if (btResult2 != 2)
                    return new PacketResult();

                //3
                byte btResult3 = packet.ReadUInt8();
                if (btResult3 == 1)
                {
                    //4
                    byte btSlotIndex = packet.ReadUInt8();
                    uint nItemGameID = packet.ReadUInt32();
                    uint nRefItemID = packet.ReadUInt32();
                    byte btNewOptLevel = packet.ReadUInt8();
                    UInt64 qwVariance = packet.ReadUInt64();
                    uint nDurability = packet.ReadUInt32();
                    byte unk3 = packet.ReadUInt8();
                    byte unk4 = packet.ReadUInt8();
                    byte unk5 = packet.ReadUInt8();
                    byte unk6 = packet.ReadUInt8();
                    byte AdvStatus = packet.ReadUInt8();



                    if (AdvStatus == 1)
                    {
                        byte unk7 = packet.ReadUInt8();
                        uint AdvItemID = packet.ReadUInt32();
                        byte AdvPlus = packet.ReadUInt8();


                        if (_serverSettings.AlchemyItemLinkMinLevel != 0 && btNewOptLevel + AdvPlus >= _serverSettings.AlchemyItemLinkMinLevel)
                        {
                            var job = new DelayedJobItem(
                                 500, session, null,
                                 async (s, p) =>
                                 {
                                     Packet pck = new Packet(0x3533);
                                     pck.WriteAscii(session.GameServerPacketKey);
                                     pck.WriteUInt8(btSlotIndex);
                                     pck.WriteUInt8(AdvPlus);
                                     await session.SendToServer(pck);
                                 });

                            RefManager.g_DelayedJobMgr.CreateJob(job);
                        }

                        await RecordAlchemyProgressAsync(session, nItemGameID, btNewOptLevel + AdvPlus);
                        QueueAlchemySuccess(session.SessionData.Charid, session.SessionData.Charname,
                            checked((int)nRefItemID), btNewOptLevel, AdvPlus, btSlotIndex);
                    }
                    else
                    {


                        if (_serverSettings.AlchemyItemLinkMinLevel != 0 && btNewOptLevel >= _serverSettings.AlchemyItemLinkMinLevel)
                        {
                            var job = new DelayedJobItem(
                                 500, session, null,
                                 async (s, p) =>
                                 {
                                     Packet pck = new Packet(0x3533);
                                     pck.WriteAscii(session.GameServerPacketKey);
                                     pck.WriteUInt8(btSlotIndex);
                                     pck.WriteUInt8(0);
                                     await session.SendToServer(pck);
                                 });

                            RefManager.g_DelayedJobMgr.CreateJob(job);
                        }

                        await RecordAlchemyProgressAsync(session, nItemGameID, btNewOptLevel);
                        QueueAlchemySuccess(session.SessionData.Charid, session.SessionData.Charname,
                            checked((int)nRefItemID), btNewOptLevel, 0, btSlotIndex);
                    }
                }
               
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, CLIENT_REINFORCE_REQUEST", ConsoleColor.Red);
            }
            return new PacketResult();
        }
        private async Task<PacketResult> SERVER_NEW_ALCHEMY_RESULT(Packet packet, ISession session, object obj)
        {
            // Suppress stale custom results during mixed-version restarts and do
            // not run success hooks for the retired pipeline.
            if (!_serverSettings.IsNewAlchemyAvailable)
                return new PacketResult(PacketResultType.Block);

            try
            {
                byte FuseType = packet.ReadUInt8();
                if (FuseType == 0)
                {
                    byte result = packet.ReadUInt8();
                    if (result != 2)
                    {
                        byte ItemSlot = packet.ReadUInt8();
                        byte EnhancerSlot = packet.ReadUInt8();
                        byte ProofSlot = packet.ReadUInt8();
                        byte btNewOptLevel = packet.ReadUInt8();
                        int nRefItemID = packet.ReadInt32();
                        byte AdvPlus = packet.ReadUInt8();

                        if (result == 0)
                        {
                            if (ItemSlot != 0)
                            {
                                if (_serverSettings.AlchemyItemLinkMinLevel != 0 && btNewOptLevel + AdvPlus >= _serverSettings.AlchemyItemLinkMinLevel)
                                {
                                    var job = new DelayedJobItem(
                                        500, session, null,
                                        async (s, p) =>
                                        {
                                            Packet pck = new Packet(0x3533);
                                            pck.WriteAscii(session.GameServerPacketKey);
                                            pck.WriteUInt8(ItemSlot);
                                            pck.WriteUInt8(AdvPlus);
                                            await session.SendToServer(pck);
                                        });

                                    RefManager.g_DelayedJobMgr.CreateJob(job);
                                }
                                await RecordAlchemyProgressAsync(session, nRefItemID, btNewOptLevel + AdvPlus);
                                QueueAlchemySuccess(session.SessionData.Charid, session.SessionData.Charname,
                                    nRefItemID, btNewOptLevel, AdvPlus, ItemSlot);
                            }
                        }
                    }
                }
                else if (FuseType == 1)
                {
                    byte result = packet.ReadUInt8();
                    if (result == 0)
                    {
                        byte ItemSlot = packet.ReadUInt8();
                        byte EnhancerSlot = packet.ReadUInt8();
                        byte btNewOptLevel = packet.ReadUInt8();
                        int nRefItemID = packet.ReadInt32();
                        byte AdvPlus = packet.ReadUInt8();

                        if (ItemSlot != 0)
                        {
                            if (_serverSettings.AlchemyItemLinkMinLevel != 0 && btNewOptLevel + AdvPlus >= _serverSettings.AlchemyItemLinkMinLevel)
                            {
                                var job = new DelayedJobItem(
                                    500, session, null,
                                    async (s, p) =>
                                    {
                                        Packet pck = new Packet(0x3533);
                                        pck.WriteAscii(session.GameServerPacketKey);
                                        pck.WriteUInt8(ItemSlot);
                                        pck.WriteUInt8(AdvPlus);
                                        await session.SendToServer(pck);
                                    });

                                RefManager.g_DelayedJobMgr.CreateJob(job);
                            }

                            await RecordAlchemyProgressAsync(session, nRefItemID, btNewOptLevel + AdvPlus);
                            QueueAlchemySuccess(session.SessionData.Charid, session.SessionData.Charname,
                                nRefItemID, btNewOptLevel, AdvPlus, ItemSlot);
                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message}, SERVER_NEW_ALCHEMY_RESULT", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Nothing);
            }
            return new PacketResult(packet, PacketResultType.Nothing);
        }
        private async Task<PacketResult> SERVER_ALCHEMY_LINK(Packet packet, ISession session, object obj)
        {
            try
            {
                int Len = packet.ReadInt32();
                List<byte> Bytes = new();
                for (int i = 0; i < Len; i++)
                {
                    byte p = packet.ReadUInt8();
                    Bytes.Add(p);
                }

                int ItemID = packet.ReadInt32();
                byte ItemPlus = packet.ReadUInt8();
                byte AdvLevel = packet.ReadUInt8();


                Packet itemlink = new Packet(0x209F);
                itemlink.WriteUnicode(session.SessionData.Charname);
                itemlink.WriteInt32(ItemID);
                itemlink.WriteUInt8(ItemPlus);
                itemlink.WriteUInt8(AdvLevel);
                itemlink.WriteInt32(Len);
                foreach (var data in Bytes)
                {
                    itemlink.WriteUInt8(data);
                }



                itemlink.WriteInt32(ItemID);
                await ServerManager.BroadcastPacket(itemlink);
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_ALCHEMY_LINK", ConsoleColor.Red);
                return new PacketResult(packet, PacketResultType.Block);
            }
            return new PacketResult(packet, PacketResultType.Block);
        }

        private static async Task RecordAlchemyProgressAsync(ISession session, long itemReference, int plus)
        {
            await AutoEventService.HandleAlchemySuccessAsync(session, plus);
        }

        private static void QueueAlchemySuccess(
            int characterId,
            string characterName,
            int referenceItemId,
            byte optionLevel,
            byte advancedPlus,
            byte slot)
        {
            DatabaseJobQueue.TryQueueBackground(async cancellationToken =>
            {
                await using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync(cancellationToken);
                await connection.ExecuteAsync(new CommandDefinition(
                    "EXEC [dbo].[Hook_AlchemySuccess] @CharID, @CharName, @RefItemID, @OptLevel, @AdvPlus, @Slot",
                    new
                    {
                        CharID = characterId,
                        CharName = characterName,
                        RefItemID = referenceItemId,
                        OptLevel = optionLevel,
                        AdvPlus = advancedPlus,
                        Slot = slot
                    },
                    commandTimeout: SqlExecutionPolicy.BackgroundSeconds,
                    cancellationToken: cancellationToken));
            }, operation: "alchemy success bookkeeping");
        }
    }
}
