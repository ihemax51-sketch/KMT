using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using KMTGuard.AsyncServerManager;
using KMTGuard.Database.Models;
using KMTGuard.Database;
using KMTGuard.Clientless;
using KMTGuard.ConsoleUi;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Features.Telegram;
using KMTGuard.Features.Discord;
using KMTGuard.Helpers;
using KMTGuard.RuntimeContract;
using KMTGuard.Server.DownloadServer;
using KMTGuard.Server;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using SilkroadSecurityAPI;
using Dapper;
using Serilog;

namespace KMTGuard.ServerManagers
{
    public static class ServerManager
    {
        public static List<IAsyncServer> Servers { get; set; } = new();
        public static IReadOnlyList<__ProxyServices> ServiceCatalog { get; private set; } = Array.Empty<__ProxyServices>();
        public static ConcurrentSessionSet AgentSessions { get; set; } = new();
        public static ConcurrentSessionSet DownloadSessions { get; set; } = new();
        public static ConcurrentSessionSet GatewaySessions { get; set; } = new();
        public static string ConnectionStringShardDB { get; set; } = null!;
        public static DelayedJobManager g_DelayedJobMgr = new DelayedJobManager();
        public static FilterRole ActiveRole { get; private set; } = FilterRole.All;
        private static volatile bool _trackingConnections;

        // TeleportControl service

        public static bool IsOnlinePlayer(ISession? session)
        {
            return session != null &&
                   !session.IsStopped &&
                   !session.ClientDetached &&
                   session.CharacterGameReady &&
                   session.SessionData.Charid > 0 &&
                   !string.IsNullOrWhiteSpace(session.SessionData.Charname);
        }

        public static bool IsUpstreamOccupiedPlayer(ISession? session)
        {
            return session != null &&
                   !session.IsStopped &&
                   session.CharacterGameReady &&
                   session.SessionData.Charid > 0 &&
                   !string.IsNullOrWhiteSpace(session.SessionData.Charname);
        }

        public static bool IsClientDeliveryTarget(ISession? session, bool clientIsReady = true)
        {
            return session != null &&
                   !session.IsStopped &&
                   !session.ClientDetached &&
                   (!clientIsReady || session.CharacterGameReady);
        }

        public static int GetOnlinePlayerCount()
        {
            return AgentSessions.CountWhere(IsOnlinePlayer);
        }

        public static Task<int> GetIPCount(string ip)
        {
            return Task.FromResult(AgentSessions.CountWhere(x =>
                x.ClientIp == ip && !x.IsStopped && !x.IsManagedClientless));
        }

        public static Task<int> GetHWIDCount(string hwid)
        {
            return Task.FromResult(AgentSessions.CountWhere(x =>
                x.SessionData.Hwid == hwid && !x.IsStopped));
        }

        public static Task<int> GetArenaSessionCountForHwid(string hwid)
        {
            // Arena registration tracking is currently disabled.
            return Task.FromResult(0);
        }

        public static Task<string> GetCharnamebyUniqueId(int UniqueID)
        {
            if (UniqueID <= 0)
                return Task.FromResult(string.Empty);

            try
            {
                var result = AgentSessions.FindByUniqueCharId((uint)UniqueID);
                return Task.FromResult(result?.SessionData.Charname ?? string.Empty);
            }
            catch (Exception)
            {
                return Task.FromResult(string.Empty);
            }
        }

        private static bool IsInJob(ISession s)
        {
            // حسب استخدامك في الهاندلر: 1/2/3 = Job (Trader/Thief/Hunter), 4 = خارج الجوب/No Job
            var jt = s?.SessionData?.JobType ?? 0;
            return jt == 1 || jt == 2 || jt == 3;
        }

        public static Task<int> GetJobHWIDCount(string hwid)
        {
            return Task.FromResult(AgentSessions.CountWhere(x =>
                x.SessionData.Hwid == hwid && !x.IsStopped && !x.ClientDetached && IsInJob(x)));
        }

