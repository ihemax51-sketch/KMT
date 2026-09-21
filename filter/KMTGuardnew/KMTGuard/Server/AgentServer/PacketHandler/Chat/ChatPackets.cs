using KMTGuard.ChatFiltering; // ✅ Added: ChatWordFilter
using KMTGuard.Database; // ChatLogger
using KMTGuard.Database.Models;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;
using SilkroadSecurityAPI;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class ChatPackets
    {
        public ChatPackets(AgentServer agentServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x7025, HandleChatReq);           // Chat
            packetHandler.RegisterClientHandler(0x705C, GLOBAL_ITEM_LINK_CLIENT); // Forward
            packetHandler.RegisterModuleHandler(0x5033, GLOBAL_ITEM_LINK);        // Global
        }

        // ===== DB chat codes (زي ما هي عندك) =====
        private static class ChatDbCodes
        {
            public const byte All = 1;
            public const byte Party = 2;
            public const byte Guild = 3;
            public const byte Union = 4;
            public const byte Global = 5; // الجلوبال = 5
            public const byte Whisper = 6;
            public const byte Academy = 7;
            public const byte Market = 8;
        }

        // ===== Mapping (نفس الترقيم اللي فوق) =====
        private static byte MapChatTypeToDb(byte chatType)
        {
            switch (chatType)
            {
                case 0x01: return ChatDbCodes.All;
                case 0x02: return ChatDbCodes.Whisper;
                case 0x03: return ChatDbCodes.Party;
                case 0x04: return ChatDbCodes.Guild;
                case 0x05: return ChatDbCodes.Union;
                case 0x09: return ChatDbCodes.Market;
                // case 0x11: return ChatDbCodes.Academy;
                default: return ChatDbCodes.All;
            }
        }

        private struct PacketCursor
        {
            public readonly byte[] Data;
            public int Index;

            public PacketCursor(byte[] data) { Data = data ?? Array.Empty<byte>(); Index = 0; }
            public int Remaining => Data.Length - Index;

            public bool TryReadByte(out byte b)
            { b = 0; if (Remaining < 1) return false; b = Data[Index++]; return true; }

            public bool TryReadUShortLE(out ushort v)
            { v = 0; if (Remaining < 2) return false; v = (ushort)(Data[Index] | (Data[Index + 1] << 8)); Index += 2; return true; }

            public bool TryReadBytes(int n, out byte[] chunk)
            {
                chunk = Array.Empty<byte>();
                if (Remaining < n) return false;
                chunk = new byte[n];
                Buffer.BlockCopy(Data, Index, chunk, 0, n);
                Index += n;
                return true;
            }

            public bool TryReadSroString(out string s)
            {
                s = string.Empty;
                int saved = Index;
                if (!TryReadUShortLE(out ushort len)) { Index = saved; return false; }

                if (Remaining >= len * 2)
                {
                    if (!TryReadBytes(len * 2, out var bytes)) { Index = saved; return false; }
                    s = ToAsciiSafe(Encoding.Unicode.GetString(bytes));
                    return true;
                }
                if (Remaining >= len)
                {
                    if (!TryReadBytes(len, out var bytes)) { Index = saved; return false; }
                    s = Encoding.ASCII.GetString(bytes);
                    return true;
                }
                Index = saved;
                return false;
            }
        }

        private static string ToAsciiSafe(string input)
        {
            var enc = Encoding.GetEncoding(
                "us-ascii",
                new EncoderReplacementFallback("?"),
                new DecoderReplacementFallback("?"));
            return enc.GetString(
                Encoding.Convert(Encoding.Unicode, enc, Encoding.Unicode.GetBytes(input ?? string.Empty)));
        }

        private static string HexDumpSafe(Packet p)
        {
            try { return BitConverter.ToString(p.GetBytes()); }
            catch { return "<hex-unavailable>"; }
        }

        // ================= 0x7025 =================
        private async Task<PacketResult> HandleChatReq(Packet packet, ISession session, object obj)
        {
            try
            {
                var raw = packet.GetBytes();
                var cur = new PacketCursor(raw);

                if (!cur.TryReadByte(out byte chatType))
                    return new PacketResult();

                if (!cur.TryReadByte(out byte _))
                    return new PacketResult();

                string receiver = string.Empty;
                string message = string.Empty;

                if (chatType == 0x02) // Whisper
                {
                    if (!cur.TryReadSroString(out receiver) ||
                        !cur.TryReadSroString(out message))
                        return new PacketResult();
                }
                else
                {
                    if (!cur.TryReadSroString(out message))
                        return new PacketResult();
                }

                var chatBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Chat,
                    "Region.ChatDisabled",
                    useNativeNotice: true);
                if (chatBlock != null)
                    return chatBlock;

                // ✅ Block prohibited words BEFORE logging/sending
                if (await ChatWordFilter.IsBlockedAsync(message))
                {
                    await session.SendNotice(PlayerLanguage.Get("Chat.ProhibitedLanguage"));
                  


                    return new PacketResult(PacketResultType.Block);
                }

                byte dbType = MapChatTypeToDb(chatType);

                await AutoEventService.HandleChatAnswerAsync(session, chatType, message);

                await ChatLogger.LogAsync(
                    sender: session?.SessionData?.Charname ?? "UNKNOWN",
                    receiver: receiver,
                    chatType: dbType,
                    message: message
                );

                if (chatType == 0x09) // Market tab uses the client stall-chat type.
                {
                    var market = new Packet(0x3026, false, false);
                    market.WriteUInt8(0x09);
                    market.WriteAscii(ToAsciiSafe(session?.SessionData?.Charname ?? "UNKNOWN"));
                    market.WriteAscii(ToAsciiSafe(message));
                    await ServerManager.BroadcastPacket(market);
                    return new PacketResult(PacketResultType.Block);
                }

                return new PacketResult();
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "[HandleChatReq] error");
                return new PacketResult();
            }
        }

        // ================= 0x705C =================
        private async Task<PacketResult> GLOBAL_ITEM_LINK_CLIENT(Packet packet, ISession session, object obj)
        {
            try
            {
                byte globalType = packet.ReadUInt8();
                byte globalSlot = packet.ReadUInt8();
                ushort globalItemType = packet.ReadUInt16();
                int globalItemId = packet.ReadInt32();
                string message = packet.ReadUnicode();

                var globalBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Global,
                    "Region.GlobalChatDisabled",
                    useNativeNotice: true);
                if (globalBlock != null)
                    return globalBlock;

                if (await ChatWordFilter.IsBlockedAsync(message))
                {
                    await session.SendNotice(PlayerLanguage.Get("Chat.ProhibitedLanguage"));
                    return new PacketResult(PacketResultType.Block);
                }

                return new PacketResult(PacketResultType.Nothing);
            }
            catch
            {
                return new PacketResult(PacketResultType.Nothing);
            }
        }

        // ================= 0x5033 =================
        private async Task<PacketResult> GLOBAL_ITEM_LINK(Packet packet, ISession session, object obj)
        {
            try
            {
                byte GlobalType = packet.ReadUInt8();
                byte GlobalSlot = packet.ReadUInt8();
                ushort GlobalItemType = packet.ReadUInt16();
                int GlobalItemID = packet.ReadInt32();
                string Message = packet.ReadUnicode();

                var globalBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Global,
                    "Region.GlobalChatDisabled");
                if (globalBlock != null)
                    return new PacketResult(packet, PacketResultType.Block);

                var reverse = new Packet(0x704C, true, false);
                reverse.WriteUInt8(GlobalSlot);
                reverse.WriteUInt16(GlobalItemType);
                reverse.WriteAscii(ToAsciiSafe(Message));
                await session.SendToServer(reverse);

                await ChatLogger.LogAsync(
                    sender: session?.SessionData?.Charname ?? "SERVER",
                    receiver: null,
                    chatType: ChatDbCodes.Global, // = 5
                    message: ToAsciiSafe(Message)
                );

                if (session != null)
                    AutoEventService.RegisterGlobalSent(session, Message);

                string RGBColor = "#FFFF00";
                if (RefManager.RefGlobalColor.ContainsKey(GlobalItemID))
                    RGBColor = RefManager.RefGlobalColor[GlobalItemID];

                int argbInputColor = Int32.Parse(RGBColor.Replace("#", ""), NumberStyles.HexNumber);

                Packet itemlink = new Packet(0x179B);
                itemlink.WriteUInt8(GlobalType);
                itemlink.WriteUnicode((session?.SessionData?.Charname ?? "Unknown") + ":" + Message);
                itemlink.WriteInt32(argbInputColor);

                if (GlobalType == 1)
                {
                    if (packet.RemainingRead() < 8)
                    {
                        Log.Warning("GLOBAL_ITEM_LINK missing linked item payload for {Char}", session?.SessionData?.Charname);
                        return new PacketResult(packet, PacketResultType.Block);
                    }

                    int len = packet.ReadInt32();
                    if (len < 0 || len > packet.RemainingRead() - sizeof(int))
                    {
                        Log.Warning("GLOBAL_ITEM_LINK invalid linked item payload length {Length} for {Char}", len, session?.SessionData?.Charname);
                        return new PacketResult(packet, PacketResultType.Block);
                    }

                    itemlink.WriteInt32(len);
                    foreach (byte value in packet.ReadUInt8Array(len))
                        itemlink.WriteUInt8(value);

                    itemlink.WriteInt32(packet.ReadInt32());
                }

                await ServerManager.BroadcastPacket(itemlink);

                return new PacketResult(packet, PacketResultType.Block);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "GLOBAL_ITEM_LINK");
                return new PacketResult(packet, PacketResultType.Block);
            }
        }
    }
}
