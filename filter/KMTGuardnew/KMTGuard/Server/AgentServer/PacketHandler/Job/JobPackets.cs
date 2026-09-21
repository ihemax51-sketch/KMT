using System.Threading.Tasks;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.AgentPacketHandler;

public sealed class JobPackets
{
    public JobPackets(IPacketHandler packetHandler)
    {
        packetHandler.RegisterClientHandler(0x70E1, JobJoinAsync);
        packetHandler.RegisterClientHandler(0x70E2, JobLeaveAsync);
    }

    private static async Task<PacketResult> JobJoinAsync(
        Packet packet,
        ISession session,
        object data)
    {
        // AGENT_JOB_JOIN: NPC unique ID (Int32), followed by Job type (UInt8).
        if (packet.GetBytes().Length < 5)
            return new PacketResult(PacketResultType.Disconnect);

        packet.ReadInt32();
        byte jobType = packet.ReadUInt8();

        bool allowed = await JobControlService.CanJoinAsync(session, jobType);
        return allowed
            ? new PacketResult(PacketResultType.Nothing)
            : new PacketResult(PacketResultType.Block);
    }

    private static async Task<PacketResult> JobLeaveAsync(
        Packet packet,
        ISession session,
        object data)
    {
        bool allowed = await JobControlService.CanLeaveAsync(session);
        return allowed
            ? new PacketResult(PacketResultType.Nothing)
            : new PacketResult(PacketResultType.Block);
    }
}
