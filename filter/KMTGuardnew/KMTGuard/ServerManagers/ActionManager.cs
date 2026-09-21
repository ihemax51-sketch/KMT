using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using KMTGuard.Database.ModelsEvents;

namespace KMTGuard.ServerManagers;
public class SCreatedKillCounterKillList
{
    public int WorldID { get; set; }
    public string CharName16 { get; set; } = string.Empty;
    public int Kill { get; set; }
};
public class SCreatedTeamKillCounterKillList
{
    public int WorldID { get; set; }
    public string CharName16 { get; set; } = string.Empty;
    public int Kill { get; set; }
    public int Team { get; set; }
};
public class SCreatedJobKillCounterKillList
{
    public int WorldID { get; set; }
    public string CharName16 { get; set; } = string.Empty;
    public int Kill { get; set; }
    public int Team { get; set; }
};
public class SilkStall : IDisposable
{
    public void Dispose()
    {
        StallPrices.Clear();
    }

    public ConcurrentDictionary<byte, SilkStallSlot> StallPrices { get; } = new();

    public int SellerJid { get; init; }
    public int SellerCharId { get; init; }
    public string SellerCharName { get; init; } = string.Empty;
    public uint SellerUniqueId { get; init; }
    public DateTime CreatedAt { get; init; } = DateTime.UtcNow;
}

public sealed class SilkStallSlot
{
    public byte StallSlot { get; init; }
    public byte InventorySlot { get; init; }
    public ushort Quantity { get; init; }
    public int SilkPrice { get; init; }
    public uint Tid { get; init; }
    public DateTime UpdatedAt { get; init; } = DateTime.UtcNow;
}

public sealed class PendingSilkStallPurchase
{
    public long TransactionId { get; init; }
    public Guid RequestToken { get; init; }
    public Guid BuyerClientGuid { get; init; }
    public int BuyerJid { get; init; }
    public int BuyerCharId { get; init; }
    public string BuyerCharName { get; init; } = string.Empty;
    public uint BuyerUniqueId { get; init; }
    public int SellerJid { get; init; }
    public int SellerCharId { get; init; }
    public string SellerCharName { get; init; } = string.Empty;
    public uint SellerUniqueId { get; init; }
    public byte StallSlot { get; init; }
    public byte InventorySlot { get; init; }
    public ushort Quantity { get; init; }
    public uint Tid { get; init; }
    public int SilkAmount { get; init; }
    public DateTime ReservedAt { get; init; } = DateTime.UtcNow;
    public int SettlementState;
}

public sealed class OpenStall
{
    public uint UniqueId { get; init; }
    public int Jid { get; init; }
    public int CharId { get; init; }
    public string CharName { get; init; } = string.Empty;
    public DateTime OpenedAtUtc { get; init; } = DateTime.UtcNow;
    public volatile bool IsOperating;
    public ConcurrentDictionary<byte, byte> Slots { get; } = new();
}

public sealed record PendingStallAction(
    byte ActionType,
    byte StallSlot,
    bool PreviousOperating,
    SilkStallSlot? SilkSlot);

public enum SilkStallTransactionStatus : byte
{
    Reserved = 0,
    Completed = 1,
    Refunded = 2,
    Failed = 3,
    SaleAccepted = 4,
    RefundPending = 5
}

public sealed class SilkStallBalance
{
    public int silk_own { get; set; }
    public int silk_gift { get; set; }
    public int silk_point { get; set; }
}

public class ActionManager
{
    public static ConcurrentDictionary<uint, OpenStall> OpenStalls { get; } = new();
    private static ConcurrentDictionary<uint, ConcurrentDictionary<byte, byte>> PendingStallSlots { get; } = new();
    private static ConcurrentDictionary<Guid, ConcurrentQueue<PendingStallAction>> PendingStallActions { get; } = new();
    public static ConcurrentDictionary<uint, SilkStall> SilkStalls { get; } = new();
    public static ConcurrentDictionary<Guid, PendingSilkStallPurchase> PendingSilkStallPurchases { get; } = new();

    [Obsolete("Use SilkStalls instead.")]
    public static ConcurrentDictionary<uint, SilkStall> stallData => SilkStalls;

    public static void OpenSilkStall(uint uniqueId, int jid, int charId, string charName)
    {
        OpenStall(uniqueId, jid, charId, charName);
        SilkStalls[uniqueId] = new SilkStall
        {
            SellerUniqueId = uniqueId,
            SellerJid = jid,
            SellerCharId = charId,
            SellerCharName = charName
        };
    }

    public static void CloseSilkStall(uint uniqueId)
    {
        if (SilkStalls.TryRemove(uniqueId, out var stall))
            stall.Dispose();
    }

