using System.Collections.Concurrent;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.RuntimeContract;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;

namespace KMTGuard.Server;

public static class OfflineStallGatewayBridge
{
    private static readonly ConcurrentDictionary<string, Guid> AutomaticLoginByAccount =
        new(StringComparer.OrdinalIgnoreCase);

    public static async Task<bool> TryReplaceOfflineStallLoginAsync(
        ISession session,
        string username,
        string password,
        Func<Task> replayAsync,
        bool credentialsAlreadyValidated = false)
    {
        if (!_serverSettings.EnableOfflineStall)
            return false;

        if (!AutomaticLoginByAccount.TryAdd(username, session.ClientGuid))
        {
            session.Stop("another Offline Stall automatic login is in progress");
            return true;
        }

        try
        {
            var lookup = await LookupAsync(username);
            if (!lookup.IsActive)
                return false;

            if (!credentialsAlreadyValidated && !await sqlQueryHelper.ValidateUserCredentials(username, password))
                return false;
            if (session.IsStopped)
                return true;

            Log.Information(
                "Automatically replacing Offline Stall {CharName} for authenticated login {Username}",
                lookup.CharName,
                username);
            await TerminateAsync(username);
            if (!session.IsStopped)
                await replayAsync();
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Gateway could not coordinate Offline Stall login for {Username}", username);
            session.Stop("Offline Stall login coordination failed");
            return true;
        }
        finally
        {
            AutomaticLoginByAccount.TryRemove(new KeyValuePair<string, Guid>(
                username,
                session.ClientGuid));
        }

        return true;
    }

    private static async Task<OfflineStallLookupResponse> LookupAsync(string username)
    {
        if (global::Program.CurrentRole == FilterRole.All)
        {
            var active = OfflineStallService.TryGetActiveByUsername(username, out var charName);
            return new OfflineStallLookupResponse { IsActive = active, CharName = charName };
        }

        return await RuntimePipeClient.SendForPayloadAsync<OfflineStallLookupResponse>(
                   FilterRole.Agent,
                   "agent.offlineStall.lookup",
                   new OfflineStallAccountPayload { Username = username },
                   TimeSpan.FromSeconds(3))
               ?? throw new InvalidDataException("Agent returned an empty Offline Stall lookup response.");
    }

    private static async Task TerminateAsync(string username)
    {
        if (global::Program.CurrentRole == FilterRole.All)
        {
            if (!await OfflineStallService.TerminateByUsernameAsync(username, "authenticated login started"))
                throw new InvalidOperationException("The Offline Stall was no longer active when termination was requested.");
            return;
        }

        var response = await RuntimePipeClient.SendAsync(
            FilterRole.Agent,
            "agent.offlineStall.terminate",
            new OfflineStallAccountPayload { Username = username },
            TimeSpan.FromSeconds(5));
        if (!response.Success)
            throw new InvalidOperationException(response.Message);
    }

}