        public static Task<int> GetHWIDCountbyWorldID(string hwid, int WorldId)
        {
            return Task.FromResult(AgentSessions.CountWhere(x =>
                x.SessionData.Hwid == hwid && !x.IsStopped && !x.ClientDetached &&
                x.SessionData.WorldID == WorldId));
        }

        public static async Task InitialServers(FilterRole role = FilterRole.All)
        {
            ActiveRole = role;
            try
            {
                if (role != FilterRole.Download)
                    await _serverSettings.InitServerSettings();

                if (role is FilterRole.Agent or FilterRole.All)
                    await RefManager.Initialize();
                else if (role == FilterRole.Gateway)
                    await RefManager.InitializeGateway();

                AgentSessions = new ConcurrentSessionSet();
                DownloadSessions = new ConcurrentSessionSet();
                GatewaySessions = new ConcurrentSessionSet();

                if (role is FilterRole.Agent or FilterRole.All)
                {
                    await JobControlService.InitializeAsync();
                    await SilkStallService.InitializeAsync();
                    await OfflineStallService.InitializeAsync();
                    await TelegramNotificationService.InitializeAsync();
                    await DiscordNotificationService.InitializeAsync();
                    DatabaseCommands.InitializeTimer();
                    DatabaseCommands.InitializePlannedTimer();
                    await AutoEventService.InitializeAsync();
                    await Scheduler.InitializeSchedulerTimer();
                    TeleportFreezeService.Start();
                    await PvpChallengeService.InitializeAsync();
                }

                if (role is FilterRole.Agent or FilterRole.Gateway or FilterRole.All)
                    g_DelayedJobMgr.Run();

                Servers = new List<IAsyncServer>();
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    var proxyDb = SqlIdentifier.Quote(Program.ProxyDb);
                    ServiceCatalog = (await connection.QueryAsync<__ProxyServices>(
                        $"SELECT * FROM {proxyDb}.[dbo].[System_ProxyServices]"))
                        .ToArray();

                    foreach (var service in ServiceCatalog.Where(service => HostsRole(role, service.ServerType)))
                        AddServer(service);
                }

                if (Servers.Count == 0)
                    throw new InvalidOperationException($"No enabled proxy service rows are configured for the {role} role.");

                if (role == FilterRole.All)
                {
                    _trackingConnections = true;
                    var thread = new Thread(TrackServerConnections)
                    {
                        IsBackground = true,
                        Name = "KMTGuard-ConnectionTracker"
                    };
                    thread.Start();
                }

                await StartAsync(true);

                if (role is FilterRole.Agent or FilterRole.All)
                {
                    using var connection = new SqlConnection(Program.Connectionstring);
                    await connection.OpenAsync();
                    using var command = new SqlCommand("EXEC [dbo].[System_Start]", connection);
                    await command.ExecuteNonQueryAsync();
                    Log.Information("World bootstrap acknowledged :: control hook=System_Start");
                    await TelegramNotificationService.QueueServerOnlineAsync();
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Initial server startup failed for role {Role}", role);
                throw;
            }
        }

        private static bool HostsRole(FilterRole role, ServerType serverType)
        {
            return role == FilterRole.All ||
                   (role == FilterRole.Agent && serverType == ServerType.AgentServer) ||
                   (role == FilterRole.Download && serverType == ServerType.DownloadServer) ||
                   (role == FilterRole.Gateway && serverType == ServerType.GatewayServer);
        }

        public static __ProxyServices? FindConfiguredService(
            ServerType serverType,
            string? remoteIp = null,
            int? remotePort = null)
        {
            var candidates = ServiceCatalog.Where(service => service.ServerType == serverType);
            if (!string.IsNullOrWhiteSpace(remoteIp) && remotePort.HasValue)
            {
                var exact = candidates.FirstOrDefault(service =>
                    service.RemotePort == remotePort.Value &&
                    string.Equals(service.RemoteIP, remoteIp, StringComparison.OrdinalIgnoreCase));
                if (exact != null)
                    return exact;
            }

            return candidates.FirstOrDefault();
        }

        public static async Task sendNotice(ISession session, NoticeType type, string Text)
        {
            Packet stMsg = new Packet(0x168A);
            stMsg.WriteUInt8(type);
            stMsg.WriteUnicode(Text);
            await session.SendToClient(stMsg);
        }

