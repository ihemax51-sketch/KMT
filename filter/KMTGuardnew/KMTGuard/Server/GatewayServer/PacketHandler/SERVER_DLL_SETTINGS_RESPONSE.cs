using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Reflection.PortableExecutable;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.IdentityModel.Tokens;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.GatewayPacketHandler
{
    public partial class SERVER_DLL_SETTINGS_RESPONSE
    {
        private const int PrimaryLoginProofGraceMilliseconds = 10_000;

        private static string BrandHwidNotice(string noticeMessage)
        {
            if (string.IsNullOrWhiteSpace(noticeMessage))
                return PlayerLanguage.Get("SystemNotices.HWID_SUCCES");

            return noticeMessage
                .Replace("JTGuard", "KMTGuard", StringComparison.OrdinalIgnoreCase)
                .Replace("jtguard", "KMTGuard", StringComparison.OrdinalIgnoreCase);
        }

        private static string GetConfiguredServerName()
        {
            return string.IsNullOrWhiteSpace(_serverSettings.ServerName)
                ? "KMTGuard"
                : _serverSettings.ServerName.Trim();
        }

        internal static byte GetConfiguredShardStatus(bool checkStatus)
        {
            return checkStatus ? (byte)0 : (byte)1;
        }

        internal static bool ShouldPresentShardAsCheck(bool checkStatus, bool isGmIp)
        {
            return checkStatus && !isGmIp;
        }

        internal static bool CanUseManagedClientlessLogin(
            bool isManagedClientless,
            string clientIp,
            string mainMachineIp,
            IEnumerable<IPAddress>? localAddresses = null)
        {
            if (!isManagedClientless || !IPAddress.TryParse(clientIp, out var candidate))
                return false;

            candidate = NormalizeAddress(candidate);
            if (IPAddress.IsLoopback(candidate))
                return true;

            if (IPAddress.TryParse(mainMachineIp, out var configuredAddress) &&
                candidate.Equals(NormalizeAddress(configuredAddress)))
            {
                return true;
            }

            try
            {
                localAddresses ??= NetworkInterface.GetAllNetworkInterfaces()
                    .SelectMany(networkInterface => networkInterface.GetIPProperties().UnicastAddresses)
                    .Select(unicast => unicast.Address)
                    .ToArray();

                return localAddresses.Any(address =>
                    candidate.Equals(NormalizeAddress(address)));
            }
            catch (NetworkInformationException)
            {
                return false;
            }
        }

        private static IPAddress NormalizeAddress(IPAddress address)
        {
            return address.IsIPv4MappedToIPv6 ? address.MapToIPv4() : address;
        }

        public SERVER_DLL_SETTINGS_RESPONSE(GatewayServer gatewayServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x165B, CLIENT_HWID_REQUEST);
            packetHandler.RegisterClientHandler(0x1211, CLIENT_SECONDARY_PASSWORD_REQUEST);
            packetHandler.RegisterClientHandler(0x166A, CLIENT_ACCOUNT_REGISTER_REQUEST);
            packetHandler.RegisterClientHandler(0x1670, CLIENT_CREATE_QUICK_LOGIN_TOKEN);
            packetHandler.RegisterClientHandler(0x1672, CLIENT_QUICK_LOGIN);
            packetHandler.RegisterClientHandler(0x1674, CLIENT_REVOKE_QUICK_LOGIN_TOKEN);
            packetHandler.RegisterClientHandler(0x6102, CLIENT_LOGIN_REQUEST);
            packetHandler.RegisterClientHandler(0xA150, SERVER_SENDDLLSETTINGS);
            packetHandler.RegisterModuleHandler(0xA101, SERVER_SHARD_INFO); // Replaces the shard info
            

        }
        private async Task<PacketResult> CLIENT_HWID_REQUEST(Packet packet, ISession session, object obj)
        {
            var isQuickLoginNonceRefresh = TakeQuickLoginNonceRefreshMarker(session);
            try
            {
                string response = packet.ReadAscii();
                if(!HwidSecurity.TryValidateResponse(session, response, out string Hwid))
                {
                    await SendSecurityMessageAsync(session, "Security.ClientUpdateRequired");
                    return new PacketResult(PacketResultType.Disconnect);
                }
                else
                {
                    session.SessionData.Hwid = Hwid;
                    session.GatewayAuthenticationState = GatewayAuthenticationState.AwaitingPrimaryCredentials;
                    var resumePrimaryLogin = session.PendingPrimaryLogin;
                    var pendingElapsedMilliseconds = resumePrimaryLogin
                        ? Math.Max(0, Environment.TickCount64 - session.PendingPrimaryLoginStartedAt)
                        : 0;
                    session.PendingPrimaryLogin = false;
                    session.PendingPrimaryLoginStartedAt = 0;
                    if (!isQuickLoginNonceRefresh)
                    {
                        string noticeMessage = BrandHwidNotice(RefManager.GetNoticeMessage("HWID_SUCCES"));
                        Packet pck = new Packet(0xA340);
                        pck.WriteUnicode(noticeMessage);
                        await session.SendToClient(pck);
                    }

                    if (resumePrimaryLogin)
                    {
                        Log.Information(
                            "Gateway client proof accepted after {ElapsedMilliseconds} ms; resuming deferred primary login for {ClientIp}",
                            pendingElapsedMilliseconds,
                            session.ClientIp);
                        return await ProcessPrimaryLoginAsync(session, replayNativeLogin: true);
                    }

                    return new PacketResult(PacketResultType.Block);
                }
            }
            catch (Exception EX)
            {
                Log.Warning(EX, "Gateway HWID v2 verification failed for {ClientIp}", session.ClientIp);
                await SendSecurityMessageAsync(session, "Security.HwidVerificationFailed");
                return new PacketResult(PacketResultType.Disconnect);
            }
        }

        private async Task<PacketResult> CLIENT_SECONDARY_PASSWORD_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                var protocolVersion = packet.ReadUInt8();
                var requestType = packet.ReadUInt8();
                if (protocolVersion != 2 || string.IsNullOrEmpty(session.SessionData.Hwid) ||
                    string.IsNullOrWhiteSpace(session.DeviceKeyThumbprint) ||
                    string.IsNullOrWhiteSpace(session.SessionData.user_id))
                {
                    await SendSecurityMessageAsync(session, "Security.ClientUpdateRequired");
                    return new PacketResult(PacketResultType.Disconnect);
                }

                var blockedUntil = await sqlQueryHelper.GetSecondaryPasswordBlockAsync(
                    session.SessionData.user_id, session.ClientIp, session.DeviceKeyThumbprint);
                if (blockedUntil.HasValue)
                {
                    await SendSecurityMessageAsync(session, "Security.SecondaryTemporarilyLocked");
                    return new PacketResult(PacketResultType.Block);
                }

                if (requestType == (byte)SecondaryPasswordRequest.CREATE_PASSWORD)
                {
                    if (session.GatewayAuthenticationState != GatewayAuthenticationState.AwaitingSecondaryCreate)
                        return new PacketResult(PacketResultType.Disconnect);

                    var selectedPassword = packet.ReadAscii();
                    if (!IsValidSecondaryPassword(selectedPassword))
                    {
                        await SendSecurityMessageAsync(session, "Security.SecondaryInvalidFormat");
                        return new PacketResult(PacketResultType.Block);
                    }

                    var secondaryPasswordData =
                        await sqlQueryHelper.GetSecondaryPasswordData(session.SessionData.user_id);
                    if (secondaryPasswordData == null &&
                        await sqlQueryHelper.InsertSecondaryPasswordData(
                            session.SessionData.user_id, selectedPassword, session.SessionData.Hwid,
                            session.DeviceKeyThumbprint, false))
                    {
                        session.GatewayAuthenticationState = GatewayAuthenticationState.AwaitingSecondaryEntry;
                        Packet pck = new Packet(0x1212);
                        pck.WriteUInt8(SecondaryPasswordResponse.CREATED_SUCCESS_ENTER_THE_PASSWORD);
                        await session.SendToClient(pck);
                        return new PacketResult(PacketResultType.Block);
                    }

                    return new PacketResult(PacketResultType.Disconnect);
                }
                else if (requestType == (byte)SecondaryPasswordRequest.CHANGE_PASSWORD)
                {
                    if (session.GatewayAuthenticationState != GatewayAuthenticationState.AwaitingSecondaryEntry)
                        return new PacketResult(PacketResultType.Disconnect);

                    var selectedPassword = packet.ReadAscii();
                    var targetPassword = packet.ReadAscii();
                    if (!IsValidSecondaryPassword(selectedPassword) ||
                        !IsValidSecondaryPassword(targetPassword))
                    {
                        await SendSecurityMessageAsync(session, "Security.SecondaryInvalidFormat");
                        return new PacketResult(PacketResultType.Block);
                    }

                    if (await sqlQueryHelper.VerifySecondaryPasswordAsync(
                            session.SessionData.user_id, selectedPassword))
                    {
                        if (!await sqlQueryHelper.UpdateSecondaryPasswordData(
                                session.SessionData.user_id, targetPassword, false,
                                session.SessionData.Hwid, session.DeviceKeyThumbprint))
                            return new PacketResult(PacketResultType.Disconnect);

                        await sqlQueryHelper.ClearSecondaryPasswordFailuresAsync(
                            session.SessionData.user_id, session.ClientIp, session.DeviceKeyThumbprint);
                        Packet pck = new Packet(0x1212);
                        pck.WriteUInt8(SecondaryPasswordResponse.PASSWORD_CHANGED_ENTER_THE_PASSWORD);
                        await session.SendToClient(pck);
                        return new PacketResult(PacketResultType.Block);
                    }
                    else
                    {
                        return await HandleWrongSecondaryPasswordAsync(session);
                    }
                }
                else if (requestType == (byte)SecondaryPasswordRequest.ENTER_THE_PASSWORD)
                {
                    if (session.GatewayAuthenticationState != GatewayAuthenticationState.AwaitingSecondaryEntry)
                        return new PacketResult(PacketResultType.Disconnect);

                    var selectedPassword = packet.ReadAscii();
                    var rememberPc = packet.ReadBool();
                    if (!IsValidSecondaryPassword(selectedPassword))
                    {
                        await SendSecurityMessageAsync(session, "Security.SecondaryInvalidFormat");
                        return new PacketResult(PacketResultType.Block);
                    }

                    if (await sqlQueryHelper.VerifySecondaryPasswordAsync(
                            session.SessionData.user_id, selectedPassword))
                    {
                        if (!await sqlQueryHelper.UpdateSecondaryPasswordRememberPcAsync(
                                session.SessionData.user_id, rememberPc, session.SessionData.Hwid,
                                session.DeviceKeyThumbprint))
                            return new PacketResult(PacketResultType.Disconnect);
                        await sqlQueryHelper.ClearSecondaryPasswordFailuresAsync(
                            session.SessionData.user_id, session.ClientIp, session.DeviceKeyThumbprint);
                        session.GatewayAuthenticationState = GatewayAuthenticationState.Released;
                        Packet pck = new Packet(0x1212);
                        pck.WriteUInt8(SecondaryPasswordResponse.PW_TRUE);
                        await session.SendToClient(pck);

                        if (session.PendingQuickLogin)
                        {
                            QuickLoginAgentAuthBridge.BeginGatewayLogin(
                                session,
                                session.SessionData.user_id,
                                session.SessionData.user_pw,
                                session.SessionData.locale);
                            session.PendingQuickLogin = false;
                            await SendQuickLoginResult(
                                session, true, PlayerLanguage.Get("QuickLogin.Accepted"));
                        }


                        if (!await OfflineStallGatewayBridge.TryReplaceOfflineStallLoginAsync(
                                session,
                                session.SessionData.user_id,
                                session.SessionData.user_pw,
                                () => ReplayNativeLoginAsync(session),
                                credentialsAlreadyValidated: true))
                        {
                            await ReplayNativeLoginAsync(session);
                        }


                        return new PacketResult(PacketResultType.Block);
                    }
                    else
                    {
                        return await HandleWrongSecondaryPasswordAsync(session);
                    }
                }
                return new PacketResult(PacketResultType.Disconnect);
            }
            catch (Exception EX)
            {
                Log.Warning(EX, "Secondary-password security operation failed for {ClientIp}", session.ClientIp);
                await SendSecurityMessageAsync(session, "Security.AuthenticationUnavailable");
                return new PacketResult(PacketResultType.Disconnect);
            }

        }

        internal static bool IsValidSecondaryPassword(string password)
        {
            return password.Length is >= 6 and <= 8 && password.All(char.IsAsciiDigit);
        }

        private static async Task<PacketResult> HandleWrongSecondaryPasswordAsync(ISession session)
        {
            var blockedUntil = await sqlQueryHelper.RegisterSecondaryPasswordFailureAsync(
                session.SessionData.user_id, session.ClientIp, session.DeviceKeyThumbprint);
            if (blockedUntil.HasValue)
                await SendSecurityMessageAsync(session, "Security.SecondaryTemporarilyLocked");

            Packet pck = new Packet(0x1212);
            pck.WriteUInt8(SecondaryPasswordResponse.WRONG_PASSWORD);
            await session.SendToClient(pck);
            return new PacketResult(PacketResultType.Block);
        }

        private static async Task SendSecurityMessageAsync(ISession session, string languageKey)
        {
            Packet packet = new Packet(0xA340);
            packet.WriteUnicode(PlayerLanguage.Get(languageKey));
            await session.SendToClient(packet);
        }
        private async Task<PacketResult> CLIENT_LOGIN_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                byte locale = packet.ReadUInt8();
                string user_id = packet.ReadAscii().ToLower();
                string user_pw = packet.ReadAscii();
                ushort ServerID = packet.ReadUInt16();

                session.SessionData.locale = locale;
                session.SessionData.user_id = user_id;
                session.SessionData.user_pw = user_pw;
                session.SessionData.ServerID = ServerID;

                return await ProcessPrimaryLoginAsync(session, replayNativeLogin: false);
            }
            catch (Exception EX)
            {
                Log.Warning(EX, "Gateway login security failed for {ClientIp}", session.ClientIp);
                await SendSecurityMessageAsync(session, "Security.AuthenticationUnavailable");
                return new PacketResult(PacketResultType.Disconnect);
            }
        }

        private async Task<PacketResult> ProcessPrimaryLoginAsync(
            ISession session,
            bool replayNativeLogin)
        {
            try
            {
                var user_id = session.SessionData.user_id;
                var user_pw = session.SessionData.user_pw;

                await BotProtectionService.PopulateClientlessIdentityAsync(session, user_id);
                var useManagedClientlessLogin = CanUseManagedClientlessLogin(
                    session.IsManagedClientless,
                    session.ClientIp,
                    Program.MainMachineIP);

                if (!useManagedClientlessLogin &&
                    (session.GatewayAuthenticationState !=
                        GatewayAuthenticationState.AwaitingPrimaryCredentials ||
                     string.IsNullOrEmpty(session.SessionData.Hwid) ||
                     string.IsNullOrWhiteSpace(session.DeviceKeyThumbprint) ||
                     string.IsNullOrWhiteSpace(session.DevicePublicKey)))
                {
                    if (!replayNativeLogin && CanDeferPrimaryLoginForClientProof(session))
                    {
                        DeferPrimaryLoginUntilClientProof(session);
                        return new PacketResult(PacketResultType.Block);
                    }

                    Log.Warning("Rejected outdated or unattested Gateway client from {ClientIp}", session.ClientIp);
                    await SendSecurityMessageAsync(session, "Security.ClientUpdateRequired");
                    return new PacketResult(PacketResultType.Disconnect);
                }

                if (await BotProtectionService.ShouldBlockGatewayLoginAsync(session, user_id))
                    return new PacketResult(PacketResultType.Disconnect);

                var credentialStatus =
                    await sqlQueryHelper.ValidateUserCredentialsStatusAsync(user_id, user_pw);
                if (credentialStatus == CredentialValidationStatus.Invalid)
                    return await ForwardOrReplayNativeLoginAsync(session, replayNativeLogin);
                if (credentialStatus == CredentialValidationStatus.Unavailable)
                {
                    await SendSecurityMessageAsync(session, "Security.AuthenticationUnavailable");
                    return new PacketResult(PacketResultType.Disconnect);
                }

                if (useManagedClientlessLogin)
                {
                    session.SessionData.Hwid = "CLIENTLESS:" + user_id;
                    Log.Information(
                        "Trusted managed Clientless Gateway login accepted for {Username} from {ClientIp}",
                        user_id,
                        session.ClientIp);
                }
                else if (!await sqlQueryHelper.BindAccountDeviceAsync(
                             user_id, session.SessionData.Hwid, session.DeviceKeyThumbprint,
                             session.DevicePublicKey))
                {
                    await SendSecurityMessageAsync(session, "Security.HwidVerificationFailed");
                    return new PacketResult(PacketResultType.Disconnect);
                }

                if (_serverSettings.SecondaryPassword && !useManagedClientlessLogin)
                {
                    var blockedUntil = await sqlQueryHelper.GetSecondaryPasswordBlockAsync(
                        user_id, session.ClientIp, session.DeviceKeyThumbprint);
                    if (blockedUntil.HasValue)
                    {
                        await SendSecurityMessageAsync(session, "Security.SecondaryTemporarilyLocked");
                        return new PacketResult(PacketResultType.Block);
                    }

                    var secondaryPasswordData = await sqlQueryHelper.GetSecondaryPasswordData(user_id);
                    if (secondaryPasswordData != null)
                    {
                        if (secondaryPasswordData.RememberPC &&
                            string.Equals(secondaryPasswordData.DeviceKeyThumbprint,
                                session.DeviceKeyThumbprint, StringComparison.OrdinalIgnoreCase))
                        {
                            session.GatewayAuthenticationState = GatewayAuthenticationState.Released;
                            if (await OfflineStallGatewayBridge.TryReplaceOfflineStallLoginAsync(
                                    session, user_id, user_pw, () => ReplayNativeLoginAsync(session),
                                    credentialsAlreadyValidated: true))
                                return new PacketResult(PacketResultType.Block);
                            return await ForwardOrReplayNativeLoginAsync(session, replayNativeLogin);
                        }

                        session.GatewayAuthenticationState = GatewayAuthenticationState.AwaitingSecondaryEntry;
                        Packet pck = new Packet(0x1212);
                        pck.WriteUInt8(SecondaryPasswordResponse.ENTER_THE_PASSWORD);
                        await session.SendToClient(pck);
                        return new PacketResult(PacketResultType.Block);
                    }

                    session.GatewayAuthenticationState = GatewayAuthenticationState.AwaitingSecondaryCreate;
                    Packet createPacket = new Packet(0x1212);
                    createPacket.WriteUInt8(SecondaryPasswordResponse.CREATE_THE_PASSWORD);
                    await session.SendToClient(createPacket);
                    return new PacketResult(PacketResultType.Block);
                }

                session.GatewayAuthenticationState = GatewayAuthenticationState.Released;
                if (await OfflineStallGatewayBridge.TryReplaceOfflineStallLoginAsync(
                        session, user_id, user_pw, () => ReplayNativeLoginAsync(session),
                        credentialsAlreadyValidated: true))
                    return new PacketResult(PacketResultType.Block);

                return await ForwardOrReplayNativeLoginAsync(session, replayNativeLogin);
            }
            catch (Exception EX)
            {
                Log.Warning(EX, "Gateway login security failed for {ClientIp}", session.ClientIp);
                await SendSecurityMessageAsync(session, "Security.AuthenticationUnavailable");
                return new PacketResult(PacketResultType.Disconnect);
            }
        }

        internal static bool CanDeferPrimaryLoginForClientProof(ISession session)
        {
            return session.GatewayAuthenticationState == GatewayAuthenticationState.AwaitingHwid &&
                   string.IsNullOrEmpty(session.SessionData.Hwid) &&
                   !string.IsNullOrWhiteSpace(session.HwidChallenge) &&
                   string.Equals(session.HwidChallengeRole, HwidSecurity.GatewayRole, StringComparison.Ordinal) &&
                   DateTime.UtcNow <= session.HwidChallengeExpiresAt;
        }

        private static void DeferPrimaryLoginUntilClientProof(ISession session)
        {
            if (session.PendingPrimaryLogin)
                return;

            session.PendingPrimaryLogin = true;
            session.PendingPrimaryLoginStartedAt = Environment.TickCount64;
            Log.Information(
                "Gateway primary login deferred for in-flight client proof from {ClientIp}",
                session.ClientIp);

            ServerManager.g_DelayedJobMgr.CreateJob(new DelayedJobItem(
                PrimaryLoginProofGraceMilliseconds,
                session,
                session.PendingPrimaryLoginStartedAt,
                async (state, marker) =>
                {
                    var pendingSession = (ISession)state;
                    var expectedStart = marker is long value ? value : 0;
                    if (pendingSession.IsStopped ||
                        !pendingSession.PendingPrimaryLogin ||
                        pendingSession.PendingPrimaryLoginStartedAt != expectedStart ||
                        HasCompleteGatewayAttestation(pendingSession))
                    {
                        return;
                    }

                    pendingSession.PendingPrimaryLogin = false;
                    pendingSession.PendingPrimaryLoginStartedAt = 0;
                    Log.Warning(
                        "Gateway client proof did not arrive within {GraceMilliseconds} ms for {ClientIp}",
                        PrimaryLoginProofGraceMilliseconds,
                        pendingSession.ClientIp);
                    await SendSecurityMessageAsync(pendingSession, "Security.ClientUpdateRequired");
                    pendingSession.Stop("gateway client proof grace expired");
                }));
        }

        private static bool HasCompleteGatewayAttestation(ISession session)
        {
            return session.GatewayAuthenticationState == GatewayAuthenticationState.AwaitingPrimaryCredentials &&
                   !string.IsNullOrEmpty(session.SessionData.Hwid) &&
                   !string.IsNullOrWhiteSpace(session.DeviceKeyThumbprint) &&
                   !string.IsNullOrWhiteSpace(session.DevicePublicKey);
        }

        private static async Task<PacketResult> ForwardOrReplayNativeLoginAsync(
            ISession session,
            bool replayNativeLogin)
        {
            if (!replayNativeLogin)
                return new PacketResult(PacketResultType.Nothing);

            await ReplayNativeLoginAsync(session);
            return new PacketResult(PacketResultType.Block);
        }
        private async Task<PacketResult> SERVER_SENDDLLSETTINGS(Packet packet, ISession session, object obj)
        {
            try
            {
                Packet dc = new Packet(0x1210, true);
                dc.WriteBool(_serverSettings.OldLogin);
                dc.WriteBool(_serverSettings.OldExpBar);
                dc.WriteBool(_serverSettings.OldAlchemy);
                dc.WriteBool(_serverSettings.GrantNameButton);
                dc.WriteBool(_serverSettings.IconManagerButton);
                dc.WriteBool(_serverSettings.IconManagerRight);
                dc.WriteBool(_serverSettings.TitleManager);
                dc.WriteBool(_serverSettings.TitleManagerColor);
                dc.WriteBool(_serverSettings.DynamicRanking);
                dc.WriteBool(_serverSettings.UniqueHistory);
                dc.WriteBool(_serverSettings.EventRegister);
                dc.WriteBool(_serverSettings.EventSchedule);
                dc.WriteBool(_serverSettings.Achievements);
                dc.WriteBool(_serverSettings.SecondarySlot);
                dc.WriteBool(_serverSettings.MoveSkillBoard);
                dc.WriteBool(_serverSettings.ServerInfoSkill);
                dc.WriteBool(_serverSettings.OldMainPopup);
                dc.WriteBool(_serverSettings.HideTitleWhileTagActive);
                dc.WriteBool(_serverSettings.ItemComparison);
                dc.WriteBool(_serverSettings.AutoSort);
                dc.WriteBool(_serverSettings.PartyMemberViewer);
                dc.WriteBool(_serverSettings.AutoSkillUpdate);
                // The legacy client field is always consumed.  Keep it at
                // least as high as either race-specific limit so an older or
                // partially extended client can never retain the stock
                // European 240 cap before it reads the optional tail below.
                dc.WriteInt32(Math.Max(
                    _serverSettings.MasteryLimit,
                    Math.Max(
                        _serverSettings.ChineseMasteryLimit,
                        _serverSettings.EuropeanMasteryLimit)));
                dc.WriteUInt8(_serverSettings.ServerMaxLevel);
                dc.WriteBool(_serverSettings.FixDamageText);
                dc.WriteBool(_serverSettings.AutoStrInt);
                dc.WriteBool(_serverSettings.PickupEffect);
                dc.WriteBool(_serverSettings.PermanentAlchemy);
                dc.WriteBool(_serverSettings.ShowGuildInJobMode);
                dc.WriteBool(_serverSettings.UniqueTarget);
                dc.WriteBool(_serverSettings.Macro);
                dc.WriteBool(_serverSettings.SecondaryPassword);
                dc.WriteBool(_serverSettings.NewCharInfo);
                dc.WriteBool(_serverSettings.NewIdPw);
                dc.WriteAscii(GetConfiguredServerName());
                dc.WriteAscii(_serverSettings.FacebookURL);
                dc.WriteAscii(_serverSettings.DiscordURL);
                dc.WriteAscii(_serverSettings.WebsiteURL);
                dc.WriteBool(_serverSettings.Changelog);
                dc.WriteBool(_serverSettings.ShowChangelogFirstSpawn);
                dc.WriteBool(_serverSettings.FixNewJobSuit);
                dc.WriteBool(_serverSettings.OldItemMall);
                dc.WriteBool(_serverSettings.InsertCommaPrices);
                dc.WriteBool(_serverSettings.WriteCharacterBound);
                dc.WriteBool(_serverSettings.NewItemMall);
                dc.WriteBool(_serverSettings.EmojiSystem);
                dc.WriteBool(_serverSettings.NewPartyMatch);
                dc.WriteBool(_serverSettings.NewJobUI);
                // Preserve the DLL settings packet layout while keeping the retired
                // New Alchemy interface unavailable to every supported client.
                dc.WriteBool(_serverSettings.IsNewAlchemyAvailable);
                dc.WriteBool(_serverSettings.EnableItemTranslation);
                dc.WriteUInt8(_serverSettings.ItemTranslationPayment);
                dc.WriteInt32(_serverSettings.ItemTranslationPrice);
                dc.WriteBool(_serverSettings.NonClosePTForm);
                dc.WriteBool(_serverSettings.EnableLuckySpin);
                dc.WriteBool(_serverSettings.EnableLuckySpinSilk);
                dc.WriteInt32(_serverSettings.LuckySpinPrice);
                dc.WriteBool(_serverSettings.ShowGuideMenu);
                dc.WriteBool(_serverSettings.ShowGuideLuckySpin);
                dc.WriteBool(_serverSettings.ShowGuideItemChest);
                dc.WriteBool(_serverSettings.ShowGuideMacro);
                dc.WriteBool(_serverSettings.ShowGuideDailyLogin);
                dc.WriteBool(_serverSettings.ShowGuideDiscord);
                dc.WriteBool(_serverSettings.ShowGuideWebsite);
                dc.WriteBool(_serverSettings.ShowGuideFacebook);
                dc.WriteBool(_serverSettings.ShowGuideAutoEquip);
                dc.WriteBool(_serverSettings.ShowGuideWebViewer);
                dc.WriteBool(_serverSettings.ShowGuideMapLocation);
                dc.WriteBool(_serverSettings.EnableSpecialOffers);
                dc.WriteBool(_serverSettings.ShowGuideSpecialOffers);
                dc.WriteBool(_serverSettings.ShowGuideDropLogs);
                dc.WriteBool(_serverSettings.EnableQuickLogin);
                dc.WriteBool(_serverSettings.EnablePvpChallenge);
                dc.WriteBool(_serverSettings.ShowGuidePvpChallenge);
                dc.WriteBool(_serverSettings.ShowGuideKillerAnimation);
                dc.WriteBool(_serverSettings.NewInventoryDesign);
                dc.WriteBool(_serverSettings.EnableOfflineStall);
                dc.WriteBool(_serverSettings.MenuLikeMaxi);
                dc.WriteInt32(Math.Max(0, _serverSettings.AutoEquipMaxLevel));
                dc.WriteBool(_serverSettings.MenuCasy);
                dc.WriteInt32(_serverSettings.ChineseMasteryLimit);
                dc.WriteInt32(_serverSettings.EuropeanMasteryLimit);
             
                await session.SendToClient(dc);


                if (RefManager.m_RefNewAvatarMall.Count() > 0)
                {
                    Packet stAckMsg = new Packet(0x209B);
                    stAckMsg.WriteInt32(RefManager.m_RefNewAvatarMall.Count());
                    foreach (var data in RefManager.m_RefNewAvatarMall)
                    {
                        stAckMsg.WriteInt32(data.Value.ID);
                        stAckMsg.WriteUInt8(data.Value.CategoryID);
                        stAckMsg.WriteInt32(data.Value.ItemID);
                        stAckMsg.WriteInt32(data.Value.Silk);
                        stAckMsg.WriteInt32(data.Value.ItemIndex);
                        stAckMsg.WriteInt32(data.Value.PetObjID);
                    }
                    await session.SendToClient(stAckMsg);
                }


                if (RefManager.m_RefNewItemMall.Count() > 0)
                {
                    Packet stAckMsg = new Packet(0x208F);
                    stAckMsg.WriteInt32(RefManager.m_RefNewItemMall.Count());
                    foreach (var data in RefManager.m_RefNewItemMall)
                    {
                        stAckMsg.WriteInt32(data.Value.ID);
                        stAckMsg.WriteAscii(data.Value.CategoryName);
                        stAckMsg.WriteUInt8(data.Value.Type);
                        stAckMsg.WriteInt32(data.Value.ItemID);
                        stAckMsg.WriteInt32(data.Value.ItemCount);
                        stAckMsg.WriteInt32(data.Value.Silk);
                        stAckMsg.WriteUInt8(data.Value.ShowInNewBest);
                        stAckMsg.WriteInt32(data.Value.ItemIndex);
                        stAckMsg.WriteInt32(data.Value.ShowInNewBestIndex);

                    }
                    await session.SendToClient(stAckMsg);
                }



                return new PacketResult(packet, PacketResultType.Block);
            }
            catch (Exception EX)
            {
                Log.Warning(EX.Message.ToString() + "SERVER_SENDDLLSETTINGS");
                return new PacketResult(packet, PacketResultType.Block);
            }
           
    
        }
        private async Task<PacketResult> SERVER_SHARD_INFO(Packet packet, ISession session, object obj)
        {
            Packet SERVER_GATEWAY_SHARD_LIST_RESPONSE = new Packet(packet.Opcode, packet.Encrypted, packet.Massive);
            Packet CLIENT_SHARD_PRESENTATION_SYNC = new Packet(0x1215);
            byte GlobalOperationFlag = packet.ReadUInt8();
            SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(GlobalOperationFlag);
            CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(GlobalOperationFlag);

            if (GlobalOperationFlag == 1)
            {
                byte GlobalOperationType = packet.ReadUInt8();
                string GlobalOperationName = packet.ReadAscii();
                GlobalOperationFlag = packet.ReadUInt8();
                string DisplayServerName = GetConfiguredServerName();

                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(GlobalOperationType);
                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteAscii(DisplayServerName);
                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(GlobalOperationFlag);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(GlobalOperationType);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteAscii(DisplayServerName);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(GlobalOperationFlag);

            }
            byte ShardFlag = packet.ReadUInt8();
            SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(ShardFlag);
            CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(ShardFlag);
            while (ShardFlag == 1)
            {
                uint ShardID = packet.ReadUInt16();
                string ShardName = packet.ReadAscii();
                uint ShardCurrent = packet.ReadUInt16();

                uint ShardCapacity = packet.ReadUInt16();
                byte ShardStatus = packet.ReadUInt8();
                byte GlobalOperationID = packet.ReadUInt8();

                ShardFlag = packet.ReadUInt8();
                string DisplayServerName = GetConfiguredServerName();
                uint DisplayShardCurrent = GetDisplayShardCurrent(ShardCurrent, ShardCapacity, _serverSettings.FakePlayerCount);
                bool PresentShardAsCheck = ShouldPresentShardAsCheck(
                    _serverSettings.CheckStatus,
                    RefManager.IsGmIp(session.ClientIp));
                byte DisplayShardStatus = GetConfiguredShardStatus(PresentShardAsCheck);

                if (session.SessionData.ServerID == 0)
                    session.SessionData.ServerID = (ushort)ShardID;


                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt16(ShardID);
                if (_serverSettings.ShowOnlinePlayers)
                {
                    SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteAscii(
                        DisplayServerName + "   " +
                        (PresentShardAsCheck ? 0 : DisplayShardCurrent) +
                        "/" + ShardCapacity);
                }
                else
                {
                    SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteAscii(DisplayServerName);
                }

                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt16(
                    PresentShardAsCheck ? 0 : ShardCurrent);
                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt16(ShardCapacity);
                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(DisplayShardStatus);
                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(GlobalOperationID);
                SERVER_GATEWAY_SHARD_LIST_RESPONSE.WriteUInt8(ShardFlag);

                // The native A101 owns the actual shard row. The custom packet only
                // refreshes the KMTGuard location/status artwork after CPSTitle is
                // recreated by Restart.
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt16(ShardID);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteAscii(DisplayServerName);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt16(
                    PresentShardAsCheck ? 0 : DisplayShardCurrent);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt16(ShardCapacity);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(DisplayShardStatus);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(GlobalOperationID);
                CLIENT_SHARD_PRESENTATION_SYNC.WriteUInt8(ShardFlag);
            }

            session.QueueClientPacketAfterCurrentServerPacket(CLIENT_SHARD_PRESENTATION_SYNC);
            if(string.IsNullOrEmpty(session.SessionData.Hwid) &&
               HwidSecurity.TryCreateChallenge(
                   session, HwidSecurity.GatewayRole, out var challenge))
            {
                Packet hwid = new Packet(0x165A);
                hwid.WriteAscii(challenge);
                // Deliver device verification before the native shard list can
                // trigger an immediate login request on fast/local clients.
                await session.SendToClient(hwid);
                Log.Information(
                    "Gateway client proof challenge issued before shard presentation for {ClientIp}",
                    session.ClientIp);
            }
            return new PacketResult(
                SERVER_GATEWAY_SHARD_LIST_RESPONSE, PacketResultType.Override);
        }

        private static Task ReplayNativeLoginAsync(ISession session)
        {
            var login = new Packet(0x6102, true, false);
            login.WriteUInt8(session.SessionData.locale);
            login.WriteAscii(session.SessionData.user_id);
            login.WriteAscii(session.SessionData.user_pw);
            login.WriteUInt16(session.SessionData.ServerID);
            return session.SendToServer(login);
        }

        private static uint GetDisplayShardCurrent(uint shardCurrent, uint shardCapacity, int fakePlayerCount)
        {
            var fakeCount = Math.Max(0, fakePlayerCount);
            var displayCurrent = (ulong)shardCurrent + (uint)fakeCount;
            return (uint)Math.Min(displayCurrent, shardCapacity);
        }
    }
}
