using System;
using System.Data;
using System.Threading.Tasks;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.ServerManagers;

public static class JobControlService
{
    private const int ProcedureTimeoutSeconds = 10;

    public static async Task InitializeAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var command = new SqlCommand(@"
SELECT COUNT(*)
  FROM sys.procedures
 WHERE schema_id=SCHEMA_ID(N'dbo')
   AND name IN (N'_JobJoin',N'_JobLeave',N'_JobSuitEquip',N'_JobSuitRemove');", connection)
        {
            CommandTimeout = ProcedureTimeoutSeconds
        };
        if (Convert.ToInt32(await command.ExecuteScalarAsync()) != 4)
            throw new InvalidOperationException(
                "Job control procedures are missing. Apply the packaged database updates before startup.");
        Log.Information("Job control procedure schema validated read-only.");
    }

    public static Task<bool> CanJoinAsync(ISession session, byte jobType)
    {
        return ExecuteAsync(
            "_JobJoin",
            session,
            command => command.Parameters.Add("@JobType", SqlDbType.TinyInt).Value = jobType);
    }

    public static Task<bool> CanLeaveAsync(ISession session)
    {
        return ExecuteAsync("_JobLeave", session);
    }

    public static Task<bool> CanEquipSuitAsync(ISession session, int suitItemId)
    {
        return ExecuteAsync(
            "_JobSuitEquip",
            session,
            command => command.Parameters.Add("@SuitItemID", SqlDbType.Int).Value = suitItemId);
    }

    public static Task<bool> CanRemoveSuitAsync(ISession session)
    {
        return ExecuteAsync("_JobSuitRemove", session);
    }

    private static async Task<bool> ExecuteAsync(
        string procedureName,
        ISession session,
        Action<SqlCommand>? addParameters = null)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        await using var command = new SqlCommand($"[dbo].[{procedureName}]", connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = ProcedureTimeoutSeconds
        };

        command.Parameters.Add("@CharID", SqlDbType.Int).Value = session.SessionData.Charid;
        addParameters?.Invoke(command);

        var charName = command.Parameters.Add("@Charname", SqlDbType.VarChar, 25);
        charName.Value = session.SessionData.Charname ?? string.Empty;

        var returnValue = command.Parameters.Add("@RETURN_VALUE", SqlDbType.Int);
        returnValue.Direction = ParameterDirection.ReturnValue;

        await command.ExecuteNonQueryAsync();

        int result = returnValue.Value == DBNull.Value
            ? 0
            : Convert.ToInt32(returnValue.Value);

        Log.Debug(
            "Job control procedure {ProcedureName} returned {Result} for CharID={CharID}.",
            procedureName,
            result,
            session.SessionData.Charid);

        return result == 1;
    }
}