        public async static Task BroadcastPacket(Packet packet, ServerType serverType = ServerType.AgentServer, bool clientIsReady = true)
        {
            var sessions = AgentSessions;
            var targets = sessions.Where(session => IsClientDeliveryTarget(session, clientIsReady));
            await Task.WhenAll(targets.Select(session => session.SendToClient(packet)));
        }

        public static async Task BroadcastPacketToCharName(string CharName, Packet packet, bool clientIsReady = true)
        {
            var sessions = AgentSessions;
            var targets = sessions.Where(targetSession =>
                IsClientDeliveryTarget(targetSession, clientIsReady) &&
                targetSession.SessionData?.Charname == CharName);
            await Task.WhenAll(targets.Select(targetSession => targetSession.SendToClient(packet)));
        }

        public static async Task BroadcastPacketbyWorldID(int WorldID, Packet packet, bool clientIsReady = true)
        {
            var sessions = AgentSessions;
            var targets = sessions.Where(targetSession =>
                IsClientDeliveryTarget(targetSession, clientIsReady) &&
                targetSession.SessionData?.WorldID == WorldID);
            await Task.WhenAll(targets.Select(targetSession => targetSession.SendToClient(packet)));
        }

        public static async Task BroadcastPacketbyRegionID(int Region, Packet packet, bool clientIsReady = true)
        {
            var sessions = AgentSessions;
            var targets = sessions.Where(targetSession =>
                IsClientDeliveryTarget(targetSession, clientIsReady) &&
                targetSession.SessionData?.LatestRegion == Region);
            await Task.WhenAll(targets.Select(targetSession => targetSession.SendToClient(packet)));
        }

        public static async Task RestoreWorldTimerOrCloseByRegionID(int regionId, bool clientIsReady = true)
        {
            var sessions = AgentSessions;
            var targets = sessions.Where(targetSession =>
                IsClientDeliveryTarget(targetSession, clientIsReady) &&
                targetSession.SessionData?.LatestRegion == regionId);

            await Task.WhenAll(targets.Select(async targetSession =>
            {
                Packet timer = new Packet(0x220A);
                if (ActionManager.CreatedTimerListWorldID.TryGetValue(
                        targetSession.SessionData.WorldID,
                        out int remainingMilliseconds) &&
                    remainingMilliseconds > 0)
                {
                    timer.WriteUInt8(0);
                    timer.WriteInt32(remainingMilliseconds);
                }
                else
                {
                    timer.WriteUInt8(1);
                }

                await targetSession.SendToClient(timer);
            }));
        }

        private static void TrackServerConnections()
        {
            while (_trackingConnections)
            {
                FilterConsole.SetConnectionTitle();
                Thread.Sleep(1000);
            }
        }

