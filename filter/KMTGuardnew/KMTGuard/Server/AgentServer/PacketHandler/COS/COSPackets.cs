using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;
using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class COSPackets
    {
        private AgentServer AgentServer { get; set; }

        private static bool IsFellowPetReference(uint refObjId)
        {
            if (refObjId > int.MaxValue)
                return false;

            int referenceId = (int)refObjId;
            if (!RefManager.m_RefFellowPetRefObjID.Contains(referenceId))
                return false;

            return RefManager.RefObjCommons.TryGetValue(referenceId, out var reference) &&
                   reference.TypeID1 == 1 &&
                   reference.TypeID2 == 2 &&
                   reference.TypeID3 == 3 &&
                   reference.TypeID4 == 3;
        }

        private static void QueueFellowBuffRemoval(int charId)
        {
            if (charId <= 0)
                return;

            if (!DatabaseJobQueue.TryQueueBackground(
                    async cancellationToken =>
                    {
                        await using var connection = new SqlConnection(Program.Connectionstring);
                        await connection.OpenAsync(cancellationToken);

                        await using var command = new SqlCommand(
                            "dbo.FellowBuff_RemoveAll",
                            connection)
                        {
                            CommandType = CommandType.StoredProcedure
                        };
                        command.Parameters.Add("@CharID", SqlDbType.Int).Value = charId;

                        await command.ExecuteNonQueryAsync(cancellationToken);
                    },
                    operation: "remove Fellow Buffs after Fellow unsummon"))
            {
                Log.Warning(
                    "Could not queue Fellow Buff removal after Fellow unsummon. CharID={CharID}",
                    charId);
            }
        }

        public COSPackets(AgentServer agentServer, IPacketHandler packetHandler)
        {
            AgentServer = agentServer;
            packetHandler.RegisterModuleHandler(0x30C8, HandleCosDataAck);
            packetHandler.RegisterClientHandler(0x70C6, CLIENT_AGENT_COS_UPDATE); // t0p
            packetHandler.RegisterClientHandler(0x70CB, CLIENT_AGENT_COS_UPDATE_RIDESTATE);// t0p
            packetHandler.RegisterClientHandler(0x7116, CLIENT_AGENT_COS_UPDATE_PETS);// t0p

            packetHandler.RegisterModuleHandler(0x30C9, SERVER_AGENT_COS_UPDATE);
            packetHandler.RegisterModuleHandler(0xB0CB, SERVER_AGENT_COS_UPDATE_RIDESTATE);
        }
        private Task<PacketResult> SERVER_AGENT_COS_UPDATE_RIDESTATE(Packet packet, ISession session, object obj)
        {
            if (packet.ReadUInt8() != 0x01)
                return Task.FromResult(new PacketResult());

            var ownerUniqueId = packet.ReadUInt32();
            var isMounted = packet.ReadBool();
            var cosUniqueId = packet.ReadUInt32();

            if (ownerUniqueId != session.SessionData.UniqueCharId)
                return Task.FromResult(new PacketResult());

            session.SessionData.OnTransport = isMounted;
            session.SessionData.TransportUniqueId = isMounted ? cosUniqueId : 0;

            return Task.FromResult(new PacketResult());
        }
        private Task<PacketResult> SERVER_AGENT_COS_UPDATE(Packet packet, ISession session, object obj) // UNK
        {
            try
            {
                uint nGameID = packet.ReadUInt32();

                byte type = packet.ReadUInt8();

                if (session.SessionData.FellowPetUniqueID == nGameID)
                {
                    if (type == 1)
                    {
                        session.SessionData.FellowPetUniqueID = 0;
                        session.SessionData.FellowPetID64 = 0;
                        session.SessionData.FellowItemID = 0;

                    }

                }
                if (type == 1 && session.SessionData.FellowRuntimeObjectID == nGameID)
                {
                    int charId = session.SessionData.Charid;
                    session.SessionData.ClearFellowRuntimeState();
                    QueueFellowBuffRemoval(charId);
                }

                if(type == 1)
                {
                    if (session.SessionData.CharPetList.Contains(nGameID))
                    {
                        session.SessionData.CharPetList.Remove(nGameID);
                        //printf("removed type 1 %d \n", uniqueId);
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, SERVER_AGENT_COS_UPDATE", ConsoleColor.Red);
            }
            return Task.FromResult(new PacketResult());
        }
        private async Task<PacketResult> HandleCosDataAck(Packet packet, ISession session, object obj) // UNK
        {
            // The GameServer packet is authoritative. Forward an independent copy
            // before optional fellow persistence so SQL latency cannot delay 0x30C8.
            await session.SendToClient(new Packet(packet));
            try
            {
                uint nGameID = packet.ReadUInt32();
                uint nRefObjID = packet.ReadUInt32();

                // t0p
                if (!session.SessionData.PetDataDictionary.ContainsKey(nGameID))
                {
                    var petInfo = new PetInfo
                    {
                        UniqueID = nGameID,
                        RefObjID = nRefObjID,
                    };

                    session.SessionData.PetDataDictionary[nGameID] = petInfo;
                }
                if (session.SessionData.PetDataDictionary.ContainsKey(nGameID))
                {
                    var petInfo = session.SessionData.PetDataDictionary[nGameID];
                }

                if (!session.SessionData.CharPetList.Contains(nGameID))
                {
                    session.SessionData.CharPetList.Add(nGameID);
                }

                if (IsFellowPetReference(nRefObjID))
                {

                    //In vsro 188, fellows are growth pets, so parsing that.
                    uint nCurHealth = packet.ReadUInt32();
                    uint nMaxHealth = packet.ReadUInt32();
                    UInt64 ulExpOffset = packet.ReadUInt64();
                    byte btUnk1 = packet.ReadUInt8();
                    ushort shHungerPoints = packet.ReadUInt16();
                    uint bIsOffensive = packet.ReadUInt32();
                    string strName = packet.ReadAscii();
                    byte btUnk2 = packet.ReadUInt8();
                    uint nOwnerGID = packet.ReadUInt32();
                    byte ItemSlot = packet.ReadUInt8();



                    if (nOwnerGID != session.SessionData.UniqueCharId)
                        return new PacketResult(PacketResultType.Block);


                    session.SessionData.IsFellowSummoned = true;
                    session.SessionData.FellowRuntimeObjectID = nGameID;
                    session.SessionData.FellowRefObjID = nRefObjID;
                    session.SessionData.FellowPetUniqueID = nGameID;
                    long itemID64 = 0;
                    int refItemID = 0;
                    using (var connection = new SqlConnection(Program.Connectionstring))
                    {
                        using (var command = new SqlCommand("[dbo].[Player_GetFellowPet]", connection) { CommandType = CommandType.StoredProcedure })
                        {
                            command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                            command.Parameters.AddWithValue("@Slot", ItemSlot);

                            var itemID64Param = new SqlParameter("@ItemID64", SqlDbType.BigInt) { Direction = ParameterDirection.Output };
                            var refItemIDParam = new SqlParameter("@RefItemID", SqlDbType.Int) { Direction = ParameterDirection.Output };
                            command.Parameters.Add(itemID64Param);
                            command.Parameters.Add(refItemIDParam);

                            await connection.OpenAsync();
                            await command.ExecuteNonQueryAsync();

                            itemID64 = (long)itemID64Param.Value;
                            refItemID = (int)refItemIDParam.Value;
                        }

                    }

                    if (itemID64 != 0 && refItemID != 0)
                    {
                        session.SessionData.FellowPetID64 = itemID64;
                        session.SessionData.FellowItemID = refItemID;


                        foreach (var data in RefManager.m_RefFellowData)
                        {
                            if (data.Value.ItemID == refItemID)
                            {
                                /// FELLOW DATA MEVCUT EVET ŞİMDİ FELLOW'UN BILGILERINI GÖNDERELİM.
                                /// int64 ile fellowun bilgilerini bağdaştıralım.. Fellowu karakterden bağımsız kendine özgü yapacağız. önce tablodan kontrol et verileri
                                /// veriler yoksa default ekle ve gönder var ise mevcut verileri cliente gönderelim. hatırlamak için NOT !!!! tabi bunu INT64 ile sorgulayacağız.
                                using (var connection = new SqlConnection(Program.Connectionstring))
                                {
                                    await connection.OpenAsync();

                                    var Fellowdata = await connection.QueryAsync<_FellowSkillData>(
                                        "SELECT * FROM [dbo].[Fellow_Skills] WITH (NOLOCK) WHERE ID64 = @ID64",
                                        new { ID64 = itemID64 });

                                    if (Fellowdata.Count() > 0)
                                    {
                                        Packet stAckMsg = new Packet(0x206B);

                                        stAckMsg.WriteUInt8(Fellowdata.Count());

                                        foreach (var Data in Fellowdata)
                                        {
                                            stAckMsg.WriteInt64(Data.ID64);

                                            if (data.Value.SkillType_1 == 0) /// pet skill
                                            {
                                                stAckMsg.WriteUInt8(1);
                                            }
                                            else
                                            {
                                                stAckMsg.WriteUInt8(Data.Enable_Skill_1);
                                            }

                                            if (data.Value.SkillType_2 == 0) /// pet skill
                                            {
                                                stAckMsg.WriteUInt8(1);
                                            }
                                            else
                                            {
                                                stAckMsg.WriteUInt8(Data.Enable_Skill_2);
                                            }

                                            if (data.Value.SkillType_3 == 0) /// pet skill
                                            {
                                                stAckMsg.WriteUInt8(1);
                                            }
                                            else
                                            {
                                                stAckMsg.WriteUInt8(Data.Enable_Skill_3);
                                            }

                                            if (data.Value.SkillType_4 == 0) /// pet skill
                                            {
                                                stAckMsg.WriteUInt8(1);
                                            }
                                            else
                                            {
                                                stAckMsg.WriteUInt8(Data.Enable_Skill_4);
                                            }

                                            if (data.Value.SkillType_5 == 0) /// pet skill
                                            {
                                                stAckMsg.WriteUInt8(1);
                                            }
                                            else
                                            {
                                                stAckMsg.WriteUInt8(Data.Enable_Skill_5);
                                            }

                                        }
                                        await session.SendToClient(stAckMsg);
                                    }
                                    else
                                    {
                                        var str = new _FellowSkillData();
                                        str.ID64 = itemID64;
                                        if (data.Value.SkillType_1 == 0 && data.Value.SkillID_1 != 0)
                                        {
                                            str.Enable_Skill_1 = 0;
                                        }
                                        if (data.Value.SkillType_2 == 0 && data.Value.SkillID_2 != 0)
                                        {
                                            str.Enable_Skill_2 = 0;
                                        }
                                        if (data.Value.SkillType_3 == 0 && data.Value.SkillID_3 != 0)
                                        {
                                            str.Enable_Skill_3 = 0;
                                        }
                                        if (data.Value.SkillType_4 == 0 && data.Value.SkillID_4 != 0)
                                        {
                                            str.Enable_Skill_4 = 0;
                                        }
                                        if (data.Value.SkillType_5 == 0 && data.Value.SkillID_5 != 0)
                                        {
                                            str.Enable_Skill_5 = 0;
                                        }

                                        Packet stAckMsg = new Packet(0x206B);

                                        stAckMsg.WriteUInt8(1);

                                        stAckMsg.WriteInt64(str.ID64);
                                        stAckMsg.WriteUInt8(str.Enable_Skill_1);
                                        stAckMsg.WriteUInt8(str.Enable_Skill_2);
                                        stAckMsg.WriteUInt8(str.Enable_Skill_3);
                                        stAckMsg.WriteUInt8(str.Enable_Skill_4);
                                        stAckMsg.WriteUInt8(str.Enable_Skill_5);
                                        await session.SendToClient(stAckMsg);

                                        DatabaseJobQueue.TryQueueBackground(async cancellationToken =>
                                        {
                                                await using var newConnection = new SqlConnection(Program.Connectionstring);
                                                await newConnection.OpenAsync(cancellationToken);
                                                await using var sqlCommand = new SqlCommand(
                                                    "EXEC [dbo].[Player_SaveFellow] @ID64, @EnableSkill1, @EnableSkill2, @EnableSkill3, @EnableSkill4, @EnableSkill5",
                                                    newConnection)
                                                {
                                                    CommandTimeout = SqlExecutionPolicy.BackgroundSeconds
                                                };
                                                sqlCommand.Parameters.AddWithValue("@ID64", str.ID64);
                                                sqlCommand.Parameters.AddWithValue("@EnableSkill1", str.Enable_Skill_1);
                                                sqlCommand.Parameters.AddWithValue("@EnableSkill2", str.Enable_Skill_2);
                                                sqlCommand.Parameters.AddWithValue("@EnableSkill3", str.Enable_Skill_3);
                                                sqlCommand.Parameters.AddWithValue("@EnableSkill4", str.Enable_Skill_4);
                                                sqlCommand.Parameters.AddWithValue("@EnableSkill5", str.Enable_Skill_5);

                                                await sqlCommand.ExecuteNonQueryAsync(cancellationToken);
                                        }, operation: "fellow skill persistence bookkeeping");
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Error($"{EX.Message.ToString()}, HandleCosDataAck", ConsoleColor.Red);
            }
            return new PacketResult(PacketResultType.Block);
        }

        private Task<PacketResult> CLIENT_AGENT_COS_UPDATE(Packet packet, ISession session, object obj)
        {
            try
            {
                uint petUniqueId = packet.ReadUInt32();

                PetInfo? petInfo = null;
                var pets = session?.SessionData?.PetDataDictionary;

                if (pets != null)
                {
                    if (!pets.TryGetValue(petUniqueId, out petInfo))
                    {
                        foreach (var kv in pets)
                        {
                            var v = kv.Value;
                            if (v != null && v.UniqueID == petUniqueId)
                            {
                                petInfo = v;
                                break;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                try { Log.Error($"[0x70C6] Error: {ex.Message}"); } catch { }
            }

            return Task.FromResult(new PacketResult());
        }

        private Task<PacketResult> CLIENT_AGENT_COS_UPDATE_RIDESTATE(Packet packet, ISession session, object obj)
        {
            try
            {
                byte state = packet.ReadUInt8();
                uint petUniqueId = packet.ReadUInt32();

                PetInfo? petInfo = null;
                var pets = session?.SessionData?.PetDataDictionary;

                if (pets != null)
                {
                    if (!pets.TryGetValue(petUniqueId, out petInfo))
                    {
                        foreach (var kv in pets)
                        {
                            var v = kv.Value;
                            if (v != null && v.UniqueID == petUniqueId)
                            {
                                petInfo = v;
                                break;
                            }
                        }
                    }
                }

            }
            catch (Exception ex)
            {
                try { Log.Error($"[0x70CB] Error: {ex.Message}"); } catch { }
            }

            return Task.FromResult(new PacketResult());
        }

        private Task<PacketResult> CLIENT_AGENT_COS_UPDATE_PETS(Packet packet, ISession session, object obj)
        {
            try
            {
                uint petUniqueId = packet.ReadUInt32();

                PetInfo? petInfo = null;
                var pets = session?.SessionData?.PetDataDictionary;

                if (pets != null)
                {
                    if (!pets.TryGetValue(petUniqueId, out petInfo))
                    {
                        foreach (var kv in pets)
                        {
                            var v = kv.Value;
                            if (v != null && v.UniqueID == petUniqueId)
                            {
                                petInfo = v;
                                break;
                            }
                        }
                    }
                }

            }
            catch (Exception ex)
            {
                try { Log.Error($"[0x7116] Error: {ex.Message}"); } catch { }
            }

            return Task.FromResult(new PacketResult());
        }

    }
}
