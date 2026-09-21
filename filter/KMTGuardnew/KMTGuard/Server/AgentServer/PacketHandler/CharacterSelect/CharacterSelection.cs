using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.AsyncServerManager;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Microsoft.IdentityModel.Tokens;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Servers.PacketHandler
{
    public partial class CharacterSelection
    {
        bool loggedcharacter { get; set; } = false;
        public CharacterSelection(AgentServer agentServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x7001, CLIENT_CHARACTER_SELECT_REQUEST);
            packetHandler.RegisterModuleHandler(0xB007, SERVER_CHARACTER_SELECTION_ACTION);
            packetHandler.RegisterModuleHandler(0xB001, SERVER_ENTER_GAME_ACTION);
            packetHandler.RegisterClientHandler(0x7007, CLIENT_AGENT_CHARACTER_SELECTION_ACTION_REQUEST); // same as above

        }
        private async Task<PacketResult> CLIENT_AGENT_CHARACTER_SELECTION_ACTION_REQUEST(Packet packet, ISession session, object obj)
        {
            if (!session.CharScreen)
            {
                Log.Warning($"Client {session.SessionData.Charname}({packet.Opcode}) attempted to send 0x7007 outside char screen!");
                return new PacketResult(PacketResultType.Disconnect);
            }
            await FuckingRead(packet);
            if (packet.RemainingRead() != 0)
            {
                Log.Warning($"Client {session.SessionData.Charname}({packet.Opcode}) crash SHARD_MANAGER!");

                return new PacketResult(PacketResultType.Disconnect);
            }
            return new PacketResult();
        }
        public CharacterSelectionAction Action { get; set; }
        public Task FuckingRead(Packet packet)
        {
            Action = (CharacterSelectionAction)packet.ReadUInt8(); // 1   byte    action
            if (Action == CharacterSelectionAction.Create)
            {
                // 2   ushort  Name.Length
                //  *   string  Name
                var Name = packet.ReadAscii();
                var RefObjID = packet.ReadUInt32(); // 4   uint    RefObjID
                var Scale = packet.ReadUInt8(); // 1   byte    Scale
                var EQUIP_SLOT_MAIL = packet.ReadUInt32(); // 4   uint    RefItemID // EQUIP_SLOT_MAIL
                var EQUIP_SLOT_PANTS = packet.ReadUInt32(); // 4   uint    RefItemID // EQUIP_SLOT_PANTS
                var EQUIP_SLOT_BOOTS = packet.ReadUInt32(); // 4   uint    RefItemID // EQUIP_SLOT_BOOTS
                var EQUIP_SLOT_WEAPON = packet.ReadUInt32(); // 4   uint    RefItemID // EQUIP_SLOT_WEAPON
            }
            else if (Action == CharacterSelectionAction.Delete ||
               Action == CharacterSelectionAction.CheckName ||
               Action == CharacterSelectionAction.Restore)
            {
                // 2 ushort Name.Length
                //  * string Name
                var Name = packet.ReadAscii();
            }

            return Task.CompletedTask;
        }

        private async Task<PacketResult> CLIENT_CHARACTER_SELECT_REQUEST(Packet packet, ISession session, object obj)
        {
            if (session.CharnameSent)
                return new PacketResult(PacketResultType.Block);

            if (!session.CharScreen)
            {
                Log.Error($"Client {session.ClientGuid}({session.ClientIp}) attempted to send 0x7001 outside char screen!", ConsoleColor.Red);
                return new PacketResult(PacketResultType.Disconnect);
            }

            session.SessionData.ClearFellowRuntimeState();
            session.SessionData.Charname = packet.ReadAscii();

            if (session.PacketLength - session.SessionData.Charname.Length != 2)
            {
                Log.Error($"Client {session.ClientGuid}({session.ClientIp}) attempted to modify 0x7001!", ConsoleColor.Red);
                return new PacketResult(PacketResultType.Disconnect);
            }

            ServerManager.AgentSessions.RefreshIndexes(session);
            session.CharnameSent = true;


            if (string.IsNullOrEmpty(session.SessionData.Hwid))
            {
                if (!await BotProtectionService.TryAdmitExternalBotSessionAsync(session))
                {
                    Log.Warning(
                        "Unverified client blocked at character selection. Account={AccountName} IP={ClientIp}",
                        session.PlayerUserID,
                        session.ClientIp);
                    BotProtectionService.QueueAudit(session, "LoginBlocked", "Disconnect", "missing-agent-client-proof");
                    return new PacketResult(PacketResultType.Disconnect);
                }
            }

            var selectedCharName = session.SessionData.Charname;
            if (await OfflineStallService.TryReplaceCharacterSelectionAsync(
                    session,
                    selectedCharName,
                    async () =>
                    {
                        var replay = new Packet(0x7001, true, false);
                        replay.WriteAscii(selectedCharName);
                        await session.SendToServer(replay);
                    }))
            {
                return new PacketResult(PacketResultType.Block);
            }



            return new PacketResult();
        }
        private async Task<PacketResult> SERVER_CHARACTER_SELECTION_ACTION(Packet packet, ISession session, object obj)
        {
            try
            {
                session.SessionData.ClearFellowRuntimeState();
                if (string.IsNullOrEmpty(session.SessionData.Hwid) &&
                    HwidSecurity.TryCreateChallenge(
                        session, HwidSecurity.AgentRole, out var challenge))
                {
                    Packet hwid = new Packet(0x165A);
                    hwid.WriteAscii(challenge);
                    session.QueueClientPacketAfterCurrentServerPacket(hwid);
                }
                if (packet.ReadUInt8() == 0x02)
                {
                    using (var connection = new SqlConnection(Program.Connectionstring))
                    {
                        await connection.OpenAsync();
                        if (packet.ReadUInt8() == 0x01)
                        {
                            byte char_count = packet.ReadUInt8();
                            //MenuSettings();
                            Packet Info = new Packet(0x1199);
                            Info.WriteUInt8(char_count);
                            for (int cc = 0; cc < char_count; cc++)
                            {
                                #region MainEntry

                                packet.ReadUInt32(); //Model
                                string cn16 = packet.ReadAscii(); // Name
                                packet.ReadUInt8(); //Volume/Height
                                packet.ReadUInt8(); //Level
                                packet.ReadUInt64(); //Exp
                                packet.ReadUInt16(); //STR
                                packet.ReadUInt16(); //INT
                                packet.ReadUInt16(); //Stats points
                                packet.ReadUInt32(); //Hp
                                packet.ReadUInt32(); //Mp
                                                     //int Last = await sqlQueryHelper.prod_int($"SELECT LatestRegion FROM {Service.ShardDB}.._Char WITH (NOLOCK) WHERE CharName16 = '{cn16}'", Program.Connectionstring);
                                int Last = 0;

                                var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                                var search = await connection.QueryFirstOrDefaultAsync<int>(
                                    $"SELECT LatestRegion FROM {shardDb}.._Char WITH (NOLOCK) WHERE CharName16 = @CharName",
                                    new { CharName = cn16 });
                                if (search != 0)
                                {
                                    Last = search;
                                }


                                Info.WriteAscii(cn16);
                                Info.WriteInt32(Last);
                                #endregion MainEntry

                                #region Deletion

                                byte char_delete = packet.ReadUInt8();
                                if (char_delete == 1)
                                {
                                    packet.ReadUInt32();
                                }
                                packet.ReadUInt8(); //Unknown
                                packet.ReadUInt8(); //Unknown
                                packet.ReadUInt8(); //Unknown
                                Info.WriteUInt8(char_delete);
                                #endregion Deletion

                                #region Items

                                int ls_itemscount = packet.ReadUInt8();
                                //Log.Warning($"Total Items Count: {itemscount} for {cn16}");
                                for (int ic = 0; ic < ls_itemscount; ic++)
                                {
                                    uint Item_id = packet.ReadUInt32(); //Item ID
                                    packet.ReadUInt8(); //Plus Value

                                }

                                #endregion Items

                                #region Avatars

                                int avatarcount = packet.ReadUInt8(); //Avatar count
                                for (int ac = 0; ac < avatarcount; ac++)
                                {
                                    packet.ReadUInt32(); // Avatar ID
                                    packet.ReadUInt8(); // Plus
                                }

                                #endregion Avatars
                            }
                            session.QueueClientPacketAfterCurrentServerPacket(Info);

                            if (RefManager.ActiveNameColors.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x204E);
                                stAckMsg.WriteInt32(RefManager.ActiveNameColors.Count());
                                foreach (var line in RefManager.ActiveNameColors)
                                {
                                    stAckMsg.WriteAscii(line.Key); /// charname maybe
                                    int argbInputColor = Int32.Parse(line.Value.Replace("#", ""), NumberStyles.HexNumber);
                                    stAckMsg.WriteUInt32(argbInputColor);
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }

                            if (RefManager.ActiveTitleColors.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x204D);
                                stAckMsg.WriteInt32(RefManager.ActiveTitleColors.Count());
                                foreach (var line in RefManager.ActiveTitleColors)
                                {
                                    stAckMsg.WriteAscii(line.Key); /// charname maybe
                                    int argbInputColor = Int32.Parse(line.Value.Replace("#", ""), NumberStyles.HexNumber);
                                    stAckMsg.WriteUInt32(argbInputColor);
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }
                            if (RefManager.m_LuckySpin.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x206F);
                                stAckMsg.WriteInt32(RefManager.m_LuckySpin.Count());
                                foreach (var line in RefManager.m_LuckySpin)
                                {
                                    stAckMsg.WriteInt32(line.Value.ItemID);
                                    stAckMsg.WriteInt32(line.Value.Amount);
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }

                            if (RefManager.Icons.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x204F);
                                stAckMsg.WriteInt32(RefManager.Icons.Count());
                                foreach (var line in RefManager.Icons)
                                {
                                    stAckMsg.WriteInt32(line.Key);
                                    stAckMsg.WriteAscii(line.Value);
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }

                            if (RefManager.ActiveLeftIcons.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x205A);
                                stAckMsg.WriteInt32(RefManager.ActiveLeftIcons.Count());

                                foreach (var data in RefManager.ActiveLeftIcons)
                                {
                                    stAckMsg.WriteAscii(data.Key);
                                    stAckMsg.WriteInt32(data.Value);

                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }
                            if (RefManager.ActiveRightIcons.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x205B);
                                stAckMsg.WriteInt32(RefManager.ActiveRightIcons.Count());

                                foreach (var data in RefManager.ActiveRightIcons)
                                {
                                    stAckMsg.WriteAscii(data.Key);
                                    stAckMsg.WriteInt32(data.Value);

                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }


                            var activeTitles = RefManager.ActiveTags.ToArray();
                            if (activeTitles.Length > 0)
                            {
                                Packet stAckMsg = new Packet(0x204C);
                                stAckMsg.WriteInt32(activeTitles.Length);
                                foreach (var data in activeTitles)
                                {
                                    stAckMsg.WriteAscii(data.Key);

                                    if (RefManager.Tags.ContainsKey(data.Value))
                                    {
                                        stAckMsg.WriteAscii(RefManager.Tags[data.Value]);
                                    }
                                    else
                                    {
                                        stAckMsg.WriteAscii("");
                                    }
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }


                            var achievementSnapshot = RefManager.m_RefAchievements;
                            if (achievementSnapshot.Count > 0)
                            {
                                Packet stAckMsg = new Packet(0x205C);
                                stAckMsg.WriteInt32(achievementSnapshot.Count);
                                foreach (var data in achievementSnapshot)
                                {
                                    stAckMsg.WriteInt32(data.Value.ID);
                                    stAckMsg.WriteUInt8(data.Value.Category);
                                    stAckMsg.WriteAscii(data.Value.Name);
                                    stAckMsg.WriteUInt8(data.Value.RewardType);
                                    if (data.Value.RewardType == 0)
                                    {
                                        if (RefManager.Tags.ContainsKey(data.Value.RewardTagID))
                                        {
                                            stAckMsg.WriteAscii(RefManager.Tags[data.Value.RewardTagID]);
                                        }
                                        else
                                        {
                                            stAckMsg.WriteAscii("");
                                        }

                                    }

                                    stAckMsg.WriteInt32(data.Value.RewardSkillPoint);
                                    stAckMsg.WriteInt64(data.Value.RewardGold);

                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }

                            var achievementConditionSnapshot = RefManager.m_RefAchievementsCondition;
                            if (achievementConditionSnapshot.Count > 0)
                            {
                                Packet stAckMsg = new Packet(0x175A);
                                stAckMsg.WriteInt32(achievementConditionSnapshot.Count);
                                foreach (var data in achievementConditionSnapshot)
                                {
                                    stAckMsg.WriteInt32(data.Value.ID);
                                    stAckMsg.WriteAscii(data.Value.Name);
                                    stAckMsg.WriteInt32(data.Value.RefAchievementID);
                                    stAckMsg.WriteInt64(data.Value.CompleteCount);
                                    stAckMsg.WriteUInt8(data.Value.Type);
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }




                            if (RefManager.m_HideSkillEffects.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x209E);
                                stAckMsg.WriteInt32(RefManager.m_HideSkillEffects.Count());
                                foreach (var data in RefManager.m_HideSkillEffects)
                                {
                                    stAckMsg.WriteInt32(data.Key);
                                    stAckMsg.WriteBool(data.Value.JobMode);
                                    stAckMsg.WriteBool(data.Value.MapSettings);

                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }

                            if (RefManager.m_RefEventMapSettings.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x207E);
                                stAckMsg.WriteInt32(RefManager.m_RefEventMapSettings.Count());
                                foreach (var data in RefManager.m_RefEventMapSettings)
                                {
                                    stAckMsg.WriteInt32(data.Key);
                                    stAckMsg.WriteBool(data.Value.EventSuit);
                                    stAckMsg.WriteBool(data.Value.HideBuffViewer);
                                    stAckMsg.WriteBool(data.Value.DisablePetSpawn);
                                    stAckMsg.WriteBool(data.Value.DisableParty);
                                    stAckMsg.WriteBool(data.Value.AutoCape);
                                    stAckMsg.WriteBool(data.Value.HideMiniMap);
                                    stAckMsg.WriteByte(data.Value.RegionType);
                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }

                            if (RefManager.m_RefFellowData.Count() > 0)
                            {
                                Packet stAckMsg = new Packet(0x206A);
                                stAckMsg.WriteUInt8(RefManager.m_RefFellowData.Count());
                                foreach (var data in RefManager.m_RefFellowData)
                                {
                                    stAckMsg.WriteAscii(data.Key);
                                    stAckMsg.WriteInt32(data.Value.SkillID_1);
                                    stAckMsg.WriteUInt8(data.Value.Active_Level_1);
                                    stAckMsg.WriteUInt8(data.Value.SkillType_1);
                                    stAckMsg.WriteInt32(data.Value.SkillID_2);
                                    stAckMsg.WriteUInt8(data.Value.Active_Level_2);
                                    stAckMsg.WriteUInt8(data.Value.SkillType_2);
                                    stAckMsg.WriteInt32(data.Value.SkillID_3);
                                    stAckMsg.WriteUInt8(data.Value.Active_Level_3);
                                    stAckMsg.WriteUInt8(data.Value.SkillType_3);
                                    stAckMsg.WriteInt32(data.Value.SkillID_4);
                                    stAckMsg.WriteUInt8(data.Value.Active_Level_4);
                                    stAckMsg.WriteUInt8(data.Value.SkillType_4);
                                    stAckMsg.WriteInt32(data.Value.SkillID_5);
                                    stAckMsg.WriteUInt8(data.Value.Active_Level_5);
                                    stAckMsg.WriteUInt8(data.Value.SkillType_5);

                                    stAckMsg.WriteInt32(data.Value.SelfSkill_1);
                                    stAckMsg.WriteUInt8(data.Value.SelfSkill_Active_Level_1);

                                    stAckMsg.WriteInt32(data.Value.SelfSkill_2);
                                    stAckMsg.WriteUInt8(data.Value.SelfSkill_Active_Level_2);

                                }
                                session.QueueClientPacketAfterCurrentServerPacket(stAckMsg);
                            }
                        }
                    }
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "-SERVER_CHARACTER_SELECTION_ACTION ");
            }
            return new PacketResult(packet, PacketResultType.Nothing);
        }


        private async Task<PacketResult> SERVER_ENTER_GAME_ACTION(Packet packet, ISession session, object obj)
        {
            try
            {
                byte btType = packet.ReadUInt8();

                if (btType != 1)
                    return new PacketResult(PacketResultType.Nothing);

                session.SessionData.ClearFellowRuntimeState();
                string strCharName = session.SessionData.Charname;
                var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
                int charId;
                await using (var lookupConnection = new SqlConnection(Program.Connectionstring))
                {
                    charId = await lookupConnection.QuerySingleOrDefaultAsync<int>(
                        $"SELECT CharID FROM {shardDb}.._Char WITH (NOLOCK) WHERE CharName16 = @CharName",
                        new { CharName = strCharName });
                }
                session.SessionData.Charid = charId;
                ServerManager.AgentSessions.RefreshIndexes(session);
                session.SessionData.CharacterChest.Clear();
                session.SessionData.PendingChestClaims.Clear();

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    // İlk sorgu: _ActiveTitleColors tablosu
                    string query1 = $"SELECT Email, JID FROM {accountDb}..TB_User WITH (NOLOCK) WHERE StrUserID = @UserId";
                    var result1 = await connection.QueryAsync<(string Mail, int JID)>(
                        query1, new { UserId = session.PlayerUserID });
                    foreach (var item in result1)
                    {
                        session.SessionData.MailAddress = item.Mail;
                        session.SessionData.JID = item.JID;
                    }
                }
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    var characterSettings = await connection.QueryFirstOrDefaultAsync<_CharacterSettings>(
                        @"SELECT TOP (1) [HideItemInfo]
                          FROM [dbo].[Player_Settings] WITH (NOLOCK)
                          WHERE [CharName16] = @CharName16",
                        new { CharName16 = session.SessionData.Charname });

                    session.SessionData.HideCharInformation = characterSettings?.HideItemInfo ?? false;
                }

                return new PacketResult(PacketResultType.Nothing);
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "SERVER_ENTER_GAME_ACTION");
                return new PacketResult(PacketResultType.Block);
            }
        }
    }
}
