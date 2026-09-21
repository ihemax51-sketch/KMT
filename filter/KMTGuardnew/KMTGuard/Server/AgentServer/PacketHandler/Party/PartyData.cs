using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;
using XFilterV2.Helpers;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class PartyData
    {
        public PartyData(AgentServer AgentServer, IPacketHandler PacketHandler)
        {
            PacketHandler.RegisterModuleHandler(0xB067, SERVER_JOINED_PARTY);
            PacketHandler.RegisterModuleHandler(0xB060, SERVER_CREATED_PARTY);
            PacketHandler.RegisterModuleHandler(0xB069, SERVER_PARTY_MATCHING_CREATED);
            PacketHandler.RegisterModuleHandler(0x3065, HandlePartyInfoAck);

            PacketHandler.RegisterModuleHandler(0x3864, HandlePartyUpdate);

            PacketHandler.RegisterClientHandler(0x7069, CLIENT_AGENT_PARTY_MATCHING_FORM_REQUEST);
            PacketHandler.RegisterClientHandler(0x7060, CLIENT_AGENT_PARTY_CREATE_REQUEST);
            PacketHandler.RegisterClientHandler(0x706D, CLIENT_AGENT_PARTY_MATCHING_JOIN_REQUEST);
            PacketHandler.RegisterClientHandler(0x7062, CLIENT_AGENT_PARTY_INVITE_REQUEST);
            PacketHandler.RegisterClientHandler(0x7061, PARTY_LEAVE_REQUEST);
            PacketHandler.RegisterClientHandler(0x7063, PARTY_BAN_REQUEST);
        }

        private async Task<PacketResult> PARTY_LEAVE_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {

                if (session.SessionData.WorldID == 99)
                {
                    await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Party.CannotWithdrawInDungeon"));
                    return new PacketResult(PacketResultType.Block);
                }


            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "PARTY_LEAVE_REQUEST");
            }
            return new PacketResult(PacketResultType.Nothing);
        }
        private async Task<PacketResult> PARTY_BAN_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                uint memberjid = packet.ReadUInt32();
                if (session.SessionData.WorldID == 99)
                {
                    await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Party.CannotDisbandInDungeon"));
                    return new PacketResult(PacketResultType.Block);
                }


            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "PARTY_BAN_REQUEST");
            }
            return new PacketResult(PacketResultType.Nothing);
        }

        private async Task<PacketResult> CLIENT_AGENT_PARTY_INVITE_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                var regionBlock = await BlockPartyByRegionAsync(session);
                if (regionBlock != null)
                    return regionBlock;

                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].HideName)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.WorldID].HideName)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "CLIENT_AGENT_PARTY_MATCHING_FORM_REQUEST");
            }

            return new PacketResult();
        }
        private async Task<PacketResult> CLIENT_AGENT_PARTY_MATCHING_FORM_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                var regionBlock = await BlockPartyByRegionAsync(session);
                if (regionBlock != null)
                    return regionBlock;

                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].HideName)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.WorldID].HideName)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "CLIENT_AGENT_PARTY_MATCHING_FORM_REQUEST");
            }

            AutoEventService.MarkPartyMatchingFormRequested(session);
            return new PacketResult();
        }
        private async Task<PacketResult> CLIENT_AGENT_PARTY_CREATE_REQUEST(Packet packet, ISession session, object obj)
        {
            var regionBlock = await BlockPartyByRegionAsync(session);
            if (regionBlock != null)
                return regionBlock;

            if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
            {
                if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].HideName)
                {
                    string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }
            else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
            {
                if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.WorldID].HideName)
                {
                    string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }

            // Lucky Party intentionally accepts only a successful Party Matching
            // Form (0x7069 -> 0xB069), never ordinary party creation (0x7060).
            return new PacketResult();
        }
        private async Task<PacketResult> CLIENT_AGENT_PARTY_MATCHING_JOIN_REQUEST(Packet packet, ISession session, object obj)
        {
            var regionBlock = await BlockPartyByRegionAsync(session);
            if (regionBlock != null)
                return regionBlock;

            if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
            {
                if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].HideName)
                {
                    string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }
            else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
            {
                if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisableParty || RefManager.m_RefEventMapSettings[session.SessionData.WorldID].HideName)
                {
                    string noticeMessage = RefManager.GetNoticeMessage("MSG_PARTY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }
            return new PacketResult();
        }

        private static async Task<PacketResult?> BlockPartyByRegionAsync(ISession session)
        {
            return await RegionControlService.BlockIfAsync(
                session,
                rule => !rule.Enable_Party,
                "Region.PartyDisabled");
        }

        private async Task<PacketResult> HandlePartyUpdate(Packet packet, ISession session, object obj)
        {
            try
            {
                var eUpdateType = (E_PARTY_UPDATE_TYPE)packet.ReadUInt8();

                if (eUpdateType == E_PARTY_UPDATE_TYPE.DISMISSED)
                {
                    //CPartyMgr.RemoveParty(nMyPartyID);

                    session.SessionData.IsInParty = false;
                    session.SessionData.IsPartyMaster = false;
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron açma daha güvenli

                                using (var command = new SqlCommand(
                                    @"IF EXISTS (SELECT 1 FROM [dbo].[Party_Members] WITH (NOLOCK) WHERE CharID = @CharID)
                                      BEGIN DELETE FROM [dbo].[Party_Members] WHERE CharID = @CharID END",
                                    connection))
                                {
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.CommandTimeout = 60;
                                    command.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"[dbo].[Party_Members] DELETE işleminde hata: {ex.Message}");
                        }
                    });
                    return new PacketResult(packet, PacketResultType.Nothing);
                }

                if (eUpdateType == E_PARTY_UPDATE_TYPE.MEMBER_LEFT)
                {
                    //NOTE: can get member by party id &member index but not needed atm actually needed ^ ^
                    var nMemberID = packet.ReadUInt32();
                    var eLeaveType = (E_PARTY_LEAVE_TYPE)packet.ReadUInt8();
                    //CPartyMgr.TryGetPartyMemberByMemberID(out SPartyMemberInfo res, nMyPartyID, nMemberID);
                    if (nMemberID == session.SessionData.JID)
                    {
                        session.SessionData.IsInParty = false;
                        session.SessionData.IsPartyMaster = false;
                        await DatabaseJobQueue.RunAsync(() =>
                        {
                            try
                            {
                                using (var connection = new SqlConnection(Program.Connectionstring))
                                {
                                    connection.Open(); // OpenAsync() yerine senkron açma daha güvenli

                                    using (var command = new SqlCommand(
                                        @"IF EXISTS (SELECT 1 FROM [dbo].[Party_Members] WITH (NOLOCK) WHERE CharID = @CharID)
                                          BEGIN DELETE FROM [dbo].[Party_Members] WHERE CharID = @CharID END",
                                        connection))
                                    {
                                        command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                        command.CommandTimeout = 60;
                                        command.ExecuteNonQuery();
                                    }
                                }
                            }
                            catch (Exception ex)
                            {
                                Log.Error($"[dbo].[Party_Members] DELETE işleminde hata: {ex.Message}");
                            }
                        });
                    }
                    return new PacketResult(packet, PacketResultType.Nothing);
                }

                if (eUpdateType == E_PARTY_UPDATE_TYPE.LEADER_CHANGE)
                {
                    var nLeaderJID = packet.ReadUInt32();
                    int isMaster = (session.SessionData.JID == nLeaderJID) ? 1 : 0;
                    session.SessionData.IsPartyMaster = isMaster == 1;
                    if (nLeaderJID == session.SessionData.JID)
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                await connection.OpenAsync();

                                int PartyID = await connection.QueryFirstOrDefaultAsync<int>(
                                    "SELECT PartyID FROM [dbo].[Party_Members] WITH (NOLOCK) WHERE CharID = @CharID",
                                    new { CharID = session.SessionData.Charid }); // Dapper ile parametre geçiliyor

                                if (PartyID > 0)
                                {
                                    string query = @"
                                    IF EXISTS (SELECT 1 FROM [dbo].[Party_Members] WHERE PartyID = @PartyID)
                                    BEGIN
                                        UPDATE [dbo].[Party_Members] SET IsMaster = 0 WHERE PartyID = @PartyID;
                                        UPDATE [dbo].[Party_Members] SET IsMaster = 1 WHERE PartyID = @PartyID AND CharID = @NewLeaderID;
                                    END";

                                    await connection.ExecuteAsync(query, new
                                    {
                                        PartyID = PartyID,
                                        NewLeaderID = session.SessionData.Charid // Parametre değerleri
                                    });
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"[dbo].[Party_Members] UPDATE işleminde hata: {ex.Message}");
                        }
                    }

                    //CPartyMgr.UpdatePartyLeader(nMyPartyID, nLeaderID);
                    return new PacketResult(packet, PacketResultType.Nothing);
                }
                return new PacketResult(packet, PacketResultType.Nothing);
            }
            catch (Exception ex)
            {
                Log.Warning("HandlePartyUpdate Hata oluştu: " + ex.Message);
                return new PacketResult(packet, PacketResultType.Nothing);
            }
        }
        private async Task<PacketResult> HandlePartyInfoAck(Packet packet, ISession session, object obj)
        {
            try
            {
                SPartyMemberInfo stMemberInfo = new();
                packet.ReadUInt8(); //skip 0xFF
                var nPartyID = packet.ReadUInt32();
                var nLeaderJID = packet.ReadUInt32();
                var eSettingFlags = (E_PARTY_SETTINGS_FLAG)packet.ReadUInt8();

                int isMaster = (session.SessionData.JID == nLeaderJID) ? 1 : 0;
                session.SessionData.IsPartyMaster = isMaster == 1;
                if (!session.SessionData.IsInParty)
                {
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron açma daha güvenli

                                using (var command = new SqlCommand(
                                    @"IF NOT EXISTS (SELECT 1 FROM [dbo].[Party_Members] WITH (NOLOCK) WHERE CharID = @CharID)
                                      BEGIN
                                          INSERT INTO [dbo].[Party_Members]
                                              (PartyID, CharID, CharName, PartyType, IsMaster, JobStatus, UpdatedAt)
                                          VALUES
                                              (@PartyID, @CharID, @CharName, @PartyType, @IsMaster, @JobStatus, @UpdatedAt)
                                      END",
                                    connection))
                                {
                                    command.Parameters.AddWithValue("@PartyID", (int)nPartyID);
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname ?? string.Empty);
                                    command.Parameters.AddWithValue("@PartyType", (byte)eSettingFlags);
                                    command.Parameters.AddWithValue("@IsMaster", isMaster);
                                    command.Parameters.AddWithValue("@JobStatus", session.SessionData.JobType);
                                    command.Parameters.AddWithValue("@UpdatedAt", DateTime.Now);
                                    command.CommandTimeout = 60;
                                    command.ExecuteNonQuery();
                                }

                                // سجل في _PartyMatchingCreated عند أول دخول/إنشاء للحزب
                                using (var insertCmd = new SqlCommand(@"
IF NOT EXISTS (SELECT 1 FROM [dbo].[Party_MatchingLog] WITH (NOLOCK) WHERE PartyNo = @PartyNo AND CharID = @CharID)
BEGIN
    INSERT INTO [dbo].[Party_MatchingLog] (CharID, CharName, RegionID, WorldID, PartyNo)
    VALUES (@CharID, @CharName, @RegionID, @WorldID, @PartyNo)
END", connection))
                                {
                                    insertCmd.Parameters.AddWithValue("@PartyNo", (int)nPartyID);
                                    insertCmd.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    insertCmd.Parameters.AddWithValue("@CharName", session.SessionData.Charname ?? string.Empty);
                                    insertCmd.Parameters.AddWithValue("@RegionID", Convert.ToInt16(session.SessionData.LatestRegion));
                                    insertCmd.Parameters.AddWithValue("@WorldID", session.SessionData.WorldID);
                                    insertCmd.CommandTimeout = 60;
                                    insertCmd.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"[dbo].[Party_Members]/[dbo].[Party_MatchingLog] INSERT işleminde hata: {ex.Message}");
                        }
                    });
                    session.SessionData.IsInParty = true;
                }


                //uint MyMemberID = 0;
                //var btMemberCount = packet.ReadUInt8();
                //for (int i = 0; i < btMemberCount; i++)
                //{
                //    packet.ReadUInt8(); // Skip 0xFF
                //    var nMemberID = packet.ReadUInt32(); // That seems to be account JID
                //    var strName = packet.ReadAscii();
                //    var nRefObjID = packet.ReadUInt32();
                //    var btLevel = packet.ReadUInt8();
                //    var btHpMana = packet.ReadUInt8(); // Union struct, 4 bits hp, 4 bits mp, 0 - 10 iirc
                //    stMemberInfo.stPos.ParseFromMsg(packet, true);
                //    var strGuildName = packet.ReadAscii();
                //    packet.ReadUInt8(); // Skip 0x04
                //    var nMasteryId1 = packet.ReadUInt32();
                //    var nMasteryId2 = packet.ReadUInt32();
                //    if (strName == session.SessionData.Charname)
                //    {
                //        MyMemberID = nMemberID;
                //    }
                //}

                //if(MyMemberID != 0)
                //{

                //}


                //session.SessionData.PartyMemberID = MyMemberID;
                return new PacketResult(packet, PacketResultType.Nothing);
            }
            catch (Exception ex)
            {
                Log.Warning("HandlePartyInfoAck Hata oluştu: " + ex.Message);
                return new PacketResult(packet, PacketResultType.Nothing);
            }
        }
        private async Task<PacketResult> SERVER_PARTY_MATCHING_CREATED(Packet packet, ISession session, PacketData data)
        {
            try
            {
                var btResult = packet.ReadUInt8(); // 1   byte    result
                if (btResult == 0x01)
                {
                    int nMatchingID = packet.ReadInt32();
                    AutoEventService.RegisterPartyMatchingFormCreated(session, nMatchingID);
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron açma daha güvenli

                                using (var command = new SqlCommand(@"
INSERT INTO [dbo].[Party_MatchingLog] (CharID, CharName, RegionID, WorldID, PartyNo)
VALUES (@CharID, @CharName, @RegionID, @WorldID, @PartyNo)", connection))
                                {
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname ?? string.Empty);
                                    command.Parameters.AddWithValue("@RegionID", Convert.ToInt16(session.SessionData.LatestRegion));
                                    command.Parameters.AddWithValue("@WorldID", session.SessionData.WorldID);
                                    command.Parameters.AddWithValue("@PartyNo", nMatchingID);
                                    command.CommandTimeout = 60;

                                    command.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"[dbo].[Party_MatchingLog] INSERT işleminde hata: {ex.Message}");
                        }
                    });

                }
                else
                {
                    AutoEventService.CancelPartyMatchingFormRequest(session);
                }
            }
            catch (Exception EX)
            {
                AutoEventService.CancelPartyMatchingFormRequest(session);
                Log.Fatal(EX.Message.ToString() + "SERVER_PARTY_MATCHING_CREATED");
            }

            return new PacketResult();
        }
        private async Task<PacketResult> SERVER_CREATED_PARTY(Packet packet, ISession session, object obj)
        {
            try
            {
                byte successflag = packet.ReadUInt8();
                if (successflag == 1)
                {
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron açma daha güvenli

                                using (var command = new SqlCommand(@"
IF NOT EXISTS (SELECT 1 FROM [dbo].[Party_MatchingLog] WITH (NOLOCK) WHERE CharID = @CharID AND PartyNo = @PartyNo)
BEGIN
    INSERT INTO [dbo].[Party_MatchingLog] (CharID, CharName, RegionID, WorldID, PartyNo)
    VALUES (@CharID, @CharName, @RegionID, @WorldID, @PartyNo)
END", connection))
                                {
                                    // PartyNo هنا سيتم تسجيله لاحقًا بدقة في HandlePartyInfoAck، نضع 0 مؤقتًا لتسجيل حدث الإنشاء
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname ?? string.Empty);
                                    command.Parameters.AddWithValue("@RegionID", Convert.ToInt16(session.SessionData.LatestRegion));
                                    command.Parameters.AddWithValue("@WorldID", session.SessionData.WorldID);
                                    command.Parameters.AddWithValue("@PartyNo", 0);
                                    command.CommandTimeout = 60;

                                    command.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                            Log.Error($"[dbo].[Party_MatchingLog] INSERT (create) işleminde hata: {ex.Message}");
                        }
                    });

                }

            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + "SERVER_CREATED_PARTY");
            }

            return new PacketResult();
        }
        private async Task<PacketResult> SERVER_JOINED_PARTY(Packet packet, ISession session, object obj)
        {
            try
            {
                byte successflag = packet.ReadUInt8();
                if (successflag == 1)
                {
                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using (var connection = new SqlConnection(Program.Connectionstring))
                            {
                                connection.Open(); // OpenAsync() yerine senkron açma daha güvenli

                                using (var command = new SqlCommand(
                                    "EXEC [dbo].[Hook_PartyJoin] @CharID, @CharName, @CurrentRegionId, @CurrentWorldId, @PVPState, @CurrentJobType", connection))
                                {
                                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname);
                                    command.Parameters.AddWithValue("@CurrentRegionId", Convert.ToInt16(session.SessionData.LatestRegion));
                                    command.Parameters.AddWithValue("@CurrentWorldId", session.SessionData.WorldID);
                                    command.Parameters.AddWithValue("@PVPState", Convert.ToByte(session.SessionData.State.PvpCape));
                                    command.Parameters.AddWithValue("@CurrentJobType", Convert.ToByte(session.SessionData.JobType));


                                    command.CommandTimeout = 60;

                                    command.ExecuteNonQuery();
                                }
                            }
                        }
                        catch (Exception ex)
                        {
                        Log.Error($"[dbo].[Hook_PartyJoin] işleminde hata: {ex.Message}");
                        }
                    });
                }

            }
            catch (Exception ex)
            {
                Log.Error(ex.Message.ToString() + "SERVER_JOINED_PARTY");
            }

            return new PacketResult();
        }
    }
}
