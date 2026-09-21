using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class CharDataPackets
    {
        public CharDataPackets(AgentServer agentServer, IPacketHandler packetHandler) 
        {
            packetHandler.RegisterModuleHandler(0x3013, SERVER_AGENT_CHARACTER_DATA); // Character Spawn Packet
            packetHandler.RegisterModuleHandler(0x3054, SERVER_LEVELINFO);
            packetHandler.RegisterClientHandler(0x6103, SERVER_USER_INFO);
        }
        private Task<PacketResult> SERVER_AGENT_CHARACTER_DATA(Packet packet, ISession session, object obj)
        {
            if (packet.GetBytes().Length > 59)
            {
                var serverTime = packet.ReadUInt32(); // * 4   uint    ServerTime      //SROTimeStamp
                var refObjId = packet.ReadUInt32(); // 4   uint    RefObjID
                var scale = packet.ReadUInt8(); // 1   byte    Scale
                var curLevel = packet.ReadUInt8(); // 1   byte    CurLevel
                var maxLevel = packet.ReadUInt8(); // 1   byte    MaxLevel
                var expOffset = packet.ReadUInt64(); // 8   ulong   ExpOffset
                var sExpOffset = packet.ReadUInt32(); // 4   uint    SExpOffset
                var remainGold = packet.ReadUInt64(); // 8   ulong   RemainGold
                var remainSkillPoint = packet.ReadUInt32(); // 4   uint    RemainSkillPoint
                var remainStatPoint = packet.ReadUInt16(); // 2   ushort  RemainStatPoint
                var remainHwanCount = packet.ReadUInt8(); // 1   byte    RemainHwanCount
                var gatheredExpPoint = packet.ReadUInt32(); // 4   uint    GatheredExpPoint
                var hp = packet.ReadUInt32(); // 4   uint    HP
                var mp = packet.ReadUInt32(); // 4   uint    MP
                var autoInverstExp = packet.ReadUInt8(); // 1   byte    AutoInverstExp
                var dailyPk = packet.ReadUInt8(); // 1   byte    DailyPK
                var totalPk = packet.ReadUInt16(); // 2   ushort  TotalPK
                var pkPenaltyPoint = packet.ReadUInt32(); // 4   uint    PKPenaltyPoint
                var hwanLevel = packet.ReadUInt8(); // 1   byte    HwanLevel
                session.SessionData.State.PvpCape = (PVPCape)packet.ReadUInt8(); // 1   byte    FreePVP     //0 = None, 1 = Red, 2 = Gray, 3 = Blue, 4 = White, 5 = Gold
                session.SessionData.CharObjID = refObjId;
                session.SessionData.CurLevel = curLevel;
                session.SessionData.HwanLevel = hwanLevel;
                if (refObjId < 14875)
                {
                    // Char is chinese
                    session.SessionData.EUChar = false;
                    session.SessionData.CHChar = true;
                }
                else
                {
                    // Char is European
                    session.SessionData.CHChar = false;
                    session.SessionData.EUChar = true;
                }
                if (refObjId >= 1907 && refObjId <= 1919)
                {
                    session.SessionData.MaleChar = true;
                }
                else if (refObjId >= 14873 && refObjId <= 14887)
                {
                    session.SessionData.MaleChar = true;
                }
                else if (refObjId >= 1920 && refObjId <= 1932)
                {
                    session.SessionData.FemaleChar = true;
                }
                else if (refObjId >= 14888 && refObjId <= 14900)
                {
                    session.SessionData.FemaleChar = true;
                }
            }

            return Task.FromResult(new PacketResult());
        }
        private Task<PacketResult> SERVER_LEVELINFO(Packet packet, ISession session, object obj)
        {
            uint PlayerUniqueID = packet.ReadUInt32();
            if (PlayerUniqueID == session.SessionData.UniqueCharId)
            {
                session.SessionData.CurLevel += 1;
                //Log.Warning($"{CHARNAME16} has leveled up.", Utils.LOG_TYPE.Special);
            }
            return Task.FromResult(new PacketResult(packet, PacketResultType.Nothing));
        }

        private async Task<PacketResult> SERVER_USER_INFO(Packet packet, ISession session, object obj)
        {
            uint TOKEN_ID = packet.ReadUInt32();
            if (!string.IsNullOrWhiteSpace(session.PlayerUserID))
            {
                return new PacketResult(PacketResultType.Block);
            }

            if (QuickLoginAgentAuthBridge.TryConsumeAgentAuth(TOKEN_ID, session.ClientIp, out var quickLoginAuth))
            {
                var originalTail = TryReadAgentAuthTail(packet);
                var quickLoginPacket = new Packet(0x6103, true, false);
                quickLoginPacket.WriteUInt32(TOKEN_ID);
                quickLoginPacket.WriteAscii(quickLoginAuth.Username);
                quickLoginPacket.WriteAscii(quickLoginAuth.Password);
                quickLoginPacket.WriteUInt8(quickLoginAuth.Locale);
                if (originalTail.Length > 0)
                    quickLoginPacket.WriteUInt8Array(originalTail);

                session.PlayerUserID = quickLoginAuth.Username;
                session.SessionData.user_id = quickLoginAuth.Username;
                session.SessionData.user_pw = quickLoginAuth.Password;
                session.SessionData.locale = quickLoginAuth.Locale;

                await BotProtectionService.PopulateClientlessIdentityAsync(session, quickLoginAuth.Username);
                if (session.IsManagedClientless)
                {
                    if (await BotProtectionService.ShouldDisconnectBotSessionAsync(session))
                        return new PacketResult(PacketResultType.Disconnect);

                    session.SessionData.Hwid = "CLIENTLESS:" + quickLoginAuth.Username.ToLowerInvariant();
                }

                var quickLoginSecondaryPasswordData = await sqlQueryHelper.GetSecondaryPasswordData(session.PlayerUserID);
                if (quickLoginSecondaryPasswordData != null)
                {
                    session.SessionData.SecondPwRememberPC = quickLoginSecondaryPasswordData.RememberPC;
                }

                Log.Information("Secure quick login agent auth consumed for {Username} from {ClientIp}. TailBytes={TailBytes}",
                    quickLoginAuth.Username,
                    session.ClientIp,
                    originalTail.Length);
                return new PacketResult(quickLoginPacket, PacketResultType.Override);
            }

            string playerUserId;
            string password;
            byte locale;
            try
            {
                playerUserId = packet.ReadAscii();
                password = packet.ReadAscii();
                locale = packet.ReadUInt8();
            }
            catch (Exception ex) when (ex is EndOfStreamException or InvalidOperationException)
            {
                Log.Warning("Invalid Agent auth packet 0x6103 from {ClientIp}. Token={Token}, Length={Length}, Reason={Reason}",
                    session.ClientIp,
                    TOKEN_ID,
                    packet.GetBytes().Length,
                    ex.Message);
                return new PacketResult(PacketResultType.Disconnect);
            }

            session.PlayerUserID = playerUserId;
            session.SessionData.user_id = playerUserId;
            session.SessionData.user_pw = password;
            session.SessionData.locale = locale;
            await BotProtectionService.PopulateClientlessIdentityAsync(session, playerUserId);
            if (session.IsManagedClientless)
            {
                if (await BotProtectionService.ShouldDisconnectBotSessionAsync(session))
                    return new PacketResult(PacketResultType.Disconnect);

                session.SessionData.Hwid = "CLIENTLESS:" + playerUserId.ToLowerInvariant();
                Log.Information("Clientless HWID assigned for {Username} from {ClientIp}", playerUserId, session.ClientIp);
            }

            var secondaryPasswordData = await sqlQueryHelper.GetSecondaryPasswordData(session.PlayerUserID);
            if (secondaryPasswordData != null)
            {
                session.SessionData.SecondPwRememberPC = secondaryPasswordData.RememberPC;
            }

            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private static byte[] TryReadAgentAuthTail(Packet packet)
        {
            try
            {
                var reader = new Packet(packet);
                reader.ToReadOnly();
                reader.ReadUInt32();
                reader.ReadAscii();
                reader.ReadAscii();
                reader.ReadUInt8();

                var remaining = reader.RemainingRead();
                return remaining > 0 ? reader.ReadUInt8Array(remaining) : Array.Empty<byte>();
            }
            catch
            {
                return Array.Empty<byte>();
            }
        }
    }
}
