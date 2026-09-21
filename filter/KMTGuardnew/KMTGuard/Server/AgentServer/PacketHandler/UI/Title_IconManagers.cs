using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class Title_IconManagers
    {
        public Title_IconManagers(AgentServer agentServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x169C, HandleIconManager);
            packetHandler.RegisterClientHandler(0x169B, HandleTitleManager);
        }
        private async Task<PacketResult> HandleIconManager(Packet packet, ISession session, object obj)
        {
            try
            {
                byte side = packet.ReadUInt8();
                int ownershipId = packet.ReadInt32();
                if (side is not (0 or 1))
                    return new PacketResult(PacketResultType.Block);

                string characterName = session.SessionData.Charname;
                if (ownershipId == 0)
                {
                    var activeIcons = side == 0
                        ? RefManager.ActiveLeftIcons
                        : RefManager.ActiveRightIcons;
                    if (!activeIcons.TryRemove(characterName, out _))
                        return new PacketResult(PacketResultType.Block);

                    Packet removePacket = new Packet((ushort)(side == 0 ? 0x174A : 0x174E));
                    removePacket.WriteAscii(characterName);
                    await ServerManager.BroadcastPacket(removePacket);

                    await DatabaseJobQueue.RunAsync(() =>
                    {
                        try
                        {
                            using var connection = new SqlConnection(Program.Connectionstring);
                            connection.Open();
                            connection.Execute(
                                side == 0
                                    ? "EXEC [dbo].[LeftIcon_Deactivate] @CharName, 0"
                                    : "EXEC [dbo].[RightIcon_Deactivate] @CharName, 0",
                                new { CharName = characterName },
                                commandTimeout: 60);
                        }
                        catch (Exception ex)
                        {
                            Log.Error(ex,
                                "Failed to deactivate {Side} icon for {CharacterName}",
                                side == 0 ? "left" : "right",
                                characterName);
                        }
                    });
                    return new PacketResult(PacketResultType.Block);
                }

                if (!session.SessionData.PlayerIcons.TryGetValue(ownershipId, out var ownedIcon) ||
                    ownedIcon.Side != side)
                {
                    return new PacketResult(PacketResultType.Block);
                }

                var targetCache = side == 0
                    ? RefManager.ActiveLeftIcons
                    : RefManager.ActiveRightIcons;
                if (targetCache.TryGetValue(characterName, out int activeIconId) &&
                    activeIconId == ownedIcon.IconID)
                {
                    return new PacketResult(PacketResultType.Block);
                }

                targetCache[characterName] = ownedIcon.IconID;
                Packet activatePacket = new Packet((ushort)(side == 0 ? 0x173F : 0x174B));
                activatePacket.WriteAscii(characterName);
                activatePacket.WriteInt32(ownedIcon.IconID);
                await ServerManager.BroadcastPacket(activatePacket);

                await DatabaseJobQueue.RunAsync(() =>
                {
                    try
                    {
                        using var connection = new SqlConnection(Program.Connectionstring);
                        connection.Open();
                        connection.Execute(
                            side == 0
                                ? "EXEC [dbo].[LeftIcon_Activate] @CharID, @CharName, @IconID, 0"
                                : "EXEC [dbo].[RightIcon_Activate] @CharID, @CharName, @IconID, 0",
                            new
                            {
                                CharID = session.SessionData.Charid,
                                CharName = characterName,
                                ownedIcon.IconID
                            },
                            commandTimeout: 60);
                    }
                    catch (Exception ex)
                    {
                        Log.Error(ex,
                            "Failed to activate {Side} icon {IconID} for {CharacterName}",
                            side == 0 ? "left" : "right",
                            ownedIcon.IconID,
                            characterName);
                    }
                });
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "HandleIconManager");
            }
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> HandleTitleManager(Packet packet, ISession session, object obj)
        {
            try
            {
                byte type = packet.ReadUInt8();
                if (type == 0)
                {
                    int TitleID = packet.ReadInt32();
                    if (session.SessionData.State.BodyState == BodyState.Berserk)
                    {
                        return new PacketResult(PacketResultType.Block);
                    }
                    if (TitleID == 0)
                    {
                        Packet stAckMsg = new Packet(0x3501);
                        stAckMsg.WriteAscii(session.GameServerPacketKey);
                        stAckMsg.WriteUInt8(TitleID);
                        await session.SendToServer(stAckMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                    if (session.SessionData.PlayerTitles.Count() > 0)
                    {
                        if (session.SessionData.PlayerTitles.Contains(TitleID))
                        {
                            Packet stAckMsg = new Packet(0x3501);
                            stAckMsg.WriteAscii(session.GameServerPacketKey);
                            stAckMsg.WriteUInt8(TitleID);
                            await session.SendToServer(stAckMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                }
                else if (type == 1)
                {
                    int DBID = packet.ReadInt32();
                    if (DBID == 0)
                    {
                        if (RefManager.ActiveTitleColors.ContainsKey(session.SessionData.Charname))
                        {
                    RefManager.ActiveTitleColors.TryRemove(session.SessionData.Charname, out _);


                            Packet stAckMsg = new Packet(0x170B);
                            stAckMsg.WriteAscii(session.SessionData.Charname);
                            await ServerManager.BroadcastPacket(stAckMsg);

                            await DatabaseJobQueue.RunAsync(() =>
                            {
                                try
                                {
                                    using (var connection = new SqlConnection(Program.Connectionstring))
                                    {
                                        connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                        connection.Execute(
                                            "EXEC [dbo].[TitleColor_Deactivate] @CharName, 0",
                                            new { CharName = session.SessionData.Charname },
                                            commandTimeout: 60);
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Log.Error($"HandleHwidList hata: {ex.Message}");
                                }
                            });
                            return new PacketResult(PacketResultType.Block);
                        }
                    }

                    else
                    {
                        if (session.SessionData.PlayerTitleColors.ContainsKey(DBID))
                        {
                            int argbInputColor = Int32.Parse(session.SessionData.PlayerTitleColors[DBID].ColorCode.Replace("#", ""), NumberStyles.HexNumber);

                            Packet stAckMsg = new Packet(0x170A);
                            stAckMsg.WriteAscii(session.SessionData.Charname);
                            stAckMsg.WriteUInt32(argbInputColor);
                            await ServerManager.BroadcastPacket(stAckMsg);

                            RefManager.ActiveTitleColors[session.SessionData.Charname] =
                                session.SessionData.PlayerTitleColors[DBID].ColorCode;
                            await DatabaseJobQueue.RunAsync(() =>
                            {
                                try
                                {
                                    using (var connection = new SqlConnection(Program.Connectionstring))
                                    {
                                        connection.Open(); // OpenAsync() yerine senkron a�ma daha g�venli

                                        connection.Execute(
                                            "EXEC [dbo].[TitleColor_Activate] @CharID, @CharName, @ColorID, 0",
                                            new
                                            {
                                                CharID = session.SessionData.Charid,
                                                CharName = session.SessionData.Charname,
                                                ColorID = DBID
                                            },
                                            commandTimeout: 60);
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Log.Error($"[dbo].[TitleColor_Activate] failed: {ex.Message}");
                                }
                            });
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
               
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "HandleTitleManager");
            }
            return new PacketResult(PacketResultType.Block);
        }
    }
}
