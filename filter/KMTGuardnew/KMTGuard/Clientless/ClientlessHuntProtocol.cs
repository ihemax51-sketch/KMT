using System.Buffers.Binary;
using SilkroadSecurityAPI;

namespace KMTGuard.Clientless;

internal static class ClientlessHuntProtocol
{
    internal readonly record struct SkillCastFeedback(
        bool Accepted,
        bool OwnAction,
        ushort ErrorCode,
        uint SkillId,
        uint ActionId);

    internal readonly record struct BuffAppliedFeedback(
        uint TargetId,
        uint SkillId,
        uint Token);

    internal static Packet BuildSelectTarget(uint targetUniqueId)
    {
        var packet = new Packet(0x7045);
        packet.WriteUInt32(targetUniqueId);
        return packet;
    }

    internal static Packet BuildBasicAttack(uint targetUniqueId)
    {
        var packet = new Packet(0x7074);
        packet.WriteUInt8(1);
        packet.WriteUInt8(1);
        packet.WriteUInt8(1);
        packet.WriteUInt32(targetUniqueId);
        return packet;
    }

    internal static Packet BuildSkillAttack(uint skillId, uint targetUniqueId)
    {
        var packet = new Packet(0x7074);
        packet.WriteUInt8(1);
        packet.WriteUInt8(4);
        packet.WriteUInt32(skillId);
        packet.WriteUInt8(1);
        packet.WriteUInt32(targetUniqueId);
        return packet;
    }

    internal static Packet BuildSelfBuff(uint skillId, uint selfUniqueId, bool targetsSelf)
    {
        var packet = new Packet(0x7074);
        packet.WriteUInt8(1);
        packet.WriteUInt8(4);
        packet.WriteUInt32(skillId);
        if (targetsSelf)
        {
            packet.WriteUInt8(1);
            packet.WriteUInt32(selfUniqueId);
        }
        else
        {
            packet.WriteUInt8(0);
        }
        return packet;
    }

    internal static Packet BuildUseItem(byte slot, ushort typeId)
    {
        // vSRO requires inventory-use requests to pass through the encrypted
        // Agent packet path. An unencrypted 0x704C is rejected by closing the
        // managed Clientless connection.
        var packet = new Packet(0x704C, true, false);
        packet.WriteUInt8(slot);
        packet.WriteUInt16(typeId);
        return packet;
    }

    internal static Packet BuildUseItemFor(byte slot, ushort typeId, uint targetUniqueId)
    {
        var packet = BuildUseItem(slot, typeId);
        packet.WriteUInt32(targetUniqueId);
        return packet;
    }

    internal static Packet BuildUseItemForSlot(byte slot, ushort typeId, byte targetSlot)
    {
        var packet = BuildUseItem(slot, typeId);
        packet.WriteUInt8(targetSlot);
        return packet;
    }

    internal static Packet BuildCosAttack(uint cosUniqueId, uint targetUniqueId)
    {
        var packet = new Packet(0x70C5);
        packet.WriteUInt32(cosUniqueId);
        packet.WriteUInt8(2);
        packet.WriteUInt32(targetUniqueId);
        return packet;
    }

    internal static Packet BuildRespawnInTown()
    {
        var packet = new Packet(0x3053);
        packet.WriteUInt8(1);
        return packet;
    }

    internal static Packet BuildMove(int regionId, float x, float y, float z)
    {
        var packet = new Packet(0x7021);
        packet.WriteUInt8(1);
        packet.WriteUInt16(unchecked((ushort)regionId));
        if ((regionId & 0x8000) == 0)
        {
            packet.WriteInt16(checked((short)Math.Clamp(MathF.Round(x), short.MinValue, short.MaxValue)));
            packet.WriteInt16(checked((short)Math.Clamp(MathF.Round(z), short.MinValue, short.MaxValue)));
            packet.WriteInt16(checked((short)Math.Clamp(MathF.Round(y), short.MinValue, short.MaxValue)));
        }
        else
        {
            packet.WriteInt32(Convert.ToInt32(MathF.Round(x)));
            packet.WriteInt32(Convert.ToInt32(MathF.Round(z)));
            packet.WriteInt32(Convert.ToInt32(MathF.Round(y)));
        }
        return packet;
    }

    internal static bool TryReadSkillCastFeedback(
        Packet packet,
        uint selfUniqueId,
        out SkillCastFeedback feedback)
    {
        feedback = default;
        var bytes = packet.GetBytes();
        if (bytes.Length == 0)
            return false;

        if (bytes[0] != 1)
        {
            // vSRO B070 failures contain a one-byte error code. The following
            // byte, when present, belongs to the client-specific action
            // extension and must not be combined with the error code. Reading
            // both bytes as UInt16 turns real errors such as 0x05/0x06 into
            // artificial 0x3005/0x3006 values and breaks retry decisions.
            feedback = new SkillCastFeedback(
                Accepted: false,
                OwnAction: true,
                ErrorCode: bytes.Length >= 2 ? bytes[1] : (ushort)0,
                SkillId: 0,
                ActionId: 0);
            return true;
        }

        // vSRO B070 success: result byte, ushort action extension, skill ID,
        // executor unique ID, action ID, target unique ID and action flag.
        const int skillOffset = 3;
        const int executorOffset = skillOffset + sizeof(uint);
        const int actionOffset = executorOffset + sizeof(uint);
        if (bytes.Length < actionOffset + sizeof(uint))
            return false;

        var executorId = BinaryPrimitives.ReadUInt32LittleEndian(
            bytes.AsSpan(executorOffset, sizeof(uint)));
        if (executorId == selfUniqueId)
        {
            feedback = new SkillCastFeedback(
                Accepted: true,
                OwnAction: true,
                ErrorCode: 0,
                SkillId: BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(skillOffset, sizeof(uint))),
                ActionId: BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(actionOffset, sizeof(uint))));
            return true;
        }

        feedback = new SkillCastFeedback(
            Accepted: true,
            OwnAction: false,
            ErrorCode: 0,
            SkillId: 0,
            ActionId: 0);
        return true;
    }

    internal static bool TryReadBuffApplied(Packet packet, out BuffAppliedFeedback feedback)
    {
        feedback = default;
        var bytes = packet.GetBytes();
        if (bytes.Length < sizeof(uint) * 3)
            return false;

        feedback = new BuffAppliedFeedback(
            BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0, sizeof(uint))),
            BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4, sizeof(uint))),
            BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(8, sizeof(uint))));
        return true;
    }

    internal static bool TryReadRemovedBuffTokens(Packet packet, out IReadOnlyList<uint> tokens)
    {
        tokens = Array.Empty<uint>();
        var bytes = packet.GetBytes();
        if (bytes.Length == 0)
            return false;

        var count = bytes[0];
        if (bytes.Length < 1 + (count * sizeof(uint)))
            return false;

        var result = new uint[count];
        for (var index = 0; index < count; index++)
            result[index] = BinaryPrimitives.ReadUInt32LittleEndian(
                bytes.AsSpan(1 + (index * sizeof(uint)), sizeof(uint)));
        tokens = result;
        return true;
    }

}