        public static void AddServer(__ProxyServices service)
        {
            switch (service.ServerType)
            {
                case ServerType.GatewayServer:
                    var gatewayServer = new GatewayServer(service);
                    Servers.Add(gatewayServer);
                    break;
                case ServerType.DownloadServer:
                    var downloadServer = new DownloadServer(service);
                    Servers.Add(downloadServer);
                    break;
                case ServerType.AgentServer:
                    var agentServer = new AgentServer(service);
                    Servers.Add(agentServer);
                    break;
                case ServerType.None:
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public static void Dispose()
        {
            _trackingConnections = false;

            if (ActiveRole is FilterRole.Agent or FilterRole.All)
                PvpChallengeService.BeginShutdown();

            // Stop ingress first so shutdown has a stable set of sessions to drain.
            foreach (var asyncServer in Servers)
                asyncServer.Dispose();

            if (ActiveRole is FilterRole.Agent or FilterRole.All)
            {
                try
                {
                    PvpChallengeService.ShutdownAsync("Filter shutdown").GetAwaiter().GetResult();
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "PvP Challenge shutdown recovery failed");
                }
            }

            var sessions = (GatewaySessions?.ToArray() ?? Array.Empty<ISession>())
                .Concat(DownloadSessions?.ToArray() ?? Array.Empty<ISession>())
                .Concat(AgentSessions?.ToArray() ?? Array.Empty<ISession>())
                .DistinctBy(session => session.ClientGuid)
                .ToArray();
            foreach (var session in sessions)
                session.Stop("filter shutdown");

            try
            {
                Task.WhenAll(sessions.Select(session => session.ShutdownCompletion))
                    .WaitAsync(TimeSpan.FromSeconds(15)).GetAwaiter().GetResult();
            }
            catch (TimeoutException)
            {
                Log.Warning(
                    "Session database cleanup did not drain within the 15 second shutdown deadline");
            }

            if (ActiveRole is FilterRole.Agent or FilterRole.All)
            {
                OfflineStallService.ShutdownAsync("filter shutdown").GetAwaiter().GetResult();
                SilkStallService.ShutdownAsync("filter shutdown").GetAwaiter().GetResult();
                DatabaseCommands.StopTimers();
                Scheduler.Stop();
                AutoEventService.Stop();
                TelegramNotificationService.Stop();
                DiscordNotificationService.Stop();
                RefManager.StopTimers();
                ClientlessManager.Stop();
                TeleportFreezeService.Stop();
                DatabaseJobQueue.StopAsync().GetAwaiter().GetResult();
            }

            if (ActiveRole is FilterRole.Agent or FilterRole.Gateway or FilterRole.All)
                g_DelayedJobMgr.Stop();

            Servers.Clear();
            ServiceCatalog = Array.Empty<__ProxyServices>();
        }

        public static void Stop(__ProxyServices service)
        {
            IAsyncServer? temp = null;
            foreach (var asyncServer in Servers.Where(asyncServer => asyncServer.Service.Equals(service)))
            {
                temp = asyncServer;
            }

            if (temp == null)
            {
                return;
            }

            temp.Dispose();
            Servers.Remove(temp);
        }

        public static void Start()
        {
            Start(false);
        }

        public static void Start(bool firstStart)
        {
            _ = StartAsync(firstStart);
        }

        private static async Task StartAsync(bool firstStart)
        {
            var targets = Servers.Where(asyncServer =>
                    (firstStart && asyncServer.Service.AutoStart) || !firstStart)
                .ToArray();
            if (targets.Length == 0)
                throw new InvalidOperationException($"No {ActiveRole} proxy service is marked AutoStart.");

            var startupTasks = targets
                .Select(server => (Server: server, Task: server.Start()))
                .ToArray();

            var deadline = DateTime.UtcNow.AddSeconds(15);
            while (DateTime.UtcNow < deadline)
            {
                if (startupTasks.All(item => item.Server.Started))
                    return;

                var failed = startupTasks.FirstOrDefault(item =>
                    item.Task.IsCompleted && !item.Server.Started);
                if (failed.Server != null)
                    throw new InvalidOperationException(
                        $"{failed.Server.Service.Name} failed to bind {failed.Server.Service.BindIP}:{failed.Server.Service.BindPort}.");

                await Task.Delay(50);
            }

            var pending = string.Join(", ", startupTasks
                .Where(item => !item.Server.Started)
                .Select(item => item.Server.Service.Name));
            throw new TimeoutException($"Timed out waiting for {ActiveRole} listeners: {pending}.");
        }

        public static void Start(string name)
        {
            Start(name, false);
        }

        public static void Start(string name, bool firstStart)
        {
            //foreach (var asyncServer in Servers
            //             .Where(asyncServer => asyncServer.Service.Name.ToLower().Equals(name.ToLower()))
            //             .Where(asyncServer => (firstStart && asyncServer.Service.AutoStart) || !firstStart))
            //{
            //    asyncServer.Start();
            //}
        }

        public static void Start(__ProxyServices service)
        {
            Start(service, false);
        }

        public static void Start(__ProxyServices service, bool firstStart)
        {
            foreach (var asyncServer in Servers.Where(asyncServer => asyncServer.Service.Equals(service))
                         .Where(asyncServer => (firstStart && asyncServer.Service.AutoStart) || !firstStart))
            {
                asyncServer.Start();
            }
        }
    }
}
