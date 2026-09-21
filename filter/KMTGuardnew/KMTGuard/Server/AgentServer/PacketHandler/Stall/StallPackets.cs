using System.Data;
using KMTGuard.Database.Models;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using KMTGuard.Localization;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.AgentPacketHandler
{
    public partial class StallPackets
    {
        public StallPackets(AgentServer agentServer, IPacketHandler packetHandler)
        {
            packetHandler.RegisterClientHandler(0x186D, CLIENT_STALL_TYPE);
            packetHandler.RegisterClientHandler(0x70BA, AGENT_STALL_UPDATE);
            packetHandler.RegisterClientHandler(0x70B1, AGENT_STALL_CREATE);
            packetHandler.RegisterModuleHandler(0xB0B1, SERVER_AGENT_STALL_CREATE_RESPONSE);
            packetHandler.RegisterModuleHandler(0xB0B2, SERVER_AGENT_STALL_DESTROY_RESPONSE);
            packetHandler.RegisterModuleHandler(0xB0B3, SERVER_AGENT_STALL_TALK_RESPONSE);
            packetHandler.RegisterModuleHandler(0xB0B4, SERVER_AGENT_STALL_BUY_RESPONSE);
            packetHandler.RegisterModuleHandler(0xB0BA, SERVER_AGENT_STALL_ACTION_RESPONSE);
            packetHandler.RegisterClientHandler(0x70BA, CLIENT_STALL_ACTION);
            packetHandler.RegisterClientHandler(0x70B4, CLIENT_AGENT_STALL_BUY_REQUEST);
            packetHandler.RegisterClientHandler(OfflineStallService.ActivateRequestOpcode, CLIENT_OFFLINE_STALL_ACTIVATE);
            packetHandler.RegisterModuleHandler(0x30B7, SERVER_STALL_SLOT_UPDATE);
        }

        private async Task<PacketResult> CLIENT_OFFLINE_STALL_ACTIVATE(Packet packet, ISession session, object obj)
        {
            if (packet.GetBytes().Length < 4)
                return new PacketResult(PacketResultType.Disconnect);

            var nonce = packet.ReadUInt32();
            var result = await OfflineStallService.ActivateAsync(session);
            var response = new Packet(OfflineStallService.ActivateResultOpcode, true, false);
            response.WriteUInt32(nonce);
            response.WriteBool(result.Success);
            response.WriteAscii(result.Message);
            await session.SendToClient(response);
            if (result.Success)
                _ = OfflineStallService.DetachAfterActivationAsync(session);
            return new PacketResult(PacketResultType.Block);
        }

        private Task<PacketResult> SERVER_STALL_SLOT_UPDATE(Packet packet, ISession session, object obj)
        {
            var payloadLength = packet.GetBytes().Length;
            if (payloadLength < 2 || session.SessionData.UniqueCharId == 0)
                return Task.FromResult(new PacketResult(PacketResultType.Nothing));

            var uniqueId = session.SessionData.UniqueCharId;
            var updateType = packet.ReadUInt8();
            var soldSlot = packet.ReadUInt8();
            if (updateType == 0x03 && soldSlot <= 9 && ActionManager.OpenStalls.ContainsKey(uniqueId))
                _ = OfflineStallService.ReconcileSoldSlotAsync(uniqueId, soldSlot);

            return Task.FromResult(new PacketResult(PacketResultType.Nothing));
        }

        private Task<PacketResult> SERVER_AGENT_STALL_ACTION_RESPONSE(Packet packet, ISession session, object obj)
        {
            var payloadLength = packet.GetBytes().Length;
            if (payloadLength < 2)
                return Task.FromResult(new PacketResult(PacketResultType.Nothing));

            // B0BA uses 1 for success and 2 for failure (not a zero/non-zero
            // boolean), so compare the raw result byte explicitly.
            var success = packet.ReadUInt8() == 0x01;
            var actionType = packet.ReadUInt8();
            var matched = ActionManager.TryTakeStallAction(session.ClientGuid, actionType, out var pending);
            if (!matched && pending != null)
            {
                if (pending.ActionType == 0x05)
                {
                    ActionManager.SetStallOperating(
                        session.SessionData.UniqueCharId,
                        pending.PreviousOperating);
                }
                Log.Warning(
                    "Stall response action {ResponseAction} did not match pending action {PendingAction} for {CharName}",
                    actionType,
                    pending.ActionType,
                    session.SessionData.Charname);
            }

            if (matched && pending != null && actionType is 0x01 or 0x02)
            {
                if (success)
                {
                    ActionManager.TrackStallSlot(session.SessionData.UniqueCharId, pending.StallSlot);
                    if (pending.SilkSlot != null)
                        SilkStallService.ApplySlotDefinition(session, pending.SilkSlot);
                }
            }
            else if (matched && pending != null && actionType == 0x03)
            {
                if (success)
                {
                    ActionManager.RemoveStallSlot(session.SessionData.UniqueCharId, pending.StallSlot);
                    if (session.SessionData.isSilkStall)
                        SilkStallService.ApplySlotRemoval(session, pending.StallSlot);
                }
            }
            else if (matched && pending != null && actionType == 0x05)
            {
                if (success && payloadLength >= 3)
                {
                    var state = packet.ReadUInt8();
                    ActionManager.SetStallOperating(session.SessionData.UniqueCharId, state == 0x01);
                }
                else if (!success)
                {
                    ActionManager.SetStallOperating(
                        session.SessionData.UniqueCharId,
                        pending.PreviousOperating);
                }
            }

            return Task.FromResult(new PacketResult(PacketResultType.Nothing));
        }

        private Task<PacketResult> SERVER_AGENT_STALL_CREATE_RESPONSE(Packet packet, ISession session, object obj)
        {
            if (packet.GetBytes().Length < 1)
                return Task.FromResult(new PacketResult(PacketResultType.Disconnect));
            var success = IsStallSuccess(packet.ReadUInt8());

            if (success)
            {
                ActionManager.CancelPendingStallActions(session.ClientGuid);
                ActionManager.OpenStall(
                    session.SessionData.UniqueCharId,
                    session.SessionData.JID,
                    session.SessionData.Charid,
                    session.SessionData.Charname);
                AutoEventService.RegisterStallOpened(session);
            }

            if (success && session.SessionData.isSilkStall)
            {
                ActionManager.OpenSilkStall(
                    session.SessionData.UniqueCharId,
                    session.SessionData.JID,
                    session.SessionData.Charid,
                    session.SessionData.Charname);
            }
            else if (!success)
            {
                ActionManager.CancelPendingStallActions(session.ClientGuid);
                session.SessionData.isSilkStall = false;
                ActionManager.CloseStall(session.SessionData.UniqueCharId);
            }

            return Task.FromResult(new PacketResult(PacketResultType.Nothing));
        }

        private Task<PacketResult> SERVER_AGENT_STALL_DESTROY_RESPONSE(Packet packet, ISession session, object obj)
        {
            if (packet.GetBytes().Length < 1)
                return Task.FromResult(new PacketResult(PacketResultType.Disconnect));
            var success = IsStallSuccess(packet.ReadUInt8());

            if (success)
            {
                ActionManager.CancelPendingStallActions(session.ClientGuid);
                session.SessionData.isSilkStall = false;
                ActionManager.CloseStall(session.SessionData.UniqueCharId);
                _ = OfflineStallService.TerminateByUniqueIdAsync(
                    session.SessionData.UniqueCharId, "stall closed");
            }

            return Task.FromResult(new PacketResult(PacketResultType.Nothing));
        }

        private async Task<PacketResult> SERVER_AGENT_STALL_TALK_RESPONSE(Packet packet, ISession session, object obj)
        {
            if (packet.GetBytes().Length < 1)
                return new PacketResult(PacketResultType.Disconnect);
            var success = IsStallSuccess(packet.ReadUInt8());

            if (success)
            {
                if (packet.GetBytes().Length < 5)
                    return new PacketResult(PacketResultType.Disconnect);
                var uniqueId = packet.ReadUInt32();
                session.SessionData.lastStallUId = uniqueId;

                var response = new Packet(0x186D);
                response.WriteBool(SilkStallService.IsSilkStall(uniqueId));
                await session.SendToClient(response);
            }
            else
            {
                session.SessionData.lastStallUId = 0;
            }

            return new PacketResult(PacketResultType.Nothing);
        }

        private async Task<PacketResult> SERVER_AGENT_STALL_BUY_RESPONSE(Packet packet, ISession session, object obj)
        {
            if (packet.GetBytes().Length < 1)
                return new PacketResult(PacketResultType.Disconnect);
            var success = IsStallSuccess(packet.ReadUInt8());
            byte? confirmedSlot = success && packet.GetBytes().Length >= 2
                ? packet.ReadUInt8()
                : null;

            // Persist the Silk outcome before any unrelated awaited work so a
            // disconnect cannot race a successful purchase into the refund path.
            if (ActionManager.PendingSilkStallPurchases.TryGetValue(session.ClientGuid, out var pending))
            {
                if (success)
                    await SilkStallService.CompleteReservedPurchaseAsync(session, pending);
                else
                    await SilkStallService.RefundReservedPurchaseAsync(
                        session, pending, "game server rejected stall buy");
            }

            await OfflineStallService.CompletePendingPurchaseAsync(session, success, confirmedSlot);

            return new PacketResult(PacketResultType.Nothing);
        }

        private Task<PacketResult> CLIENT_STALL_TYPE(Packet packet, ISession session, object obj)
        {
            session.SessionData.isSilkStall = packet.ReadBool();
            Log.Information(
                "Silk stall type selected for {CharName}: {IsSilkStall}",
                session.SessionData.Charname,
                session.SessionData.isSilkStall);
            return Task.FromResult(new PacketResult(PacketResultType.Block));
        }

        private async Task<PacketResult> CLIENT_STALL_ACTION(Packet packet, ISession session, object obj)
        {
            var regionBlock = await RegionControlService.BlockIfAsync(
                session,
                rule => !rule.Enable_Stall,
                "Region.StallDisabled");
            if (regionBlock != null)
                return regionBlock;

            var payloadLength = packet.GetBytes().Length;
            if (payloadLength < 1)
                return new PacketResult(PacketResultType.Disconnect);

            var actionType = packet.ReadUInt8();

            switch (actionType)
            {
                case 0x01:
                {
                    if (payloadLength < 14)
                        return new PacketResult(PacketResultType.Disconnect);
                    var stallSlot = packet.ReadUInt8();
                    var quantity = packet.ReadUInt16();
                    var price = packet.ReadUInt64();
                    var tail = packet.ReadUInt16();

                    Log.Information(
                        "Stall action 0x01 by {CharName}: silk={IsSilkStall}, slot={StallSlot}, quantity={Quantity}, price={Price}",
                        session.SessionData.Charname,
                        session.SessionData.isSilkStall,
                        stallSlot,
                        quantity,
                        price);

                    SilkStallSlot? silkSlot = null;
                    if (session.SessionData.isSilkStall)
                    {
                        if (!SilkStallService.TryCreateSlotDefinition(
                                session, stallSlot, 0, quantity, price, 0, out silkSlot, out var error))
                            return error;
                        if (error.PacketResultType == PacketResultType.Block)
                            return error;
                    }

                    ActionManager.QueueStallAction(
                        session.ClientGuid,
                        new PendingStallAction(0x01, stallSlot, false, silkSlot));

                    break;
                }

                case 0x02:
                {
                    if (payloadLength < 19)
                        return new PacketResult(PacketResultType.Disconnect);
                    var stallSlot = packet.ReadUInt8();
                    var inventorySlot = packet.ReadUInt8();
                    var quantity = packet.ReadUInt16();
                    var price = packet.ReadUInt64();
                    var tid = packet.ReadUInt32();
                    var tail = packet.ReadUInt16();

                    Log.Information(
                        "Stall action 0x02 by {CharName}: silk={IsSilkStall}, slot={StallSlot}, invSlot={InventorySlot}, quantity={Quantity}, price={Price}, tid={Tid}",
                        session.SessionData.Charname,
                        session.SessionData.isSilkStall,
                        stallSlot,
                        inventorySlot,
                        quantity,
                        price,
                        tid);

                    SilkStallSlot? silkSlot = null;
                    if (session.SessionData.isSilkStall)
                    {
                        if (!SilkStallService.TryCreateSlotDefinition(
                                session, stallSlot, inventorySlot, quantity, price, tid, out silkSlot, out var error))
                            return error;
                        if (error.PacketResultType == PacketResultType.Block)
                            return error;
                    }

                    ActionManager.QueueStallAction(
                        session.ClientGuid,
                        new PendingStallAction(0x02, stallSlot, false, silkSlot));

                    break;
                }

                case 0x03:
                {
                    if (payloadLength < 2)
                        return new PacketResult(PacketResultType.Disconnect);
                    var stallSlot = packet.ReadUInt8();
                    if (session.SessionData.isSilkStall && !SilkStallService.CanRemoveSlot(session, stallSlot))
                        return new PacketResult(PacketResultType.Block);
                    ActionManager.QueueStallAction(
                        session.ClientGuid,
                        new PendingStallAction(0x03, stallSlot, false, null));
                    break;
                }

                case 0x05:
                {
                    if (payloadLength < 2)
                        return new PacketResult(PacketResultType.Disconnect);
                    var state = packet.ReadUInt8();
                    var previousOperating = ActionManager.OpenStalls.TryGetValue(
                                                session.SessionData.UniqueCharId,
                                                out var openStall) && openStall.IsOperating;
                    ActionManager.QueueStallAction(
                        session.ClientGuid,
                        new PendingStallAction(0x05, 0, previousOperating, null));
                    // Closing Operating mode must take effect before the
                    // GameServer response so a fast activation request cannot
                    // race the transition into Modifying mode.
                    if (state != 0x01)
                        ActionManager.SetStallOperating(session.SessionData.UniqueCharId, false);
                    break;
                }
            }

            return new PacketResult(PacketResultType.Nothing);
        }

        private async Task<PacketResult> CLIENT_AGENT_STALL_BUY_REQUEST(Packet packet, ISession session, object obj)
        {
            var regionBlock = await RegionControlService.BlockIfAsync(
                session,
                rule => !rule.Enable_Stall,
                "Region.StallDisabled");
            if (regionBlock != null)
                return regionBlock;

            var stallSlot = packet.ReadUInt8();

            if (!SilkStallService.TryGetSlot(session.SessionData.lastStallUId, stallSlot, out var stall, out var slot))
            {
                OfflineStallService.TrackPendingPurchase(
                    session, session.SessionData.lastStallUId, stallSlot);
                return new PacketResult(PacketResultType.Nothing);
            }

            var sellerSession = SilkStallService.FindActiveSellerSession(stall.SellerUniqueId);
            if (sellerSession == null)
            {
                await session.SendNotice(PlayerLanguage.Get("SilkStall.NoLongerAvailable"));
                return new PacketResult(PacketResultType.Block);
            }

            var pending = await SilkStallService.ReservePurchaseAsync(session, sellerSession, stall, slot);
            if (pending == null)
                return new PacketResult(PacketResultType.Block);

            try
            {
                // This private packet reaches the GameServer immediately before
                // the original 0x70B4 request on the same ordered connection.
                await SilkStallService.PrepareGameServerPurchaseAsync(session, pending);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Failed to prepare Gold-neutral Silk Stall purchase {TransactionId}",
                    pending.TransactionId);
                await SilkStallService.RefundReservedPurchaseAsync(
                    session, pending, "game server preparation failed");
                await session.SendNotice(PlayerLanguage.Get("SilkStall.PurchaseUnavailable"));
                return new PacketResult(PacketResultType.Block);
            }

            OfflineStallService.TrackPendingPurchase(
                session, session.SessionData.lastStallUId, stallSlot);

            return new PacketResult(PacketResultType.Nothing);
        }

        private async Task<PacketResult> AGENT_STALL_CREATE(Packet packet, ISession session, object obj)
        {
            var challengeBlock = await PvpChallengeService.BlockInteractionIfLockedAsync(session);
            if (challengeBlock != null)
                return challengeBlock;

            var regionBlock = await RegionControlService.BlockIfAsync(
                session,
                rule => !rule.Enable_Stall,
                "Region.StallDisabled");
            if (regionBlock != null)
                return regionBlock;

            var stallName = packet.ReadAscii();
            Log.Information(
                "Stall create requested by {CharName}: silk={IsSilkStall}, title={StallTitle}",
                session.SessionData.Charname,
                session.SessionData.isSilkStall,
                stallName);
            try
            {
                var elapsedSeconds = Convert.ToInt32(DateTime.Now.Subtract(session.SessionData.LAST_STALL_TIME).TotalSeconds);
                if (elapsedSeconds < _serverSettings.StallDelay)
                {
                    var remainingSeconds = _serverSettings.StallDelay - elapsedSeconds;
                    var noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_STALL_DELAY"), remainingSeconds);
                    await SendWarningAsync(session, noticeMessage);
                    return new PacketResult(PacketResultType.Block);
                }

                if (session.SessionData.CurLevel < _serverSettings.StallLevel)
                {
                    var noticeMessage = string.Format(RefManager.GetNoticeMessage("MSG_STALL_LEVEL"), _serverSettings.StallLevel);
                    await SendWarningAsync(session, noticeMessage);
                    return new PacketResult(PacketResultType.Block);
                }

                if (session.SessionData.OnTransport)
                {
                    await SendWarningAsync(session, RefManager.GetNoticeMessage("MSG_STALL_DISABLED_ON_TRANSPORT"));
                    return new PacketResult(PacketResultType.Block);
                }

                session.SessionData.LAST_STALL_TIME = DateTime.Now;

                try
                {
                    using var connection = new SqlConnection(Program.Connectionstring);
            using var command = new SqlCommand("[dbo].[Hook_StallCreate]", connection);
                    command.CommandType = CommandType.StoredProcedure;
                    command.Parameters.Add(new SqlParameter("@CharID", SqlDbType.Int) { Value = session.SessionData.Charid });
                    command.Parameters.Add(new SqlParameter("@CharName", SqlDbType.NVarChar, 64) { Value = (object)session.SessionData.Charname ?? DBNull.Value });
                    command.Parameters.Add(new SqlParameter("@UniqueCharID", SqlDbType.BigInt) { Value = (long)session.SessionData.UniqueCharId });
                    command.Parameters.Add(new SqlParameter("@RegionID", SqlDbType.SmallInt) { Value = (short)0 });
                    command.Parameters.Add(new SqlParameter("@WorldID", SqlDbType.Int) { Value = session.SessionData.WorldID });
                    command.Parameters.Add(new SqlParameter("@StallTitle", SqlDbType.NVarChar, 128) { Value = (object)stallName ?? DBNull.Value });

                    await connection.OpenAsync();
                    await command.ExecuteNonQueryAsync();
                }
                catch (Exception ex)
                {
            Log.Error("[dbo].[Hook_StallCreate] {Message}", ex.Message);
                }
            }
            catch
            {
                // Preserve existing stall behavior: protection failures above block, unexpected
                // logging/procedure failures must not prevent the original stall create packet.
            }

            return new PacketResult();
        }

        private async Task<PacketResult> AGENT_STALL_UPDATE(Packet packet, ISession session, object obj)
        {
            var regionBlock = await RegionControlService.BlockIfAsync(
                session,
                rule => !rule.Enable_Stall,
                "Region.StallDisabled");
            if (regionBlock != null)
                return regionBlock;

            if (session.SessionData.OnTransport)
            {
                await SendWarningAsync(session, RefManager.GetNoticeMessage("MSG_STALL_DISABLED_ON_TRANSPORT"));
                return new PacketResult(PacketResultType.Block);
            }

            return new PacketResult(packet, PacketResultType.Nothing);
        }

        private static async Task SendWarningAsync(ISession session, string message)
        {
            var packet = new Packet(0x168A);
            packet.WriteUInt8(NoticeType.WARNING);
            packet.WriteUnicode(message);
            await session.SendToClient(packet);
        }

        internal static bool IsStallSuccess(byte result) => result == 0x01;

    }
}
