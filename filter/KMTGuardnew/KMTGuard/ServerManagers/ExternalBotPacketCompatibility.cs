using System.Text;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

internal static class ExternalBotPacketCompatibility
{
    internal const ushort CustomNoticeOpcode = 0x168A;
    internal const ushort NativeChatOpcode = 0x3026;
    private const byte NativeNoticeType = 7;

    internal static bool TryAdaptServerPacket(
        bool isExternalBot,
        Packet packet,
        out Packet? compatiblePacket)
    {
        ArgumentNullException.ThrowIfNull(packet);
        compatiblePacket = packet;

        if (!isExternalBot || packet.Opcode != CustomNoticeOpcode)
            return true;

        if (!TryReadCustomNotice(packet.GetBytes(), out var message))
        {
            compatiblePacket = null;
            return false;
        }

        compatiblePacket = CreateNativeNotice(message);
        return true;
    }

    internal static Packet CreateNativeNotice(string message)
    {
        var notice = new Packet(NativeChatOpcode, false, false);
        notice.WriteUInt8(NativeNoticeType);
        notice.WriteAscii(ToAsciiSafe(message));
        return notice;
    }

    private static bool TryReadCustomNotice(byte[] payload, out string message)
    {
        message = string.Empty;
        if (payload.Length < 3)
            return false;

        var characterCount = payload[1] | (payload[2] << 8);
        var byteCount = characterCount * 2;
        if (byteCount < 0 || payload.Length != 3 + byteCount)
            return false;

        message = Encoding.Unicode.GetString(payload, 3, byteCount);
        return true;
    }

    private static string ToAsciiSafe(string message)
    {
        var ascii = Encoding.GetEncoding(
            "us-ascii",
            new EncoderReplacementFallback("?"),
            new DecoderReplacementFallback("?"));
        return ascii.GetString(
            Encoding.Convert(Encoding.Unicode, ascii, Encoding.Unicode.GetBytes(message ?? string.Empty)));
    }
}
