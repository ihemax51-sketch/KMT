using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Features.ItemChest;

internal static class ItemChestService
{
    internal const ushort SnapshotOpcode = 0x208D;
    internal const int SnapshotChunkSize = 50;
    internal const int MaximumChestEntries = 100000;

    internal static async Task SendSnapshotAsync(ISession session)
    {
        IReadOnlyList<_ItemChest> items;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            items = (await connection.QueryAsync<_ItemChest>(
                @"SELECT
                      C.ID,
                      C.CharID,
                      C.ItemCodeName,
                      C.ItemID,
                      C.Quantity,
                      C.[Date],
                      C.[Type],
                      CONVERT(TINYINT, C.Plus) AS Plus
                  FROM dbo.Item_Chest AS C WITH (READCOMMITTEDLOCK)
                  WHERE C.CharID = @CharID
                    AND NOT EXISTS
                    (
                        SELECT 1
                        FROM dbo.Item_ChestClaim AS ClaimRow WITH (READCOMMITTEDLOCK)
                        WHERE ClaimRow.ChestID = C.ID
                    )
                  ORDER BY C.ID",
                new { CharID = session.SessionData.Charid })).ToList();
        }
        catch (Exception ex)
        {
            Log.Error(
                ex,
                "Failed to load Item Chest snapshot for CharID {CharID}",
                session.SessionData.Charid);
            await SendWarningAsync(session, PlayerLanguage.Get("ItemChest.LoadFailed"));
            return;
        }

        if (items.Count > MaximumChestEntries)
        {
            Log.Error(
                "Item Chest entry limit exceeded for CharID {CharID}. Count={Count}, Limit={Limit}",
                session.SessionData.Charid,
                items.Count,
                MaximumChestEntries);
            await SendWarningAsync(session, PlayerLanguage.Get("ItemChest.TooManyEntries"));
            return;
        }

        session.SessionData.CharacterChest.Clear();
        foreach (var item in items)
            session.SessionData.CharacterChest.TryAdd(item.ID, item);

        var begin = new Packet(SnapshotOpcode);
        begin.WriteUInt8(0);
        begin.WriteInt32(items.Count);
        await session.SendToClient(begin);

        for (var offset = 0; offset < items.Count; offset += SnapshotChunkSize)
        {
            var count = Math.Min(SnapshotChunkSize, items.Count - offset);
            var chunk = new Packet(SnapshotOpcode);
            chunk.WriteUInt8(1);
            chunk.WriteUInt8((byte)count);

            for (var index = 0; index < count; index++)
                WriteItem(chunk, items[offset + index]);

            await session.SendToClient(chunk);
        }

