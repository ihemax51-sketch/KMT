using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class NewReverse
    {
        public NewReverse(AgentServer agentServer, IPacketHandler packetHandler) 
        {
            packetHandler.RegisterClientHandler(0x201F, NEW_REVERSE_TELEPORT_PT_MEMBER);
            packetHandler.RegisterClientHandler(0x181C, NEW_REVERSE_TELEPORT_SAVE_LOCATION);
            packetHandler.RegisterClientHandler(0x200C, NEW_REVERSE_SAVE_LOCATION);
            packetHandler.RegisterClientHandler(0x200D, NEW_REVERSE_REMOVE_LOCATION);
        }
        private async Task<PacketResult> NEW_REVERSE_TELEPORT_PT_MEMBER(Packet packet, ISession session, object obj)
        {
            try
            {
                var challengeBlock = await PvpChallengeService.BlockTravelIfLockedAsync(session);
                if (challengeBlock != null)
                    return challengeBlock;

                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Reverse,
                    "Region.ReverseDisabled");
                if (regionBlock != null)
                    return regionBlock;

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
                if (session.SessionData.WorldID == 99)
                {
                    await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Region.ItemUnavailableHere"));
                    return new PacketResult(PacketResultType.Block);
                }

                // RESTART_DELAY s�resini kontrol et

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

                int reverse_Slot = packet.ReadInt32();
                short Region = packet.ReadInt16();
                int Current_x = packet.ReadInt32();
                int Current_y = packet.ReadInt32();
                int Current_z = packet.ReadInt32();
                string Charname = packet.ReadAscii();

                var pSession = ServerManager.AgentSessions.FindByCharName(Charname);
                if (pSession == null)
                {
                    await RegionControlService.SendNoticeAsync(session, PlayerLanguage.Get("Region.AdmissionDestinationUnavailable"));
                    return new PacketResult(PacketResultType.Block);
                }

                var targetRegion = RegionControlService.NormalizeRegionId(pSession.SessionData.LatestRegion);
                if (pSession.SessionData.WorldID != 1 ||
                    !RegionControlService.IsValidRegionId(targetRegion) ||
                    RegionControlService.NormalizeRegionId(Region) != targetRegion)
                {
                    await RegionControlService.SendNoticeAsync(session, PlayerLanguage.Get("Region.AdmissionDestinationUnavailable"));
                    return new PacketResult(PacketResultType.Block);
                }

                var admission = await RegionControlService.CheckAdmissionAsync(
                    session,
                    pSession.SessionData.WorldID,
                    targetRegion,
                    RegionTravelMethod.Reverse);
                if (!admission.Allowed)
                    return new PacketResult(PacketResultType.Block);
                RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.Reverse);

                Packet packetx = new Packet(0x3504);
                packetx.WriteAscii(session.GameServerPacketKey);
                packetx.WriteInt32(reverse_Slot);
                packetx.WriteUInt8(1);
                packetx.WriteUInt16(Region);
                packetx.WriteFloat(Current_x);
                packetx.WriteFloat(Current_y);
                packetx.WriteFloat(Current_z);
                packetx.WriteUInt8(1); // Explicit admission approval; validated again by GameServer.
                await session.SendToServer(packetx);
                session.SessionData.LAST_REVERSE_TIME = DateTime.Now;
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "NEW_REVERSE_TELEPORT_PT_MEMBER");
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> NEW_REVERSE_TELEPORT_SAVE_LOCATION(Packet packet, ISession session, object obj)
        {
            try
            {
                var challengeBlock = await PvpChallengeService.BlockTravelIfLockedAsync(session);
                if (challengeBlock != null)
                    return challengeBlock;

                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Reverse,
                    "Region.ReverseDisabled");
                if (regionBlock != null)
                    return regionBlock;

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
                // RESTART_DELAY süresini kontrol et

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

                byte telslot = packet.ReadUInt8();
                byte reverse_Slot = packet.ReadUInt8();

                // Eger gecikme dolmussa, LAST_RESTART_TIME güncellenir
                if (session.SessionData.CharacterNewReverseSavedLocations.ContainsKey(telslot))
                {
                    var savedLocation = session.SessionData.CharacterNewReverseSavedLocations[telslot];
                    var admission = await RegionControlService.CheckAdmissionAsync(
                        session,
                        savedLocation.WorldID,
                        RegionControlService.NormalizeRegionId(savedLocation.RegionID),
                        RegionTravelMethod.Reverse);
                    if (!admission.Allowed)
                        return new PacketResult(PacketResultType.Block);
                    RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.Reverse);

                    Packet packetx = new Packet(0x3504);
                    packetx.WriteAscii(session.GameServerPacketKey);
                    packetx.WriteInt32(reverse_Slot);
                    packetx.WriteUInt8(session.SessionData.CharacterNewReverseSavedLocations[telslot].WorldID);
                    packetx.WriteUInt16(session.SessionData.CharacterNewReverseSavedLocations[telslot].RegionID);
                    packetx.WriteFloat(session.SessionData.CharacterNewReverseSavedLocations[telslot].PosX);
                    packetx.WriteFloat(session.SessionData.CharacterNewReverseSavedLocations[telslot].PosY);
                    packetx.WriteFloat(session.SessionData.CharacterNewReverseSavedLocations[telslot].PosZ);
                    packetx.WriteUInt8(1); // Explicit admission approval; validated again by GameServer.
                    await session.SendToServer(packetx);

                    session.SessionData.LAST_REVERSE_TIME = DateTime.Now;
                    return new PacketResult(PacketResultType.Block);
                }

            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "NEW_REVERSE_TELEPORT_SAVE_LOCATION");
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> NEW_REVERSE_REMOVE_LOCATION(Packet packet, ISession session, PacketData data)
        {
            try
            {
                byte locationID = packet.ReadUInt8();

                if (session.SessionData.CharacterNewReverseSavedLocations.ContainsKey(locationID))
                {
                    session.SessionData.CharacterNewReverseSavedLocations.Remove(locationID);
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                using (var command = new SqlCommand(
                                    "DELETE FROM [dbo].[Teleport_SavedLocations] WHERE CharID = @CharID AND LocationID = @LocationID",
                                    connection))
                                {
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@LocationID", locationID);
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
                Log.Warning(ex.Message.ToString() + "NEW_REVERSE_REMOVE_LOCATION");
            }

            return new PacketResult(packet, PacketResultType.Block);
        }
        private async Task<PacketResult> NEW_REVERSE_SAVE_LOCATION(Packet packet, ISession session, object obj)
        {
            try
            {
                byte locationID = packet.ReadUInt8();
                ushort Region = packet.ReadUInt16();
                int Current_x = packet.ReadInt32();
                int Current_y = packet.ReadInt32();
                int Current_z = packet.ReadInt32();
                int WorldID = packet.ReadInt32();
                if (WorldID == 1 &&
                    WorldID == session.SessionData.WorldID &&
                    Region == session.SessionData.LatestRegion)
                {
                    {
                        var str = new _NewReverseSavedLocations();

                        str.CharID = session.SessionData.Charid;
                        str.LocationID = locationID;
                        str.RegionID = Region;
                        str.PosX = Current_x;
                        str.PosY = Current_y;
                        str.PosZ = Current_z;
                        str.WorldID = WorldID;

                        var persisted = false;
                        await DatabaseJobQueue.RunAsync(() =>
                        {
                            try
                            {
                                using (var connection = new SqlConnection(Program.Connectionstring))
                                {
                                    connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                    using (var command = new SqlCommand(
                                        @"UPDATE [dbo].[Teleport_SavedLocations]
                                          SET RegionID = @RegionID,
                                              PosX = @PosX,
                                              PosY = @PosY,
                                              PosZ = @PosZ,
                                              WorldID = @WorldID
                                          WHERE CharID = @CharID AND LocationID = @LocationID;

                                          IF @@ROWCOUNT = 0
                                          BEGIN
                                              INSERT INTO [dbo].[Teleport_SavedLocations]
                                                  (CharID, LocationID, RegionID, PosX, PosY, PosZ, WorldID)
                                              VALUES
                                                  (@CharID, @LocationID, @RegionID, @PosX, @PosY, @PosZ, @WorldID);
                                          END",
                                        connection))
                                    {
                                        command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                        command.Parameters.AddWithValue("@LocationID", locationID);
                                        command.Parameters.AddWithValue("@RegionID", (int)Region);
                                        command.Parameters.AddWithValue("@PosX", Current_x);
                                        command.Parameters.AddWithValue("@PosY", Current_y);
                                        command.Parameters.AddWithValue("@PosZ", Current_z);
                                        command.Parameters.AddWithValue("@WorldID", WorldID);
                                        command.CommandTimeout = 60;
                                        command.ExecuteNonQuery();
                                        persisted = true;
                                    }
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error($"newreverse hata: {ex.Message}");
                            }
                        });


                        if (!persisted)
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("NEW_REVERSE_SAVE_ERROR");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }

                        session.SessionData.CharacterNewReverseSavedLocations[locationID] = str;
                        Packet Info = new Packet(0x180C);
                        Info.WriteUInt8(locationID);
                        Info.WriteInt32(Region);
                        await session.SendToClient(Info);
                        // Islem basarili, geri d�n
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                else
                {
                    string noticeMessage = RefManager.GetNoticeMessage("NEW_REVERSE_SAVE_ERROR");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "NEW_REVERSE_SAVE_LOCATION");
            }
            return new PacketResult(PacketResultType.Block);
        }
    }
}
