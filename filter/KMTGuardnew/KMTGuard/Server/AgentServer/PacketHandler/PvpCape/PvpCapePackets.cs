using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.Localization;
using KMTGuard.SessionManager;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class PvpCapePackets
    {
        public PvpCapePackets(AgentServer agentServer, IPacketHandler packetHandler) 
        {
            packetHandler.RegisterClientHandler(0x7516, AGENT_FRPVP_UPDATE);
            packetHandler.RegisterModuleHandler(0xB516, SERVER_FRPVP_UPDATE);
        }
        private async Task<PacketResult> AGENT_FRPVP_UPDATE(Packet packet, ISession session, PacketData data)
        {
            var challengeBlock = await PvpChallengeService.BlockCapeChangeIfLockedAsync(session);
            if (challengeBlock != null)
                return challengeBlock;

            if (await RegionControlService.IsEventSuitCapeControlledAsync(session))
            {
                await RegionControlService.SendNoticeAsync(session, PlayerLanguage.Get("Region.EventSuitControlsCape"));
                return new PacketResult(PacketResultType.Block);
            }

            var regionRule = await RegionControlService.GetRuleAsync(session);
            if (regionRule != null && regionRule.Enable_AutoPvP >= 1 && regionRule.Enable_AutoPvP <= 5)
            {
                await RegionControlService.SendNoticeAsync(session, PlayerLanguage.Get("Region.PvpCapeControlled"));
                return new PacketResult(PacketResultType.Block);
            }
            //if (AgentServer.eventManager.defendTower.m_DefendTowerSetting.Count() > 0)
            //{
            //    if (session.SessionData.WorldID == AgentServer.eventManager.defendTower.m_DefendTowerSetting[0].WorldID && !AgentServer.eventManager.defendTower.DefendTowerEventStatus)
            //    {
            //        return new PacketResult(PacketResultType.Block);
            //    }
            //}
            return new PacketResult();
        }
        private Task<PacketResult> SERVER_FRPVP_UPDATE(Packet packet, ISession session, PacketData data)
        {
            var result = packet.ReadUInt8(); // 1   byte    result
            if (result == 1)
            {
                var uniqueId = packet.ReadUInt32(); // 4   uint    Player.UniqueID
                var cape = (PVPCape)packet.ReadUInt8(); // 1   byte    Player.FRPVPMode
                if (session.SessionData.UniqueCharId == uniqueId)
                {
                    session.SessionData.State.PvpCape = cape;
                }
            }
            else if (result == 2)
            {
                packet.ReadUInt16(); // 2   ushort  errorCode
            }

            return Task.FromResult(new PacketResult());
        }
    }
}
