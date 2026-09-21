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
using SilkroadSecurityAPI;

namespace KMTGuard.Servers.PacketHandler
{
    public partial class AcademyPackets
    {
        public AcademyPackets(AgentServer agentServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x7470, AGENT_ACADEMY_CREATE);
            packetHandler.RegisterClientHandler(0x7472, AGENT_ACADEMY_CREATE2);
            packetHandler.RegisterClientHandler(0x747E, AGENT_ACADEMY_MATCHING_JOIN);
        }
        private async Task<PacketResult> AGENT_ACADEMY_MATCHING_JOIN(Packet packet, ISession session, object obj)
        {
            try
            {
                if (_serverSettings.DisableAcademy)
                {

                    string noticeMessage = RefManager.GetNoticeMessage("MSG_ACADEMY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }
            catch { }

            return new PacketResult();
        }
        private async Task<PacketResult> AGENT_ACADEMY_CREATE2(Packet packet, ISession session, object obj)
        {
            try
            {
                if (_serverSettings.DisableAcademy)
                {

                    string noticeMessage = RefManager.GetNoticeMessage("MSG_ACADEMY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }
            catch { }

            return new PacketResult();
        }
        private async Task<PacketResult> AGENT_ACADEMY_CREATE(Packet packet, ISession session, object obj)
        {
            try
            {
                if (_serverSettings.DisableAcademy)
                {
                    string noticeMessage = RefManager.GetNoticeMessage("MSG_ACADEMY_DISABLED");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
            }
            catch { }

            return new PacketResult();
        }
    }
}