        var end = new Packet(SnapshotOpcode);
        end.WriteUInt8(2);
        await session.SendToClient(end);
    }

    internal static async Task BeginClaimAsync(ISession session, int chestId)
    {
        if (chestId <= 0 || session.SessionData.Charid <= 0 || !session.CharacterGameReady)
        {
            await SendWarningAsync(session, PlayerLanguage.Get("ItemChest.InvalidRequest"));
            return;
        }

        var claimToken = Guid.NewGuid();
        if (!session.SessionData.PendingChestClaims.TryAdd(chestId, claimToken))
        {
            await SendWarningAsync(session, PlayerLanguage.Get("ItemChest.AlreadyProcessing"));
            return;
        }

        _ItemChest? item;
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            item = await connection.QuerySingleOrDefaultAsync<_ItemChest>(
                "EXEC dbo.Item_ChestProcessClaim @Action, @ChestID, @CharID, @ClaimToken",
                new
                {
                    Action = "BEGIN",
                    CharID = session.SessionData.Charid,
                    ChestID = chestId,
                    ClaimToken = claimToken
                });
        }
        catch (Exception ex)
        {
            session.SessionData.PendingChestClaims.TryRemove(chestId, out _);
            Log.Error(
                ex,
                "Failed to begin Item Chest claim. CharID={CharID}, ChestID={ChestID}",
                session.SessionData.Charid,
                chestId);
            await SendWarningAsync(session, PlayerLanguage.Get("ItemChest.ClaimStartFailed"));
            await SendSnapshotAsync(session);
            return;
        }

        if (item == null || !IsValidItem(item))
        {
            session.SessionData.PendingChestClaims.TryRemove(chestId, out _);
            await SendWarningAsync(
                session,
                item == null
                    ? PlayerLanguage.Get("ItemChest.MissingOrProcessing")
                    : PlayerLanguage.Get("ItemChest.InvalidData"));
            await SendSnapshotAsync(session);
            return;
        }

        session.SessionData.CharacterChest[item.ID] = item;

        var request = new Packet(0x3506);
        request.WriteAscii(session.GameServerPacketKey);
        request.WriteInt32(item.ID);
        request.WriteAscii(item.ItemCodeName);
        request.WriteInt32(item.Quantity);
        request.WriteUInt8(0);
        request.WriteUInt8(item.Plus);
        await session.SendToServer(request);
    }

    internal static async Task CompleteClaimAsync(ISession session, int chestId, bool succeeded)
    {
        if (!session.SessionData.PendingChestClaims.TryGetValue(chestId, out var claimToken))
        {
            Log.Warning(
                "Ignored Item Chest result without a pending claim. CharID={CharID}, ChestID={ChestID}, Succeeded={Succeeded}",
                session.SessionData.Charid,
                chestId,
                succeeded);
            return;
        }

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync(
                "EXEC dbo.Item_ChestProcessClaim @Action, @ChestID, @CharID, @ClaimToken, @Reason",
                new
                {
                    Action = succeeded ? "SUCCESS" : "FAIL",
                    CharID = session.SessionData.Charid,
                    ChestID = chestId,
                    ClaimToken = claimToken,
                    Reason = succeeded ? null : "GameServer rejected AddItem."
                });
        }
        catch (Exception ex)
        {
            Log.Error(
                ex,
                "Failed to finalize Item Chest claim. CharID={CharID}, ChestID={ChestID}, Succeeded={Succeeded}",
                session.SessionData.Charid,
                chestId,
                succeeded);

            if (!succeeded)
                session.SessionData.PendingChestClaims.TryRemove(chestId, out _);

            await SendWarningAsync(
                session,
                succeeded
                    ? PlayerLanguage.Get("ItemChest.DeliveredNeedsReconciliation")
                    : PlayerLanguage.Get("ItemChest.NotDeliveredCannotCancel"));
            await SendSnapshotAsync(session);
            return;
        }

        session.SessionData.PendingChestClaims.TryRemove(chestId, out _);

        if (succeeded)
        {
            session.SessionData.CharacterChest.TryRemove(chestId, out _);
        }
        else
        {
            await SendWarningAsync(session, PlayerLanguage.Get("ItemChest.AddFailed"));
        }

        await SendSnapshotAsync(session);
    }

    internal static async Task SendWarningAsync(ISession session, string message)
    {
        var warning = new Packet(0x168A);
        warning.WriteUInt8(NoticeType.WARNING);
        warning.WriteUnicode(message);
        await session.SendToClient(warning);
    }

    internal static bool IsValidItem(_ItemChest item)
    {
        return item.ID > 0 &&
               item.CharID > 0 &&
               item.ItemID > 0 &&
               item.Quantity > 0 &&
               item.Quantity <= 1000000 &&
               !string.IsNullOrWhiteSpace(item.ItemCodeName);
    }

    private static void WriteItem(Packet packet, _ItemChest item)
    {
        packet.WriteInt32(item.ID);
        packet.WriteInt32(item.ItemID);
        packet.WriteInt32(item.Quantity);
        packet.WriteAscii(item.Date ?? string.Empty);
        packet.WriteAscii(item.Type ?? string.Empty);
        packet.WriteUInt8(item.Plus);
    }
}
