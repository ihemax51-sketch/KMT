using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Servers.PacketHandler
{
    public partial class GuildPackets
    {
        public GuildPackets(AgentServer agentServer, IPacketHandler packetHandler) 
        {
            packetHandler.RegisterClientHandler(0x705E, AGENT_SIEGE_ACTION); // SQL Injection - 0x705E - Also contains Tax / checkout checks - https://www.elitepvpers.com/forum/sro-private-server/4141360-information-sql-injection-ingame.html
            packetHandler.RegisterClientHandler(0x70F9, AGENT_GUILD_UPDATE_NOTICE); // Guild Notice - 0x70F9 - Better safe than sorry.
            packetHandler.RegisterClientHandler(0x70F3, AGENT_GUILD_INVITE);
            packetHandler.RegisterClientHandler(0x70FB, AGENT_UNION_INVITE);
        }
        private Task<PacketResult> AGENT_SIEGE_ACTION(Packet packet, ISession session, object obj)
        {
            packet.ReadUInt32();
            var unk2 = packet.ReadUInt8();
            uint unk3 = 0;
            if (unk2 == 1 || unk2 == 2 || unk2 == 26)
                unk3 = packet.ReadUInt32();

            // About guild

            if (unk3 == 3)
            {
                byte c = packet.ReadUInt8();
                byte d = packet.ReadUInt8();
                if (d == 0xFF)
                {
                    //SendMessageViaText(session, 3, "VFILTER_SPECIAL_CHARS_BLOCK_NOTICE");
                    return Task.FromResult(new PacketResult(packet, PacketResultType.Block));
                }
            }
            if (unk2 != 26 || unk3 != 1) return Task.FromResult(new PacketResult());
            var message = packet.ReadAscii();

            if (!message.Contains("\'") && !message.Contains("\"") && !message.Contains("-"))
                return Task.FromResult(new PacketResult());

            Log.Warning($"EXPLOIT - {0} tried to use FW_SQL_INJECTION - {1:X} {session.SessionData.Charname} {packet.Opcode}");
            //SendMessageViaText(session, 3, "VFILTER_SPECIAL_CHARS_BLOCK_NOTICE");
            return Task.FromResult(new PacketResult(packet, PacketResultType.Nothing));
        }
        private Task<PacketResult> AGENT_GUILD_UPDATE_NOTICE(Packet packet, ISession session, object obj)
        {
            var guildNoticeTitle = packet.ReadAscii();
            var guildNoticeMessage = packet.ReadAscii();

            if (!guildNoticeMessage.Contains('\'') &&
             !guildNoticeMessage.Contains('\"') &&
             !guildNoticeMessage.Contains('-') &&
             !guildNoticeTitle.Contains('\'') &&
             !guildNoticeTitle.Contains('\"') &&
             !guildNoticeTitle.Contains('-'))
                return Task.FromResult(new PacketResult());

            Log.Warning($"EXPLOIT - {session.SessionData.Charname} tried to use GUILD_SQL_INJECTION - {packet.Opcode:X}");
            //SendMessageViaText(session, 4, "LEXA_SPECIAL_CHARS_BLOCK_NOTICE");

            guildNoticeTitle = guildNoticeTitle
             .Replace('\'', ' ').Replace('-', ' ').Replace('\"', ' ').Replace(';', ' ');
            guildNoticeMessage = guildNoticeMessage
             .Replace('\'', ' ').Replace('-', ' ').Replace('\"', ' ')
             .Replace(';', ' ');

            var newPacket = new Packet(packet.Opcode, packet.Encrypted, packet.Massive);
            newPacket.WriteAscii(guildNoticeTitle);
            newPacket.WriteAscii(guildNoticeMessage);

            return Task.FromResult(new PacketResult(newPacket, PacketResultType.Override));
        }
        private async Task<PacketResult> AGENT_UNION_INVITE(Packet packet, ISession session, object obj)
        {
            try
            {

                int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_UNION_INVITE_TIME).TotalSeconds);
                if (gecensaniye < _serverSettings.UnionInviteDelay)
                {
                    int kalanSaniye = _serverSettings.UnionInviteDelay - gecensaniye;
                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_UNION_INVITE_DELAY"), kalanSaniye);
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
                session.SessionData.LAST_UNION_INVITE_TIME = DateTime.Now;

            }
            catch { }

            return new PacketResult();
        }
        private async Task<PacketResult> AGENT_GUILD_INVITE(Packet packet, ISession session, object obj)
        {
            try
            {

                int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_GUILD_INVITE_TIME).TotalSeconds);
                if (gecensaniye < _serverSettings.GuildInviteDelay)
                {
                    int kalanSaniye = _serverSettings.GuildInviteDelay - gecensaniye;
                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_GUILD_INVITE_DELAY"), kalanSaniye);
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
                session.SessionData.LAST_GUILD_INVITE_TIME = DateTime.Now;

            }
            catch { }

            return new PacketResult();

        }
    }
}