    public static void OpenStall(uint uniqueId, int jid, int charId, string charName)
    {
        if (uniqueId == 0 || charId <= 0)
            return;

        var stall = OpenStalls.AddOrUpdate(
            uniqueId,
            _ => new OpenStall
            {
                UniqueId = uniqueId,
                Jid = jid,
                CharId = charId,
                CharName = charName
            },
            (_, existing) => existing);

        if (PendingStallSlots.TryGetValue(uniqueId, out var pendingSlots))
        {
            foreach (var slot in pendingSlots.Keys)
                stall.Slots[slot] = slot;
        }
    }

    public static void TrackStallSlot(uint uniqueId, byte slot)
    {
        if (uniqueId == 0)
            return;

        PendingStallSlots.GetOrAdd(uniqueId, _ => new ConcurrentDictionary<byte, byte>())[slot] = slot;
        if (OpenStalls.TryGetValue(uniqueId, out var stall))
            stall.Slots[slot] = slot;
    }

    public static int RemoveStallSlot(uint uniqueId, byte slot)
    {
        if (uniqueId == 0)
            return -1;

        if (PendingStallSlots.TryGetValue(uniqueId, out var pendingSlots))
            pendingSlots.TryRemove(slot, out _);
        if (!OpenStalls.TryGetValue(uniqueId, out var stall))
            return pendingSlots?.Count ?? -1;

        stall.Slots.TryRemove(slot, out _);
        return stall.Slots.Count;
    }

    public static void SetStallOperating(uint uniqueId, bool isOperating)
    {
        if (uniqueId != 0 && OpenStalls.TryGetValue(uniqueId, out var stall))
            stall.IsOperating = isOperating;
    }

    public static void QueueStallAction(Guid sessionId, PendingStallAction action)
    {
        PendingStallActions.GetOrAdd(sessionId, static _ => new ConcurrentQueue<PendingStallAction>())
            .Enqueue(action);
    }

    public static bool TryTakeStallAction(Guid sessionId, byte responseActionType, out PendingStallAction? action)
    {
        action = null;
        if (!PendingStallActions.TryGetValue(sessionId, out var queue) || !queue.TryPeek(out var pending))
            return false;

        if (pending.ActionType != responseActionType)
        {
            action = pending;
            PendingStallActions.TryRemove(sessionId, out _);
            return false;
        }

        if (!queue.TryDequeue(out pending))
            return false;

        if (queue.IsEmpty)
            PendingStallActions.TryRemove(new KeyValuePair<Guid, ConcurrentQueue<PendingStallAction>>(sessionId, queue));

        action = pending;
        return true;
    }

    public static void CancelPendingStallActions(Guid sessionId) =>
        PendingStallActions.TryRemove(sessionId, out _);

    public static void CloseStall(uint uniqueId)
    {
        if (uniqueId == 0)
            return;

        OpenStalls.TryRemove(uniqueId, out _);
        PendingStallSlots.TryRemove(uniqueId, out _);
        CloseSilkStall(uniqueId);
    }

    public static ConcurrentDictionary<int, bool> m_CanAttackbyregionId { get; private set; } = new();
    public static ConcurrentDictionary<int, bool> m_CanAttackbyWorldId { get; private set; } = new();
    public static ConcurrentDictionary<int, bool> m_CanTeleport { get; } = new();
    public static ConcurrentDictionary<int, int> CreatedTimerListWorldID = new ConcurrentDictionary<int, int>();
    public static ConcurrentDictionary<int, int> CreatedTimerListRegionID = new ConcurrentDictionary<int, int>();

    public static ConcurrentDictionary<int, string> CreatedKillCounterWorldID = new ConcurrentDictionary<int, string>();
    public static ConcurrentDictionary<int, byte> CreatedFullKillCounterWorldID = new ConcurrentDictionary<int, byte>();
    public static ConcurrentDictionary<string, SCreatedKillCounterKillList> KillCounterKillList = new ConcurrentDictionary<string, SCreatedKillCounterKillList>();

    public static ConcurrentDictionary<int, string> CreatedTeamKillCounterWorldID = new ConcurrentDictionary<int, string>();
    public static ConcurrentDictionary<string, SCreatedTeamKillCounterKillList> TeamKillCounterKillList = new ConcurrentDictionary<string, SCreatedTeamKillCounterKillList>();

    public static ConcurrentDictionary<int, string> CreatedJobKillCounterWorldID = new ConcurrentDictionary<int, string>();
    public static ConcurrentDictionary<string, SCreatedJobKillCounterKillList> JobKillCounterWorldID = new ConcurrentDictionary<string, SCreatedJobKillCounterKillList>();
}
