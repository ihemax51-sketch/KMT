using System.Security.Cryptography;
using KMTGuard.Database.Models;
using KMTGuard.SessionManager;
using KMTGuard.Localization;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public static class TradeSellCaptchaService
{
    private const ushort CaptchaRequestOpcode = 0x2071;

    public static async Task RequestAsync(
        ISession session,
        int petUniqueId,
        byte petSlot,
        short count,
        int npcUniqueId)
    {
        var pending = new PendingTradeSellCaptcha
        {
            PetUniqueId = petUniqueId,
            PetSlot = petSlot,
            Count = count,
            NpcUniqueId = npcUniqueId
        };

        session.SessionData.PendingTradeSellCaptcha = pending;
        await RefreshChallengeAsync(session, pending);
    }

    public static async Task HandleResponseAsync(Packet packet, ISession session)
    {
        int submittedCode = packet.ReadInt32();
        var pending = session.SessionData.PendingTradeSellCaptcha;
        if (pending == null)
        {
            await session.SendNotice(PlayerLanguage.Get("TradeVerification.NoPending"));
            return;
        }

        if (DateTime.UtcNow > pending.ExpiresAtUtc)
        {
            session.SessionData.PendingTradeSellCaptcha = null;
            await session.SendNotice(PlayerLanguage.Get("TradeVerification.Expired"));
            return;
        }

        if (submittedCode != pending.Code)
        {
            pending.Attempts++;
            if (pending.Attempts >= MaxAttempts)
            {
                session.SessionData.PendingTradeSellCaptcha = null;
                await session.SendNotice(PlayerLanguage.Get("TradeVerification.Failed"));
                return;
            }

            await session.SendNotice(PlayerLanguage.Get("TradeVerification.WrongCode"));
            await RefreshChallengeAsync(session, pending);
            return;
        }

        session.SessionData.PendingTradeSellCaptcha = null;

        var sellPacket = new Packet(0x7034, true, false);
        sellPacket.WriteUInt8(0x14);
        sellPacket.WriteInt32(pending.PetUniqueId);
        sellPacket.WriteUInt8(pending.PetSlot);
        sellPacket.WriteUInt16((ushort)pending.Count);
        sellPacket.WriteInt32(pending.NpcUniqueId);

        await session.SendToServer(sellPacket);
        Log.Information(
            "Trade sell captcha passed for {CharName} pet {PetUniqueId} slot {PetSlot}",
            session.SessionData.Charname,
            pending.PetUniqueId,
            pending.PetSlot);
    }

    private static async Task RefreshChallengeAsync(ISession session, PendingTradeSellCaptcha pending)
    {
        pending.Code = RandomNumberGenerator.GetInt32(1000, 10000);
        pending.ExpiresAtUtc = DateTime.UtcNow.AddSeconds(TimeoutSeconds);

        var request = new Packet(CaptchaRequestOpcode);
        request.WriteInt32(pending.Code);
        request.WriteInt32(TimeoutSeconds);
        request.WriteInt32(Math.Max(0, MaxAttempts - pending.Attempts));
        request.WriteUInt8(pending.Attempts > 0 ? (byte)1 : (byte)0);
        await session.SendToClient(request);
    }

    private static int TimeoutSeconds
    {
        get
        {
            int configured = _serverSettings.TradeSellCaptchaTimeoutSeconds;
            return Math.Clamp(configured <= 0 ? 60 : configured, 10, 300);
        }
    }

    private static int MaxAttempts
    {
        get
        {
            int configured = _serverSettings.TradeSellCaptchaMaxAttempts;
            return Math.Clamp(configured <= 0 ? 3 : configured, 1, 10);
        }
    }
}
