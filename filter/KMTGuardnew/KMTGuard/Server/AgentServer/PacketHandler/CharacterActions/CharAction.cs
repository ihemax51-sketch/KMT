using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database.Models;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Servers.PacketHandler
{
    internal readonly record struct TeleportUseRequest(uint UniqueId, byte TeleportType, int DestinationSelector);

    public partial class CharAction
    {
        public CharAction(AgentServer agentServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x7045, CLIENT_CHARACTER_SELECTOBJECT); // t0p

            packetHandler.RegisterClientHandler(0x7074, CLIENT_CHARACTER_ACTION_REQUEST); // Snow Shield fix
            packetHandler.RegisterClientHandler(0x3053, CHARACTER_GETUP_REQUEST);
            packetHandler.RegisterClientHandler(0x705A, AGENT_TELEPORT_USE);
            packetHandler.RegisterClientHandler(0x7081, AGENT_EXCHANGE_START);
            packetHandler.RegisterModuleHandler(0xB081, AGENT_EXCHANGE_START_RESPONSE);
            packetHandler.RegisterClientHandler(0x7082, AGENT_EXCHANGE_BOT_GUARD);
            packetHandler.RegisterClientHandler(0x7083, AGENT_EXCHANGE_BOT_GUARD);
            packetHandler.RegisterClientHandler(0x70A7, CLIENT_PLAYER_BERSERK); // Zerk Exploit - 0x70A7 - https://www.elitepvpers.com/forum/sro-pserver-guides-releases/3991992-release-invincible-avatar-magopt-exploit-3.html
        }

        private async Task<PacketResult> CLIENT_PLAYER_BERSERK(Packet packet, ISession session, object obj)
        {
            var flag = packet.ReadUInt8();
            if (flag == 1)
            {
                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Zerk,
                    "Region.BerserkDisabled");
                if (regionBlock != null)
                    return regionBlock;

                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableZerk)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_ZERK_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisableZerk)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_ZERK_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }

                return new PacketResult();
            }
            else
            {
                Log.Warning($"EXPLOIT {session.SessionData.Charname}({packet.Opcode}) tried to use INVIS EXPLOIT!");
                return new PacketResult(PacketResultType.Disconnect);
            }
        }

        private async Task<PacketResult> AGENT_EXCHANGE_START(Packet packet, ISession session, object obj)
        {
            try
            {
                var challengeBlock = await PvpChallengeService.BlockInteractionIfLockedAsync(session);
                if (challengeBlock != null)
                    return challengeBlock;

                if (packet.GetBytes().Length >= sizeof(uint))
                {
                    var reader = new Packet(packet);
                    reader.ToReadOnly();
                    var targetUniqueId = reader.ReadUInt32();
                    var hideAndSeekResult = await HideAndSeekEventService.PrepareExchangeAttemptAsync(
                        session,
                        targetUniqueId);
                    if (hideAndSeekResult.IsEventTarget &&
                        !hideAndSeekResult.ForwardForRangeValidation)
                        return new PacketResult(PacketResultType.Block);
                    if (hideAndSeekResult.ForwardForRangeValidation)
                        return new PacketResult();
                }

                var botTradeBlock = await BotProtectionService.BlockBotTradeIfDisabledAsync(session, "player exchange");
                if (botTradeBlock != null)
                    return botTradeBlock;

                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Exchange,
                    "Region.ExchangeDisabled");
                if (regionBlock != null)
                    return regionBlock;

                int gecensaniye = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_EXCHANGE_TIME).TotalSeconds);
                if (gecensaniye < _serverSettings.ExchangeDelay)
                {
                    int kalanSaniye = _serverSettings.ExchangeDelay - gecensaniye;
                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_EXCHANGE_DELAY"), kalanSaniye);
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }

                if (session.SessionData.CurLevel < _serverSettings.ExchangeLevel)
                {
                    string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_EXCHANGE_LEVEL"), _serverSettings.ExchangeLevel);
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
                session.SessionData.LAST_EXCHANGE_TIME = DateTime.Now;

            }
            catch
            {
            }

            return new PacketResult();
        }

        private async Task<PacketResult> AGENT_EXCHANGE_START_RESPONSE(
            Packet packet,
            ISession session,
            object obj)
        {
            var accepted = false;
            if (packet.GetBytes().Length > 0)
            {
                var reader = new Packet(packet);
                reader.ToReadOnly();
                accepted = reader.ReadUInt8() == 1;
            }

            var handled = await HideAndSeekEventService.ConfirmExchangeRangeAsync(session, accepted);
            return handled
                ? new PacketResult(PacketResultType.Block)
                : new PacketResult();
        }

        private async Task<PacketResult> AGENT_EXCHANGE_BOT_GUARD(Packet packet, ISession session, object obj)
        {
            var challengeBlock = await PvpChallengeService.BlockInteractionIfLockedAsync(session);
            if (challengeBlock != null)
                return challengeBlock;

            var botTradeBlock = await BotProtectionService.BlockBotTradeIfDisabledAsync(session, "player exchange");
            return botTradeBlock ?? new PacketResult();
        }

        private async Task<PacketResult> AGENT_TELEPORT_USE(Packet packet, ISession session, object obj)
        {
            var challengeBlock = await PvpChallengeService.BlockTravelIfLockedAsync(session);
            if (challengeBlock != null)
                return challengeBlock;

            if (!TryParseTeleportUseRequest(packet.GetBytes(), out var request))
            {
                Log.Warning(
                    "[RegionAdmission] malformed or unsupported teleport request blocked. CharID={CharID}, PayloadLength={PayloadLength}",
                    session.SessionData.Charid,
                    packet.GetBytes().Length);
                return new PacketResult(packet, PacketResultType.Block);
            }

            var targetTeleport = request.DestinationSelector;
            RegionDestination? admissionDestination;
            RegionTravelMethod admissionMethod;
            switch (request.TeleportType)
            {
                case 1: // Native return point.
                    admissionDestination = await RegionControlService.ResolveReverseDestinationAsync(session, deadLocation: false);
                    admissionMethod = RegionTravelMethod.Teleport;
                    break;
                case 2: // A normal RefTeleport destination.
                    admissionDestination = await RegionControlService.ResolveTeleportDestinationAsync(targetTeleport);
                    admissionMethod = RegionTravelMethod.Teleport;
                    break;
                case 3: // Runtime portals resolve their destination inside the GameServer.
                    admissionDestination = null;
                    admissionMethod = RegionTravelMethod.Teleport;
                    break;
                case 5: // Reverse guide: 2 = last recall, 3 = death location.
                    admissionDestination = await RegionControlService.ResolveReverseDestinationAsync(
                        session,
                        deadLocation: targetTeleport == 3);
                    admissionMethod = RegionTravelMethod.Reverse;
                    break;
                default:
                    return new PacketResult(packet, PacketResultType.Block);
            }

            if (admissionDestination == null)
            {
                // New characters can have no persisted native Reverse/return
                // destination yet. The GameServer owns that target; admit the
                // native request and enforce the resulting region after arrival.
                Log.Debug(
                    "[RegionAdmission] destination unresolved before teleport; post-arrival enforcement armed. CharID={CharID}, TeleportType={TeleportType}, Selector={Selector}",
                    session.SessionData.Charid,
                    request.TeleportType,
                    targetTeleport);
            }
            else
            {
                var admission = await RegionControlService.CheckAdmissionAsync(
                    session,
                    admissionDestination.WorldID,
                    admissionDestination.RegionID,
                    admissionMethod);
                if (!admission.Allowed)
                    return new PacketResult(packet, PacketResultType.Block);

            }

            RegionControlService.ArmPostArrivalAdmission(session, admissionMethod);

            // Guide Reverse packets are six bytes, not nine. They must be
            // admitted above, then passed to the native server without running
            // gate-only stored procedures that expect a RefTeleport ID.
            if (request.TeleportType != 2)
                return new PacketResult();

            {
                if (ActionManager.m_CanTeleport.TryGetValue(targetTeleport, out bool liveState)
                    && !liveState)
                {
                    Log.Information(
                        "Blocked teleport by live state. CharID={CharID}, GateID={GateID}",
                        session.SessionData.Charid,
                        targetTeleport);
                    await ServerManager.sendNotice(
                        session,
                        NoticeType.WARNING,
                        PlayerLanguage.Get("Teleport.Closed"));
                    return new PacketResult(packet, PacketResultType.Block);
                }

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    bool CanTeleport = true;
                    string? denyReason = null;

                    // ===== [قديم] Stored Procedure كما هو =====
                        using (var command = new SqlCommand("[dbo].[Hook_Teleport]", connection))
                    {
                        command.CommandType = CommandType.StoredProcedure;
                        command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                        command.Parameters.AddWithValue("@RefTeleportID", targetTeleport);

                        var returnValueParam = new SqlParameter("@CanTeleport", SqlDbType.Bit)
                        { Direction = ParameterDirection.Output };
                        command.Parameters.Add(returnValueParam);

                        await command.ExecuteNonQueryAsync();
                        CanTeleport = (bool)(returnValueParam.Value ?? false);
                    }

                    // لو الـ SP منع بالفعل — امنع فورًا
                    if (!CanTeleport)
                    {
                        await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Teleport.Blocked"));
                        return new PacketResult(packet, PacketResultType.Block);
                    }

                    // ===== [جديد] جِيب STR/INT من جدول SRO_VT_SHARD.dbo._Char بالـ CharID =====
                    // ملاحظة: لو الداتابيز على نفس السيرفر وتحت نفس اليوزر، 3-part name هتشتغل.
                    // لو على سيرفر مختلف، اعمل SqlConnection تانية بـ Program.ShardConnectionstring واستخدمها بدل "connection".
                    int baseStr = session.SessionData.BaseStr;   // fallback من السيشن
                    int baseInt = session.SessionData.BaseInt;   // fallback من السيشن
                    try
                    {
                        using (var cmdStats = new SqlCommand(@"
                    SELECT Strength, Intellect
                    FROM [SRO_VT_SHARD].[dbo].[_Char] WITH (NOLOCK)
                    WHERE CharID = @CharID;", connection))
                        {
                            cmdStats.Parameters.AddWithValue("@CharID", session.SessionData.Charid);

                            using var rs = await cmdStats.ExecuteReaderAsync(CommandBehavior.SingleRow);
                            if (await rs.ReadAsync())
                            {
                                // الكولمنز smallint => GetInt16
                                if (!rs.IsDBNull(0)) baseStr = rs.GetInt16(0);
                                if (!rs.IsDBNull(1)) baseInt = rs.GetInt16(1);
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        // في حال فشل القراءة، نكمّل بقيم السيشن (Fail-open على مستوى الSTR/INT فقط)
                        Serilog.Log.Warning($"[teleport_control] STR/INT fetch failed CharID={session.SessionData.Charid}: {ex.Message}");
                    }

                    // ===== فحص جدول teleport_control (بدون MaxPerIP) =====
                    try
                    {
                        using (var cmdRule = new SqlCommand(@"
                    SELECT TOP(1)
                        OnlyStr, OnlyInt, JobOnly, TraderOnly, ThiefOnly, HunterOnly,
                        LevelMin, EUOnly, CHOnly, PartyOnly, NoParty,
                        TimeWindowStart, TimeWindowEnd
                    FROM [dbo].[Teleport_RulesLegacy] WITH (NOLOCK)
                    WHERE GateID = @GateID", connection))
                        {
                            cmdRule.Parameters.AddWithValue("@GateID", targetTeleport);

                            using var r = await cmdRule.ExecuteReaderAsync(CommandBehavior.SingleRow);
                            if (await r.ReadAsync())
                            {
                                // قراءات آمنة لو الأعمدة ممكن تكون NULL
                                bool onlyStr = !r.IsDBNull(0) && r.GetBoolean(0);
                                bool onlyInt = !r.IsDBNull(1) && r.GetBoolean(1);
                                bool jobOnly = !r.IsDBNull(2) && r.GetBoolean(2);
                                bool traderOnly = !r.IsDBNull(3) && r.GetBoolean(3);
                                bool thiefOnly = !r.IsDBNull(4) && r.GetBoolean(4);
                                bool hunterOnly = !r.IsDBNull(5) && r.GetBoolean(5);

                                byte? levelMin = r.IsDBNull(6) ? (byte?)null : r.GetByte(6);
                                bool euOnly = !r.IsDBNull(7) && r.GetBoolean(7);
                                bool chOnly = !r.IsDBNull(8) && r.GetBoolean(8);
                                bool partyOnly = !r.IsDBNull(9) && r.GetBoolean(9);
                                bool noParty = !r.IsDBNull(10) && r.GetBoolean(10);
                                byte? twStart = r.IsDBNull(11) ? (byte?)null : r.GetByte(11); // 0..23
                                byte? twEnd = r.IsDBNull(12) ? (byte?)null : r.GetByte(12); // 0..23

                                // --- معلومات اللاعب من السيشن ---
                                byte job = session.SessionData.JobType;          // 0=NoJob, 1=Trader, 2=Thief, 3=Hunter
                                byte curLevel = session.SessionData.CurLevel;         // Level
                                bool isEU = session.SessionData.EUChar;           // Races
                                bool isCH = session.SessionData.CHChar;
                                bool inParty = session.SessionData.IsInParty;        // Party

                                // بناء على قيم الداتابيز (أو fallback من السيشن)
                                bool isStrBuild = baseStr > baseInt;
                                bool isIntBuild = baseInt > baseStr;

                                bool jobActive = (job == 1 || job == 2 || job == 3);
                                bool isTrader = (job == 1);
                                bool isThief = (job == 2);
                                bool isHunter = (job == 3);

                                // نافذة الساعات اليومية (بتوقيت السيرفر/UTC)
                                bool withinTime = true;
                                if (twStart.HasValue && twEnd.HasValue && twStart.Value != twEnd.Value)
                                {
                                    int hour = DateTime.UtcNow.Hour; // 0..23
                                    if (twStart.Value < twEnd.Value)
                                    {
                                        // مثال: 10 -> 22
                                        withinTime = (hour >= twStart.Value && hour < twEnd.Value);
                                    }
                                    else
                                    {
                                        // نافذة ليلية: مثال 22 -> 03
                                        withinTime = (hour >= twStart.Value || hour < twEnd.Value);
                                    }
                                }
                                // لو الاتنين NULL أو متساويين => بدون قيد ساعات

                                // نجمع الأسباب (لو فيه أكثر من قيد مخالف)
                                var reasons = new List<string>(4);

                                // Build
                                if (onlyStr && !isStrBuild) reasons.Add(PlayerLanguage.Get("Teleport.Reason.StrOnly"));
                                if (onlyInt && !isIntBuild) reasons.Add(PlayerLanguage.Get("Teleport.Reason.IntOnly"));

                                // Job
                                if (jobOnly && !jobActive) reasons.Add(PlayerLanguage.Get("Teleport.Reason.JobRequired"));
                                if (traderOnly && !isTrader) reasons.Add(PlayerLanguage.Get("Teleport.Reason.TradersOnly"));
                                if (thiefOnly && !isThief) reasons.Add(PlayerLanguage.Get("Teleport.Reason.ThievesOnly"));
                                if (hunterOnly && !isHunter) reasons.Add(PlayerLanguage.Get("Teleport.Reason.HuntersOnly"));

                                // Level
                                if (levelMin.HasValue && curLevel < levelMin.Value)
                                    reasons.Add(PlayerLanguage.Get("Teleport.Reason.MinimumLevel", levelMin.Value));

                                // Race
                                if (euOnly && !isEU) reasons.Add(PlayerLanguage.Get("Teleport.Reason.EuropeansOnly"));
                                if (chOnly && !isCH) reasons.Add(PlayerLanguage.Get("Teleport.Reason.ChineseOnly"));

                                // Party
                                if (partyOnly && !inParty) reasons.Add(PlayerLanguage.Get("Teleport.Reason.PartyRequired"));
                                if (noParty && inParty) reasons.Add(PlayerLanguage.Get("Teleport.Reason.PartyNotAllowed"));

                                // Time window
                                if (!withinTime)
                                {
                                    string tw = (twStart.HasValue && twEnd.HasValue)
                                        ? $"{twStart.Value:00}:00–{twEnd.Value:00}:00"
                                        : PlayerLanguage.Get("Teleport.Reason.RestrictedHours");
                                    reasons.Add(PlayerLanguage.Get("Teleport.Reason.AvailableBetween", tw));
                                }

                                if (reasons.Count > 0)
                                {
                                    CanTeleport = false;
                                    denyReason = PlayerLanguage.Get("Teleport.BlockedWithReasons", string.Join(" / ", reasons));
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        // Fail-open: أي خطأ في قراءة الجدول => لا تمنع التليبورت
                        Serilog.Log.Warning($"[teleport_control] check failed for GateID={targetTeleport}: {ex.Message}");
                    }

                    // ===== القرار النهائي والإشعار =====
                    if (!CanTeleport)
                    {
                        if (!string.IsNullOrEmpty(denyReason))
                            await ServerManager.sendNotice(session, NoticeType.WARNING, denyReason);
                        else
                            await ServerManager.sendNotice(session, NoticeType.WARNING, PlayerLanguage.Get("Teleport.Blocked"));

                        return new PacketResult(packet, PacketResultType.Block);
                    }
                }
            }
            return new PacketResult();
        }

        internal static bool TryParseTeleportUseRequest(byte[] payload, out TeleportUseRequest request)
        {
            request = default;
            if (payload == null || payload.Length < 5)
                return false;

            var uniqueId = BinaryPrimitives.ReadUInt32LittleEndian(payload.AsSpan(0, 4));
            var teleportType = payload[4];
            int selector;
            switch (teleportType)
            {
                case 1:
                    selector = 0;
                    break;
                case 2:
                    if (payload.Length < 9)
                        return false;
                    selector = BinaryPrimitives.ReadInt32LittleEndian(payload.AsSpan(5, 4));
                    if (selector <= 0)
                        return false;
                    break;
                case 3:
                    if (payload.Length < 6)
                        return false;
                    selector = payload[5];
                    break;
                case 5:
                    if (payload.Length < 6 || payload[5] is not (2 or 3))
                        return false;
                    selector = payload[5];
                    break;
                default:
                    return false;
            }

            request = new TeleportUseRequest(uniqueId, teleportType, selector);
            return true;
        }


        private async Task<PacketResult> CHARACTER_GETUP_REQUEST(Packet packet, ISession session, object obj)
        {
            try
            {
                byte x = packet.ReadUInt8();

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    var parameterReturn = new SqlParameter("@ReturnValue", System.Data.SqlDbType.Int)
                    {
                        Direction = System.Data.ParameterDirection.Output
                    };

                var command = new SqlCommand("EXEC @ReturnValue = [dbo].[Hook_CharacterGetUp] @CharID, @CharName, @LatestRegion, @LatestWorld, @PVPState; SELECT @ReturnValue;", connection);
                    command.Parameters.AddWithValue("@CharID", session.SessionData.Charid);
                    command.Parameters.AddWithValue("@CharName", session.SessionData.Charname);
                    command.Parameters.AddWithValue("@LatestRegion", session.SessionData.LatestRegion);
                    command.Parameters.AddWithValue("@LatestWorld", session.SessionData.WorldID);
                    command.Parameters.AddWithValue("@PVPState", Convert.ToByte(session.SessionData.State.PvpCape));

                    command.Parameters.Add(parameterReturn);

                    var result = await command.ExecuteScalarAsync();

                    var returnValue = (int)parameterReturn.Value;

                    if (returnValue == 0)
                    {
                        return new PacketResult(packet, PacketResultType.Block);
                    }
                    else
                    {
                        return new PacketResult(packet, PacketResultType.Nothing);
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString() + "AGENT_RESPAWN");
            }

            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private async Task<PacketResult> CLIENT_CHARACTER_ACTION_REQUEST(Packet packet, ISession session, object obj)
        {
            if (packet.RemainingRead() < 2)
            {
                Log.Warning(
                    "Blocked short character action packet. Char={CharName} Opcode=0x{Opcode:X4} Remaining={Remaining}",
                    session.SessionData.Charname,
                    packet.Opcode,
                    packet.RemainingRead());
                return new PacketResult(PacketResultType.Block);
            }

            var unk1 = packet.ReadUInt8();
            if (unk1 != 0x01) return new PacketResult();

            if (session.SessionData.CharPetList.Count() > 0)
            {
                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisablePetSpawn)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_PET_SPAWN_IS_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                else if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.WorldID))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.WorldID].DisablePetSpawn)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_PET_SPAWN_IS_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
            }

            var action = (CharacterAction)packet.ReadUInt8();
            if (action is CharacterAction.CommonAttack or CharacterAction.SkillCast)
            {
                var challengeBlock = await PvpChallengeService.BlockCombatIfFrozenAsync(session);
                if (challengeBlock != null)
                    return challengeBlock;
            }

            if (action == CharacterAction.CommonAttack)
            {
                if (IsSelectedTargetPlayer(session))
                {
                    var pvpBlock = await RegionControlService.BlockIfAsync(
                        session,
                        rule => !rule.Enable_PvP,
                        "Region.PvpDisabled");
                    if (pvpBlock != null)
                        return pvpBlock;
                }

                if (ActionManager.m_CanAttackbyWorldId.ContainsKey(session.SessionData.WorldID))
                {
                    if (!ActionManager.m_CanAttackbyWorldId[session.SessionData.WorldID])
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_SKILL_USAGE_CLOSED_AT_THIS_TIME");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                else if (ActionManager.m_CanAttackbyregionId.ContainsKey(session.SessionData.LatestRegion))
                {
                    if (!ActionManager.m_CanAttackbyregionId[session.SessionData.LatestRegion])
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_SKILL_USAGE_CLOSED_AT_THIS_TIME");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }

                if (_serverSettings.DisableAutoAttack)
                {
                    if (!(session.SessionData.WorldID >= 2 && session.SessionData.WorldID <= 9))
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_AUTO_ATTACK_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                if (_serverSettings.AutoAttackMaxLevel > 0)
                {
                    if (!(session.SessionData.WorldID >= 2 && session.SessionData.WorldID <= 9))
                    {
                        if (_serverSettings.AutoAttackMaxLevel < session.SessionData.CurLevel)
                        {
                            string noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_AUTO_ATTACK_DISABLED_LEVEL"), _serverSettings.AutoAttackMaxLevel);
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                }
            }
            else if (action == CharacterAction.Trace)
            {
                var regionBlock = await RegionControlService.BlockIfAsync(
                    session,
                    rule => !rule.Enable_Trace,
                    "Region.TraceDisabled");
                if (regionBlock != null)
                    return regionBlock;

                if (_serverSettings.DisableTraceWhileJob)
                {
                    if (session.SessionData.JobType != 4)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_TRACE_DISABLED_WHILE_JOB_MODE");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }
                if (RefManager.m_RefEventMapSettings.ContainsKey(session.SessionData.LatestRegion))
                {
                    if (RefManager.m_RefEventMapSettings[session.SessionData.LatestRegion].DisableTrace)
                    {
                        string noticeMessage = RefManager.GetNoticeMessage("MSG_TRACE_DISABLED");
                        Packet stMsg = new Packet(0x168A);
                        stMsg.WriteUInt8(NoticeType.WARNING);
                        stMsg.WriteUnicode(noticeMessage);
                        await session.SendToClient(stMsg);
                        return new PacketResult(PacketResultType.Block);
                    }
                }

                var traceTarget = ServerManager.AgentSessions.FindByUniqueCharId(
                    session.SessionData.SELECTEDUNIQUEID,
                    ServerManager.IsOnlinePlayer);
                if (traceTarget != null && RegionControlService.IsValidRegionId(
                        RegionControlService.NormalizeRegionId(traceTarget.SessionData.LatestRegion)))
                {
                    var traceAdmission = await RegionControlService.CheckAdmissionAsync(
                        session,
                        traceTarget.SessionData.WorldID,
                        traceTarget.SessionData.LatestRegion,
                        RegionTravelMethod.Trace);
                    if (!traceAdmission.Allowed)
                        return new PacketResult(PacketResultType.Block);
                }
                else
                {
                    // The native GameServer owns Trace target resolution. A visible target
                    // can legitimately be absent from this Filter process (for example,
                    // while session indexes converge or when multiple Agent instances are
                    // in use). Do not reject a valid native Trace solely because the Filter
                    // lacks a pre-movement destination snapshot; enforce the actual region
                    // immediately after the resulting spawn instead.
                    Log.Debug(
                        "[RegionAdmission] Trace destination unresolved before movement; native Trace allowed and post-arrival enforcement armed. CharID={CharID}, SelectedUniqueID={SelectedUniqueID}",
                        session.SessionData.Charid,
                        session.SessionData.SELECTEDUNIQUEID);
                }

                RegionControlService.ArmPostArrivalAdmission(session, RegionTravelMethod.Trace);
            }
            else if (action == CharacterAction.SkillCast)
            {
                if (packet.RemainingRead() < sizeof(uint) + sizeof(byte))
                {
                    Log.Warning(
                        "Blocked short skill-cast action packet. Char={CharName} Opcode=0x{Opcode:X4} Remaining={Remaining}",
                        session.SessionData.Charname,
                        packet.Opcode,
                        packet.RemainingRead());
                    return new PacketResult(PacketResultType.Block);
                }

                var skillId = (int)packet.ReadUInt32();
                byte action2 = packet.ReadUInt8(); // el yakmada 0 skill de 1 geliyor

                // ===== [جديد] حظر مهارة حسب الـ Region =====
                // يتطلب: RefManager.m_BlockedSkillsByRegion (ConcurrentDictionary<int, HashSet<int>>)
                int region = session.SessionData.LatestRegion;
                if (RefManager.m_BlockedSkillsByRegion != null &&
                    RefManager.m_BlockedSkillsByRegion.TryGetValue(region, out var blockedSet) &&
                    blockedSet != null && blockedSet.Contains(skillId))
                {
                    string noticeMessage = RefManager.GetNoticeMessage("MSG_SKILL_BLOCKED_IN_REGION");
                    Packet stMsg = new Packet(0x168A);
                    stMsg.WriteUInt8(NoticeType.WARNING);
                    stMsg.WriteUnicode(noticeMessage);
                    await session.SendToClient(stMsg);
                    return new PacketResult(PacketResultType.Block);
                }
                // ===== نهاية منطق الحظر =====

                if (action2 == 0)
                {
                    if (session.SessionData.LastBuffUsage + 150 > DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() || session.SessionData.LastSkillUsage + 150 > DateTimeOffset.UtcNow.ToUnixTimeMilliseconds())
                    {
                        return new PacketResult(PacketResultType.Block);
                    }
                    session.SessionData.LastBuffUsage = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
                }
                if (action2 == 1)
                {
                    if (IsSelectedTargetPlayer(session))
                    {
                        var pvpBlock = await RegionControlService.BlockIfAsync(
                            session,
                            rule => !rule.Enable_PvP,
                            "Region.PvpDisabled");
                        if (pvpBlock != null)
                            return pvpBlock;
                    }

                    if (session.SessionData.LastSkillUsage + 150 > DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() || session.SessionData.LastBuffUsage + 150 > DateTimeOffset.UtcNow.ToUnixTimeMilliseconds())
                    {
                        return new PacketResult(PacketResultType.Block);
                    }
                    session.SessionData.LastSkillUsage = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();

                    if (ActionManager.m_CanAttackbyWorldId.ContainsKey(session.SessionData.WorldID))
                    {
                        if (!ActionManager.m_CanAttackbyWorldId[session.SessionData.WorldID])
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("MSG_SKILL_USAGE_CLOSED_AT_THIS_TIME");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                    else if (ActionManager.m_CanAttackbyregionId.ContainsKey(session.SessionData.LatestRegion))
                    {
                        if (!ActionManager.m_CanAttackbyregionId[session.SessionData.LatestRegion])
                        {
                            string noticeMessage = RefManager.GetNoticeMessage("MSG_SKILL_USAGE_CLOSED_AT_THIS_TIME");
                            Packet stMsg = new Packet(0x168A);
                            stMsg.WriteUInt8(NoticeType.WARNING);
                            stMsg.WriteUnicode(noticeMessage);
                            await session.SendToClient(stMsg);
                            return new PacketResult(PacketResultType.Block);
                        }
                    }
                }
            }
            else if (action == CharacterAction.SkillRemove)
            {
                if (packet.RemainingRead() < sizeof(uint))
                {
                    Log.Warning(
                        "Blocked short skill-remove action packet. Char={CharName} Opcode=0x{Opcode:X4} Remaining={Remaining}",
                        session.SessionData.Charname,
                        packet.Opcode,
                        packet.RemainingRead());
                    return new PacketResult(PacketResultType.Block);
                }

                var skillId = (int)packet.ReadUInt32();
                // مكان لمنع إزالة باف معينة إن لزم
            }

            return new PacketResult();
        }

        private Task<PacketResult> CLIENT_CHARACTER_SELECTOBJECT(Packet packet, ISession session, object obj)
        {
            try
            {
                uint selecteduniqueid = packet.ReadUInt32();
                session.SessionData.SELECTEDUNIQUEID = selecteduniqueid;
            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString() + "AGENT_RESPAWN");
            }

            return Task.FromResult(new PacketResult(packet, PacketResultType.Nothing));
        }

        private static bool IsSelectedTargetPlayer(ISession session)
        {
            var selected = session.SessionData.SELECTEDUNIQUEID;
            if (selected == 0)
                return false;

            return ServerManager.AgentSessions.FindByUniqueCharId(selected) != null;
        }
    }
}
