using System.Collections.Concurrent;
using System.Data;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public static class SilkStallService
{
    internal const ushort GameServerPrepareBuyOpcode = 0x3541;
    private static readonly TimeSpan RecoveryInterval = TimeSpan.FromSeconds(15);
    private static readonly ConcurrentDictionary<ulong, Guid> ReservedSlots = new();
    private static readonly ConcurrentDictionary<Guid, BuyerLockEntry> BuyerLocks = new();
    private static readonly ConcurrentDictionary<long, Task> BackgroundSettlements = new();
    private static readonly SemaphoreSlim LifecycleLock = new(1, 1);
    private static CancellationTokenSource? _recoveryCancellation;
    private static Task? _recoveryTask;

    private sealed class BuyerLockEntry
    {
        public readonly SemaphoreSlim Semaphore = new(1, 1);
        public int References;
        public bool Retired;
    }

    public static async Task InitializeAsync()
    {
        await LifecycleLock.WaitAsync();
        try
        {
            if (_recoveryCancellation != null)
                return;

            await RecoverDurableTransactionsAsync(recoverAllReserved: true, CancellationToken.None);
            _recoveryCancellation = new CancellationTokenSource();
            _recoveryTask = Task.Run(() => RecoveryLoopAsync(_recoveryCancellation.Token));
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static async Task ShutdownAsync(string reason)
    {
        await LifecycleLock.WaitAsync();
        try
        {
            foreach (var entry in ActionManager.PendingSilkStallPurchases.ToArray())
            {
                if (Volatile.Read(ref entry.Value.SettlementState) == 1)
                    QueueAcceptedSettlement(entry.Value);
                else
                    QueueRefund(entry.Value, reason);
            }

            var running = BackgroundSettlements.Values.ToArray();
            if (running.Length > 0)
                await Task.WhenAll(running).WaitAsync(TimeSpan.FromSeconds(10));

            if (_recoveryCancellation != null)
            {
                await _recoveryCancellation.CancelAsync();
                if (_recoveryTask != null)
                {
                    try
                    {
                        await _recoveryTask;
                    }
                    catch (OperationCanceledException)
                    {
                    }
                }

                _recoveryCancellation.Dispose();
                _recoveryCancellation = null;
                _recoveryTask = null;
            }

            await RecoverDurableTransactionsAsync(recoverAllReserved: true, CancellationToken.None);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Silk Stall shutdown recovery failed");
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static bool IsSilkStall(uint sellerUniqueId) =>
        ActionManager.SilkStalls.ContainsKey(sellerUniqueId);

    public static bool TryGetSlot(uint sellerUniqueId, byte stallSlot, out SilkStall stall, out SilkStallSlot slot)
    {
        stall = null!;
        slot = null!;
        return ActionManager.SilkStalls.TryGetValue(sellerUniqueId, out stall!) &&
               stall.StallPrices.TryGetValue(stallSlot, out slot!);
    }

    public static bool TryCreateSlotDefinition(ISession session, byte stallSlot, byte invSlot, ushort quantity,
        ulong silkPrice, uint tid, out SilkStallSlot? slot, out PacketResult error)
    {
        slot = null;
        error = new PacketResult(PacketResultType.Block);
        if (!session.SessionData.isSilkStall)
            return false;

        if (!ActionManager.SilkStalls.TryGetValue(session.SessionData.UniqueCharId, out var stall))
        {
            _ = session.SendNotice(PlayerLanguage.Get("SilkStall.CreateFirst"));
            return true;
        }

        if (!IsValidSlotDefinition(stallSlot, quantity, silkPrice))
        {
            _ = session.SendNotice(PlayerLanguage.Get("SilkStall.InvalidPrice"));
            return true;
        }

        var slotKey = GetSlotKey(session.SessionData.UniqueCharId, stallSlot);
        if (ReservedSlots.ContainsKey(slotKey))
        {
            _ = session.SendNotice(PlayerLanguage.Get("SilkStall.SlotBusy"));
            return true;
        }

        slot = new SilkStallSlot
        {
            StallSlot = stallSlot,
            InventorySlot = invSlot,
            Quantity = quantity,
            SilkPrice = (int)silkPrice,
            Tid = tid
        };

        error = new PacketResult(PacketResultType.Nothing);
        return true;
    }

    public static void ApplySlotDefinition(ISession session, SilkStallSlot slot)
    {
        if (ActionManager.SilkStalls.TryGetValue(session.SessionData.UniqueCharId, out var stall))
            stall.StallPrices[slot.StallSlot] = slot;
    }

    public static bool CanRemoveSlot(ISession session, byte stallSlot)
    {
        if (ReservedSlots.ContainsKey(GetSlotKey(session.SessionData.UniqueCharId, stallSlot)))
        {
            _ = session.SendNotice(PlayerLanguage.Get("SilkStall.SlotBusy"));
            return false;
        }
        return true;
    }

    public static void ApplySlotRemoval(ISession session, byte stallSlot)
    {
        if (ActionManager.SilkStalls.TryGetValue(session.SessionData.UniqueCharId, out var stall))
            stall.StallPrices.TryRemove(stallSlot, out _);
    }

    public static async Task<PendingSilkStallPurchase?> ReservePurchaseAsync(
        ISession buyerSession,
        ISession sellerSession,
        SilkStall stall,
        SilkStallSlot slot)
    {
        var buyerLock = AcquireBuyerLock(buyerSession.ClientGuid);
        await buyerLock.Semaphore.WaitAsync();
        try
        {
            if (ActionManager.PendingSilkStallPurchases.ContainsKey(buyerSession.ClientGuid))
            {
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.WaitForPreviousPurchase"));
                return null;
            }

            if (buyerSession.SessionData.JID <= 0 || sellerSession.SessionData.JID <= 0 ||
                stall.SellerUniqueId != sellerSession.SessionData.UniqueCharId ||
                !TryGetSlot(stall.SellerUniqueId, slot.StallSlot, out _, out var currentSlot) ||
                !SameSlot(currentSlot, slot))
            {
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.PurchaseUnavailable"));
                return null;
            }

            if (buyerSession.SessionData.JID == sellerSession.SessionData.JID)
            {
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.CannotBuyOwnItem"));
                return null;
            }

            var requestToken = Guid.NewGuid();
            var slotKey = GetSlotKey(stall.SellerUniqueId, slot.StallSlot);
            if (!ReservedSlots.TryAdd(slotKey, requestToken))
            {
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.SlotBusy"));
                return null;
            }

            try
            {
                var pending = await ReserveInDatabaseAsync(buyerSession, sellerSession, stall, slot, requestToken);
                if (pending == null)
                {
                    ReservedSlots.TryRemove(slotKey, out _);
                    return null;
                }

                if (!ActionManager.PendingSilkStallPurchases.TryAdd(buyerSession.ClientGuid, pending))
                {
                    QueueRefund(pending, "duplicate pending purchase");
                    await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.WaitForPreviousPurchase"));
                    return null;
                }

                return pending;
            }
            catch
            {
                ReservedSlots.TryRemove(slotKey, out _);
                throw;
            }
        }
        finally
        {
            buyerLock.Semaphore.Release();
            ReleaseBuyerLock(buyerSession.ClientGuid, buyerLock);
        }
    }

    private static BuyerLockEntry AcquireBuyerLock(Guid buyerId)
    {
        while (true)
        {
            var entry = BuyerLocks.GetOrAdd(buyerId, static _ => new BuyerLockEntry());
            lock (entry)
            {
                if (entry.Retired)
                    continue;
                entry.References++;
                return entry;
            }
        }
    }

    private static void ReleaseBuyerLock(Guid buyerId, BuyerLockEntry entry)
    {
        var dispose = false;
        lock (entry)
        {
            entry.References--;
            if (entry.References == 0)
            {
                entry.Retired = true;
                BuyerLocks.TryRemove(
                    new KeyValuePair<Guid, BuyerLockEntry>(buyerId, entry));
                dispose = true;
            }
        }
        if (dispose)
            entry.Semaphore.Dispose();
    }

    public static async Task PrepareGameServerPurchaseAsync(ISession buyerSession, PendingSilkStallPurchase pending)
    {
        await buyerSession.SendToServer(BuildGameServerPreparationPacket(
            buyerSession.GameServerPacketKey, pending));
    }

    public static async Task CompleteReservedPurchaseAsync(ISession buyerSession, PendingSilkStallPurchase pending)
    {
        if (Interlocked.CompareExchange(ref pending.SettlementState, 1, 0) != 0)
            return;

        RemoveSoldSlot(pending);
        try
        {
            var completed = await SettleAcceptedSaleAsync(pending, CancellationToken.None);
            if (completed)
            {
                FinishPending(pending);
                await RefreshOnlineBalancesAsync(pending);
                var sellerSession = FindOnlineSession(pending.SellerUniqueId);
                if (sellerSession != null)
                    await sellerSession.SendNotice(PlayerLanguage.Get("SilkStall.SaleCompleted", pending.SilkAmount));
            }
            else
            {
                FinishPending(pending);
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.SettlementPending"));
                QueueRecovery(pending.TransactionId);
            }

            await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.PurchaseCompleted", pending.SilkAmount));
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to persist accepted Silk Stall transaction {TransactionId}", pending.TransactionId);
            await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.SettlementPending"));
            QueueAcceptedSettlement(pending);
        }
    }

    public static async Task RefundReservedPurchaseAsync(ISession buyerSession, PendingSilkStallPurchase pending,
        string reason)
    {
        if (Interlocked.CompareExchange(ref pending.SettlementState, 2, 0) != 0)
            return;

        try
        {
            var refunded = await SettleRefundAsync(pending, reason, CancellationToken.None);
            if (refunded)
            {
                FinishPending(pending);
                await RefreshOnlineBalancesAsync(pending);
            }
            else
            {
                FinishPending(pending);
                QueueRecovery(pending.TransactionId);
            }
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Failed to refund Silk Stall transaction {TransactionId}", pending.TransactionId);
            QueueRefund(pending, reason);
        }
    }

    public static void QueueCancellation(ISession session, string reason)
    {
        if (ActionManager.PendingSilkStallPurchases.TryGetValue(session.ClientGuid, out var pending))
        {
            if (Volatile.Read(ref pending.SettlementState) == 1)
                QueueAcceptedSettlement(pending);
            else
                QueueRefund(pending, reason);
        }
        if (session.SessionData.UniqueCharId > 0)
            ActionManager.CloseStall(session.SessionData.UniqueCharId);
    }

    public static async Task CancelPendingForSessionAsync(ISession session, string reason)
    {
        if (ActionManager.PendingSilkStallPurchases.TryGetValue(session.ClientGuid, out var pending))
            await RefundReservedPurchaseAsync(session, pending, reason);
        if (session.SessionData.UniqueCharId > 0)
            ActionManager.CloseStall(session.SessionData.UniqueCharId);
    }

    public static ISession? FindOnlineSession(uint uniqueId) =>
        ServerManager.AgentSessions.FindByUniqueCharId(
            uniqueId,
            static session => session.CharacterGameReady && !session.IsStopped && !session.ClientDetached);

    public static ISession? FindActiveSellerSession(uint uniqueId) =>
        ServerManager.AgentSessions.FindByUniqueCharId(
            uniqueId,
            static session => session.CharacterGameReady &&
                              !session.IsStopped &&
                              (!session.ClientDetached || OfflineStallService.IsOfflineStallSession(session)));

    private static async Task<PendingSilkStallPurchase?> ReserveInDatabaseAsync(
        ISession buyerSession,
        ISession sellerSession,
        SilkStall stall,
        SilkStallSlot slot,
        Guid requestToken)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);
        try
        {
            var buyerSilk = await GetSilkBalanceForUpdateAsync(connection, transaction, accountDb,
                buyerSession.SessionData.JID);
            if (buyerSilk == null || buyerSilk.silk_own < slot.SilkPrice)
            {
                await transaction.RollbackAsync();
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.InsufficientSilk"));
                return null;
            }

            var sellerSilk = await GetSilkBalanceForUpdateAsync(connection, transaction, accountDb,
                sellerSession.SessionData.JID);
            if (sellerSilk == null || !CanCreditSilkBalance(sellerSilk.silk_own, slot.SilkPrice))
            {
                await transaction.RollbackAsync();
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.SellerCannotReceive"));
                return null;
            }

            var affected = await connection.ExecuteAsync(
                $@"UPDATE {accountDb}..SK_Silk
                   SET silk_own = silk_own - @Price
                   WHERE JID = @JID AND silk_own >= @Price",
                new { Price = slot.SilkPrice, JID = buyerSession.SessionData.JID }, transaction);
            if (affected != 1)
            {
                await transaction.RollbackAsync();
                await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.InsufficientSilk"));
                return null;
            }

            var transactionId = await connection.ExecuteScalarAsync<long>(
                @"INSERT INTO dbo.Stall_SilkTransactions
                  (RequestToken, BuyerJID, BuyerCharID, BuyerCharName, BuyerUniqueID,
                   SellerJID, SellerCharID, SellerCharName, SellerUniqueID,
                   StallSlot, InventorySlot, Quantity, ItemTid, SilkAmount, Status,
                   CreatedAt, ReservedAt, UpdatedAt)
                  OUTPUT INSERTED.ID
                  VALUES
                  (@RequestToken, @BuyerJID, @BuyerCharID, @BuyerCharName, @BuyerUniqueID,
                   @SellerJID, @SellerCharID, @SellerCharName, @SellerUniqueID,
                   @StallSlot, @InventorySlot, @Quantity, @ItemTid, @SilkAmount, @Status,
                   SYSUTCDATETIME(), SYSUTCDATETIME(), SYSUTCDATETIME());",
                new
                {
                    RequestToken = requestToken,
                    BuyerJID = buyerSession.SessionData.JID,
                    BuyerCharID = buyerSession.SessionData.Charid,
                    BuyerCharName = buyerSession.SessionData.Charname,
                    BuyerUniqueID = (long)buyerSession.SessionData.UniqueCharId,
                    SellerJID = sellerSession.SessionData.JID,
                    SellerCharID = sellerSession.SessionData.Charid,
                    SellerCharName = sellerSession.SessionData.Charname,
                    SellerUniqueID = (long)stall.SellerUniqueId,
                    StallSlot = slot.StallSlot,
                    InventorySlot = slot.InventorySlot,
                    Quantity = (short)slot.Quantity,
                    ItemTid = (long)slot.Tid,
                    SilkAmount = slot.SilkPrice,
                    Status = (byte)SilkStallTransactionStatus.Reserved
                }, transaction);

            await transaction.CommitAsync();
            buyerSilk.silk_own -= slot.SilkPrice;
            var pending = new PendingSilkStallPurchase
            {
                TransactionId = transactionId,
                RequestToken = requestToken,
                BuyerClientGuid = buyerSession.ClientGuid,
                BuyerJid = buyerSession.SessionData.JID,
                BuyerCharId = buyerSession.SessionData.Charid,
                BuyerCharName = buyerSession.SessionData.Charname,
                BuyerUniqueId = buyerSession.SessionData.UniqueCharId,
                SellerJid = sellerSession.SessionData.JID,
                SellerCharId = sellerSession.SessionData.Charid,
                SellerCharName = sellerSession.SessionData.Charname,
                SellerUniqueId = stall.SellerUniqueId,
                StallSlot = slot.StallSlot,
                InventorySlot = slot.InventorySlot,
                Quantity = slot.Quantity,
                Tid = slot.Tid,
                SilkAmount = slot.SilkPrice
            };

            try
            {
                await SendSilkBalanceToGameServerAsync(buyerSession, buyerSilk);
            }
            catch (Exception ex)
            {
                // The committed reservation remains authoritative. A later
                // balance refresh or reconnect repairs this display update.
                Log.Warning(ex, "Silk Stall buyer balance refresh failed after reservation {TransactionId}",
                    transactionId);
            }

            return pending;
        }
        catch (Exception ex)
        {
            try
            {
                await transaction.RollbackAsync();
            }
            catch (InvalidOperationException)
            {
                // The transaction may already have been completed by SQL.
            }
            Log.Error(ex, "Failed to reserve Silk Stall purchase for {Buyer} from {Seller}",
                buyerSession.SessionData.Charname, sellerSession.SessionData.Charname);
            await buyerSession.SendNotice(PlayerLanguage.Get("SilkStall.PurchaseUnavailable"));
            return null;
        }
    }

    private static async Task<bool> SettleAcceptedSaleAsync(PendingSilkStallPurchase pending,
        CancellationToken cancellationToken)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(
            IsolationLevel.Serializable, cancellationToken);

        var status = await connection.ExecuteScalarAsync<byte?>(
            @"SELECT Status FROM dbo.Stall_SilkTransactions WITH (UPDLOCK, HOLDLOCK) WHERE ID = @ID",
            new { ID = pending.TransactionId }, transaction);
        if (status == null || status is not ((byte)SilkStallTransactionStatus.Reserved) and
            not ((byte)SilkStallTransactionStatus.SaleAccepted) and
            not ((byte)SilkStallTransactionStatus.Completed))
        {
            await transaction.RollbackAsync(cancellationToken);
            throw new InvalidOperationException($"Silk Stall transaction {pending.TransactionId} cannot be accepted from status {status}.");
        }

        if (status == (byte)SilkStallTransactionStatus.Completed)
        {
            await transaction.CommitAsync(cancellationToken);
            return true;
        }

        if (status == (byte)SilkStallTransactionStatus.Reserved)
        {
            var transitioned = await connection.ExecuteAsync(
                @"UPDATE dbo.Stall_SilkTransactions
                  SET Status = @Accepted, AcceptedAt = COALESCE(AcceptedAt, SYSUTCDATETIME()),
                      UpdatedAt = SYSUTCDATETIME()
                  WHERE ID = @ID AND Status = @Reserved",
                new
                {
                    ID = pending.TransactionId,
                    Accepted = (byte)SilkStallTransactionStatus.SaleAccepted,
                    Reserved = (byte)SilkStallTransactionStatus.Reserved
                }, transaction);
            if (transitioned != 1)
            {
                await transaction.RollbackAsync(cancellationToken);
                throw new InvalidOperationException(
                    $"Silk Stall transaction {pending.TransactionId} could not transition to accepted.");
            }
        }

        var credited = await connection.ExecuteAsync(
            $@"UPDATE {accountDb}..SK_Silk
               SET silk_own = silk_own + @Price
               WHERE JID = @JID AND silk_own <= @MaximumBeforeCredit",
            new
            {
                Price = pending.SilkAmount,
                JID = pending.SellerJid,
                MaximumBeforeCredit = int.MaxValue - pending.SilkAmount
            }, transaction);

        if (credited == 1)
        {
            var finalized = await connection.ExecuteAsync(
                @"UPDATE dbo.Stall_SilkTransactions
                  SET Status = @Completed, CompletedAt = SYSUTCDATETIME(), UpdatedAt = SYSUTCDATETIME()
                  WHERE ID = @ID AND Status = @Accepted",
                new
                {
                    ID = pending.TransactionId,
                    Completed = (byte)SilkStallTransactionStatus.Completed,
                    Accepted = (byte)SilkStallTransactionStatus.SaleAccepted
                }, transaction);
            if (finalized != 1)
            {
                await transaction.RollbackAsync(cancellationToken);
                throw new InvalidOperationException(
                    $"Silk Stall transaction {pending.TransactionId} could not be finalized after seller credit.");
            }
        }

        await transaction.CommitAsync(cancellationToken);
        return credited == 1;
    }

    private static async Task<bool> SettleRefundAsync(PendingSilkStallPurchase pending, string reason,
        CancellationToken cancellationToken)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(
            IsolationLevel.Serializable, cancellationToken);

        var status = await connection.ExecuteScalarAsync<byte?>(
            @"SELECT Status FROM dbo.Stall_SilkTransactions WITH (UPDLOCK, HOLDLOCK) WHERE ID = @ID",
            new { ID = pending.TransactionId }, transaction);
        if (status == (byte)SilkStallTransactionStatus.Completed ||
            status == (byte)SilkStallTransactionStatus.SaleAccepted)
        {
            await transaction.RollbackAsync(cancellationToken);
            return false;
        }
        if (status == (byte)SilkStallTransactionStatus.Refunded)
        {
            await transaction.CommitAsync(cancellationToken);
            return true;
        }
        if (status == null || status is not ((byte)SilkStallTransactionStatus.Reserved) and
            not ((byte)SilkStallTransactionStatus.RefundPending))
        {
            await transaction.RollbackAsync(cancellationToken);
            throw new InvalidOperationException(
                $"Silk Stall transaction {pending.TransactionId} cannot be refunded from status {status}.");
        }
        if (status == (byte)SilkStallTransactionStatus.Reserved)
        {
            var transitioned = await connection.ExecuteAsync(
                @"UPDATE dbo.Stall_SilkTransactions
                  SET Status = @Pending, FailureReason = @Reason, UpdatedAt = SYSUTCDATETIME()
                  WHERE ID = @ID AND Status = @Reserved",
                new
                {
                    ID = pending.TransactionId,
                    Pending = (byte)SilkStallTransactionStatus.RefundPending,
                    Reserved = (byte)SilkStallTransactionStatus.Reserved,
                    Reason = TruncateReason(reason)
                }, transaction);
            if (transitioned != 1)
            {
                await transaction.RollbackAsync(cancellationToken);
                throw new InvalidOperationException(
                    $"Silk Stall transaction {pending.TransactionId} could not transition to refund pending.");
            }
        }

        var refunded = await connection.ExecuteAsync(
            $@"UPDATE {accountDb}..SK_Silk
               SET silk_own = silk_own + @Price
               WHERE JID = @JID AND silk_own <= @MaximumBeforeCredit",
            new
            {
                Price = pending.SilkAmount,
                JID = pending.BuyerJid,
                MaximumBeforeCredit = int.MaxValue - pending.SilkAmount
            }, transaction);
        if (refunded == 1)
        {
            var finalized = await connection.ExecuteAsync(
                @"UPDATE dbo.Stall_SilkTransactions
                  SET Status = @Refunded, RefundedAt = SYSUTCDATETIME(), UpdatedAt = SYSUTCDATETIME()
                  WHERE ID = @ID AND Status = @Pending",
                new
                {
                    ID = pending.TransactionId,
                    Refunded = (byte)SilkStallTransactionStatus.Refunded,
                    Pending = (byte)SilkStallTransactionStatus.RefundPending
                }, transaction);
            if (finalized != 1)
            {
                await transaction.RollbackAsync(cancellationToken);
                throw new InvalidOperationException(
                    $"Silk Stall transaction {pending.TransactionId} could not be finalized after buyer refund.");
            }
        }

        await transaction.CommitAsync(cancellationToken);
        return refunded == 1;
    }

    private static async Task RecoveryLoopAsync(CancellationToken cancellationToken)
    {
        using var timer = new PeriodicTimer(RecoveryInterval);
        while (await timer.WaitForNextTickAsync(cancellationToken))
        {
            try
            {
                await RecoverDurableTransactionsAsync(recoverAllReserved: false, cancellationToken);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                return;
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Silk Stall durable recovery pass failed");
            }
        }
    }

    private static async Task RecoverDurableTransactionsAsync(bool recoverAllReserved,
        CancellationToken cancellationToken)
    {
        // A GameServer success already observed by this process must be made
        // durable before generic abandoned-reservation recovery can refund it.
        foreach (var pending in ActionManager.PendingSilkStallPurchases.Values
                     .Where(static value => Volatile.Read(ref value.SettlementState) == 1)
                     .ToArray())
        {
            if (await SettleAcceptedSaleAsync(pending, cancellationToken))
                await RefreshOnlineBalancesAsync(pending);
            FinishPending(pending);
        }

        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);

        var ageClause = recoverAllReserved ? string.Empty : "AND ReservedAt < DATEADD(SECOND, -90, SYSUTCDATETIME())";
        await connection.ExecuteAsync(
            $@"UPDATE dbo.Stall_SilkTransactions
               SET Status = @RefundPending, FailureReason = COALESCE(FailureReason, @Reason),
                   UpdatedAt = SYSUTCDATETIME()
               WHERE Status = @Reserved {ageClause}",
            new
            {
                RefundPending = (byte)SilkStallTransactionStatus.RefundPending,
                Reserved = (byte)SilkStallTransactionStatus.Reserved,
                Reason = "filter recovery: purchase response not confirmed"
            });

        var rows = (await connection.QueryAsync<RecoveryRow>(
            @"SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
              SELECT TOP (200) ID, BuyerJID, BuyerUniqueID, SellerJID, SellerUniqueID, SilkAmount, Status
              FROM dbo.Stall_SilkTransactions WITH (READPAST, READCOMMITTEDLOCK)
              WHERE Status IN (@Accepted, @RefundPending)
              ORDER BY UpdatedAt, ID",
            new
            {
                Accepted = (byte)SilkStallTransactionStatus.SaleAccepted,
                RefundPending = (byte)SilkStallTransactionStatus.RefundPending
            })).ToArray();

        foreach (var row in rows)
        {
            await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(
                IsolationLevel.Serializable, cancellationToken);
            var current = await connection.ExecuteScalarAsync<byte?>(
                @"SELECT Status FROM dbo.Stall_SilkTransactions WITH (UPDLOCK, HOLDLOCK) WHERE ID = @ID",
                new { row.ID }, transaction);
            if (current != row.Status)
            {
                await transaction.RollbackAsync(cancellationToken);
                continue;
            }

            var recipientJid = row.Status == (byte)SilkStallTransactionStatus.SaleAccepted
                ? row.SellerJID
                : row.BuyerJID;
            var credited = await connection.ExecuteAsync(
                $@"UPDATE {accountDb}..SK_Silk
                   SET silk_own = silk_own + @Price
                   WHERE JID = @JID AND silk_own <= @MaximumBeforeCredit",
                new
                {
                    Price = row.SilkAmount,
                    JID = recipientJid,
                    MaximumBeforeCredit = int.MaxValue - row.SilkAmount
                }, transaction);
            if (credited == 1)
            {
                var finalStatus = row.Status == (byte)SilkStallTransactionStatus.SaleAccepted
                    ? SilkStallTransactionStatus.Completed
                    : SilkStallTransactionStatus.Refunded;
                var finalized = await connection.ExecuteAsync(
                    @"UPDATE dbo.Stall_SilkTransactions
                      SET Status = @FinalStatus,
                          CompletedAt = CASE WHEN @FinalStatus = 1 THEN SYSUTCDATETIME() ELSE CompletedAt END,
                          RefundedAt = CASE WHEN @FinalStatus = 2 THEN SYSUTCDATETIME() ELSE RefundedAt END,
                          UpdatedAt = SYSUTCDATETIME()
                      WHERE ID = @ID AND Status = @OriginalStatus",
                    new { FinalStatus = (byte)finalStatus, row.ID, OriginalStatus = row.Status }, transaction);
                if (finalized != 1)
                {
                    await transaction.RollbackAsync(cancellationToken);
                    continue;
                }
            }

            await transaction.CommitAsync(cancellationToken);
            if (credited == 1)
            {
                var uniqueId = row.Status == (byte)SilkStallTransactionStatus.SaleAccepted
                    ? row.SellerUniqueID
                    : row.BuyerUniqueID;
                var online = FindOnlineSession((uint)uniqueId);
                if (online != null)
                    await RefreshSessionBalanceAsync(online);
            }
        }
    }

    private static void QueueAcceptedSettlement(PendingSilkStallPurchase pending) =>
        QueueBackground(pending.TransactionId, async () =>
        {
            for (var attempt = 0; attempt < 5; attempt++)
            {
                try
                {
                    if (await SettleAcceptedSaleAsync(pending, CancellationToken.None))
                    {
                        FinishPending(pending);
                        await RefreshOnlineBalancesAsync(pending);
                        return;
                    }
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Retry {Attempt} failed for accepted Silk Stall transaction {TransactionId}",
                        attempt + 1, pending.TransactionId);
                }
                await Task.Delay(TimeSpan.FromSeconds(2 + attempt * 2));
            }
            // Keep the accepted in-memory outcome authoritative. The periodic
            // recovery pass will continue until it is written as status 4/1.
        });

    private static void QueueRefund(PendingSilkStallPurchase pending, string reason) =>
        QueueBackground(pending.TransactionId, async () =>
        {
            for (var attempt = 0; attempt < 5; attempt++)
            {
                try
                {
                    if (Interlocked.CompareExchange(ref pending.SettlementState, 2, 0) is 0 or 2 &&
                        await SettleRefundAsync(pending, reason, CancellationToken.None))
                    {
                        FinishPending(pending);
                        await RefreshOnlineBalancesAsync(pending);
                        return;
                    }
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Retry {Attempt} failed for Silk Stall refund {TransactionId}",
                        attempt + 1, pending.TransactionId);
                }
                await Task.Delay(TimeSpan.FromSeconds(2 + attempt * 2));
            }
            FinishPending(pending);
        });

    private static void QueueRecovery(long transactionId) =>
        QueueBackground(transactionId, () => RecoverDurableTransactionsAsync(false, CancellationToken.None));

    private static void QueueBackground(long transactionId, Func<Task> work)
    {
        var task = Task.Run(work);
        BackgroundSettlements[transactionId] = task;
        _ = task.ContinueWith(
            completedTask => BackgroundSettlements.TryRemove(transactionId, out _),
            CancellationToken.None,
            TaskContinuationOptions.ExecuteSynchronously,
            TaskScheduler.Default);
    }

    private static void RemoveSoldSlot(PendingSilkStallPurchase pending)
    {
        ActionManager.RemoveStallSlot(pending.SellerUniqueId, pending.StallSlot);
        if (ActionManager.SilkStalls.TryGetValue(pending.SellerUniqueId, out var stall))
            stall.StallPrices.TryRemove(pending.StallSlot, out _);
    }

    private static void FinishPending(PendingSilkStallPurchase pending)
    {
        if (ActionManager.PendingSilkStallPurchases.TryGetValue(pending.BuyerClientGuid, out var current) &&
            current.TransactionId == pending.TransactionId)
            ActionManager.PendingSilkStallPurchases.TryRemove(pending.BuyerClientGuid, out _);
        ReservedSlots.TryRemove(GetSlotKey(pending.SellerUniqueId, pending.StallSlot), out _);
    }

    private static async Task RefreshOnlineBalancesAsync(PendingSilkStallPurchase pending)
    {
        var buyer = FindOnlineSession(pending.BuyerUniqueId);
        var seller = FindOnlineSession(pending.SellerUniqueId);
        if (buyer != null)
            await RefreshSessionBalanceAsync(buyer);
        if (seller != null)
            await RefreshSessionBalanceAsync(seller);
    }

    private static async Task RefreshSessionBalanceAsync(ISession session)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        var balance = await connection.QuerySingleOrDefaultAsync<SilkStallBalance>(
            $@"SELECT TOP (1) silk_own, silk_gift, silk_point FROM {accountDb}..SK_Silk WHERE JID = @JID",
            new { JID = session.SessionData.JID });
        if (balance != null)
            await SendSilkBalanceToGameServerAsync(session, balance);
    }

    private static Task<SilkStallBalance?> GetSilkBalanceForUpdateAsync(
        SqlConnection connection, SqlTransaction transaction, string accountDb, int jid) =>
        connection.QuerySingleOrDefaultAsync<SilkStallBalance>(
            $@"SELECT TOP (1) silk_own, silk_gift, silk_point
               FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK) WHERE JID = @JID",
            new { JID = jid }, transaction);

    private static async Task SendSilkBalanceToGameServerAsync(ISession session, SilkStallBalance balance)
    {
        if (session.IsStopped)
            return;
        var packet = new Packet(0x3527);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteInt32(balance.silk_own);
        packet.WriteInt32(balance.silk_gift);
        packet.WriteInt32(balance.silk_point);
        await session.SendToServer(packet);
    }

    private static ulong GetSlotKey(uint sellerUniqueId, byte stallSlot) =>
        ((ulong)sellerUniqueId << 8) | stallSlot;

    internal static bool IsValidSlotDefinition(byte stallSlot, ushort quantity, ulong silkPrice) =>
        stallSlot <= 9 && quantity > 0 && silkPrice is > 0 and <= int.MaxValue;

    internal static bool CanCreditSilkBalance(int currentBalance, int amount) =>
        currentBalance >= 0 && amount > 0 && currentBalance <= int.MaxValue - amount;

    internal static Packet BuildGameServerPreparationPacket(string sessionKey,
        PendingSilkStallPurchase pending)
    {
        var packet = new Packet(GameServerPrepareBuyOpcode);
        packet.WriteAscii(sessionKey);
        packet.WriteInt64(pending.TransactionId);
        packet.WriteUInt32(pending.SellerUniqueId);
        packet.WriteUInt8(pending.StallSlot);
        packet.WriteInt32(pending.SilkAmount);
        return packet;
    }

    private static bool SameSlot(SilkStallSlot left, SilkStallSlot right) =>
        left.StallSlot == right.StallSlot && left.InventorySlot == right.InventorySlot &&
        left.Quantity == right.Quantity && left.SilkPrice == right.SilkPrice && left.Tid == right.Tid;

    private static string TruncateReason(string reason) =>
        string.IsNullOrWhiteSpace(reason) ? "unspecified" : reason[..Math.Min(reason.Length, 256)];

    private sealed class RecoveryRow
    {
        public long ID { get; init; }
        public int BuyerJID { get; init; }
        public long BuyerUniqueID { get; init; }
        public int SellerJID { get; init; }
        public long SellerUniqueID { get; init; }
        public int SilkAmount { get; init; }
        public byte Status { get; init; }
    }
}
