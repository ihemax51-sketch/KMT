using System.Data;
using Dapper;
using KMTGuard.Database.Models;
using Microsoft.Data.SqlClient;

namespace KMTGuard.Features.Attendance;

internal static class AttendanceService
{
    internal const int CycleDays = 35;
    internal const int MaxRewards = 50;

    internal static async Task<IReadOnlyList<_RefAttendanceReward>> LoadRewardsAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        var rewards = (await connection.QueryAsync<_RefAttendanceReward>(
            @"SELECT ID, ItemID, ItemCodeName128, ItemCount, DayCount
              FROM dbo.Attendance_Rewards
              ORDER BY DayCount, ID")).AsList();

        ValidateRewards(rewards);
        return rewards;
    }

    internal static async Task<AttendanceState> GetStateAsync(int charId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        using var results = await connection.QueryMultipleAsync(
            "dbo.Attendance_GetState",
            new { CharID = charId },
            commandType: CommandType.StoredProcedure,
            commandTimeout: 60);

        var state = await results.ReadSingleAsync<AttendanceStateRow>();
        var eligibleRewardIds = (await results.ReadAsync<int>()).AsList();
        ValidateState(state.DayCount, eligibleRewardIds);

        return new AttendanceState(
            state.DayCount,
            state.LastAttendedDate,
            eligibleRewardIds);
    }

    internal static async Task<AttendanceRecordResult> RecordAsync(int charId)
    {
        var parameters = new DynamicParameters();
        parameters.Add("@CharID", charId, DbType.Int32);
        parameters.Add("@Result", dbType: DbType.Int32, direction: ParameterDirection.Output);
        parameters.Add("@DayCount", dbType: DbType.Int32, direction: ParameterDirection.Output);
        parameters.Add("@AttendanceDate", dbType: DbType.Date, direction: ParameterDirection.Output);

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(
            "dbo.Attendance_Record",
            parameters,
            commandType: CommandType.StoredProcedure,
            commandTimeout: 60);

        var dayCount = parameters.Get<int>("@DayCount");
        if (dayCount is < 0 or > CycleDays)
            throw new InvalidOperationException($"Attendance returned invalid DayCount {dayCount}.");

        return new AttendanceRecordResult(
            parameters.Get<int>("@Result"),
            dayCount,
            parameters.Get<DateTime>("@AttendanceDate"));
    }

    internal static async Task<AttendanceClaimResult> ClaimRewardAsync(int charId, int refRewardId)
    {
        var parameters = new DynamicParameters();
        parameters.Add("@CharID", charId, DbType.Int32);
        parameters.Add("@RefRewardID", refRewardId, DbType.Int32);
        parameters.Add("@Result", dbType: DbType.Int32, direction: ParameterDirection.Output);
        parameters.Add("@ItemID", dbType: DbType.Int32, direction: ParameterDirection.Output);
        parameters.Add("@ItemCount", dbType: DbType.Int32, direction: ParameterDirection.Output);

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(
            "dbo.Attendance_ClaimReward",
            parameters,
            commandType: CommandType.StoredProcedure,
            commandTimeout: 60);

        return new AttendanceClaimResult(
            parameters.Get<int>("@Result"),
            parameters.Get<int>("@ItemID"),
            parameters.Get<int>("@ItemCount"));
    }

    private static void ValidateRewards(IReadOnlyCollection<_RefAttendanceReward> rewards)
    {
        if (rewards.Count > MaxRewards)
            throw new InvalidOperationException(
                $"Attendance has {rewards.Count} rewards, but the client supports only {MaxRewards}.");

        foreach (var reward in rewards)
        {
            if (reward.ID <= 0 ||
                reward.ItemID <= 0 ||
                reward.ItemCount <= 0 ||
                reward.DayCount is < 1 or > CycleDays ||
                string.IsNullOrWhiteSpace(reward.ItemCodeName128))
            {
                throw new InvalidOperationException(
                    $"Attendance reward {reward.ID} has invalid configuration.");
            }
        }
    }

    private static void ValidateState(int dayCount, IReadOnlyCollection<int> eligibleRewardIds)
    {
        if (dayCount is < 0 or > CycleDays)
            throw new InvalidOperationException($"Attendance state contains invalid DayCount {dayCount}.");

        if (eligibleRewardIds.Count > MaxRewards)
            throw new InvalidOperationException(
                $"Attendance state contains more than {MaxRewards} eligible rewards.");
    }

    private sealed class AttendanceStateRow
    {
        public int DayCount { get; init; }
        public DateTime? LastAttendedDate { get; init; }
    }
}

internal sealed record AttendanceState(
    int DayCount,
    DateTime? LastAttendedDate,
    IReadOnlyList<int> EligibleRewardIds);

internal sealed record AttendanceRecordResult(
    int Result,
    int DayCount,
    DateTime AttendanceDate);

internal sealed record AttendanceClaimResult(
    int Result,
    int ItemID,
    int ItemCount);
