using System.Net;
using System.Net.Sockets;
using System.Collections.Concurrent;
using System.Threading;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;

namespace KMTGuard.AsyncServerManager
{
    public class AsyncServer : IAsyncServer
    {
        public __ProxyServices Service { get; init; }
        public bool Exit { get; set; }
        public bool Started { get; set; } = false;
        public TcpListener _tcpServer { get; set; } = null!;
        public IPEndPoint RemoteEndPoint { get; set; } = null!;
        public IPacketHandler PacketHandler { get; set; } = null!;
        private const int MaxAcceptsPerIpWindow = 120;
        private const int AcceptWindowMs = 10_000;
        private const int MaxActiveSessionsPerIpPerService = 80;
        private static readonly ConcurrentDictionary<string, AcceptRateInfo> AcceptRates = new();
        private static int _acceptCleanupCounter;
        private int _acceptFailureCount;

        private sealed class AcceptRateInfo
        {
            public int Count;
            public long WindowStarted = Environment.TickCount64;
        }

        protected AsyncServer(__ProxyServices service)
        {
            Service = service;
        }
        private bool IsPortAvailable(int port)
        {
            try
            {
                using (var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
                {
                    socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                    socket.Bind(new IPEndPoint(IPAddress.Loopback, port));
                    return true; // Port kullanılabilir
                }
            }
            catch (SocketException)
            {
                return false; // Port kullanımda
            }
        }

        public async Task Start()
        {
            try
            {
                // defines a binding endpoint
                var bindEndPoint = new IPEndPoint(IPAddress.Parse(Service.BindIP), Service.BindPort);
                RemoteEndPoint = new IPEndPoint(IPAddress.Parse(Service.RemoteIP), Service.RemotePort);

                _tcpServer = new TcpListener(bindEndPoint);
                _tcpServer.Start();

                Log.Information("Transport profile selected :: lane={LaneName} :: protocol={Protocol}",
                    Service.Name, Service.ServerType);
                Log.Information("Ingress socket opened :: lane={LaneName} :: endpoint={BindAddress}:{BindPort}",
                    Service.Name, Service.BindIP, Service.BindPort);
                Log.Information("Traffic route committed :: ingress={BindAddress}:{BindPort} => upstream={RemoteAddress}:{RemotePort}",
                    Service.BindIP, Service.BindPort, Service.RemoteIP, Service.RemotePort);


                Started = true;
                // accepts client connections
                while (!Exit)
                {
                    var tcpClient = await _tcpServer.AcceptTcpClientAsync();
                    _acceptFailureCount = 0;
                    if (tcpClient.Client.RemoteEndPoint is IPEndPoint clientEndPoint)
                    {
                        Log.Verbose("{Service} accepted TCP client {ClientEndPoint}", Service.Name, clientEndPoint);

                        var clientIp = clientEndPoint.Address.ToString();
                        if (!AllowAcceptedClient(clientIp))
                        {
                            Log.Warning("{Service} rejected TCP client {ClientIp}: emergency accept limit", Service.Name, clientIp);
                            tcpClient.Dispose();
                            continue;
                        }
                    }

                    _ = HandleAcceptedClientAsync(tcpClient);
                }
            }
            catch (ObjectDisposedException) when (Exit)
            {
                // Listener was stopped intentionally.
            }
            catch (SocketException) when (Exit)
            {
                // Listener was stopped intentionally.
            }
            catch (Exception ex)
            {
                Started = false;
                try { _tcpServer?.Stop(); } catch { }
                var failure = Interlocked.Increment(ref _acceptFailureCount);
                Log.Error(ex,
                    "{Service} accept loop failed ({Failure}/5); listener health is down",
                    Service.Name, failure);
                if (!Exit && failure <= 5)
                {
                    await Task.Delay(TimeSpan.FromSeconds(Math.Min(1 << (failure - 1), 10)));
                    await Start();
                    return;
                }

                if (!Exit)
                    Environment.FailFast(
                        $"{Service.Name} accept loop repeatedly failed; supervisor restart required.", ex);
            }
            finally
            {
                if (Exit)
                    Started = false;
            }
        }

        private bool AllowAcceptedClient(string clientIp)
        {
            if (string.IsNullOrWhiteSpace(clientIp))
                return true;

            if (IsTrustedLocalClient(clientIp))
                return true;

            if ((Interlocked.Increment(ref _acceptCleanupCounter) & 0xFF) == 0)
                CleanupExpiredAcceptRates();

            if (CountActiveSessions() >= GetSessionCap() ||
                CountActiveSessionsForIp(clientIp) >= MaxActiveSessionsPerIpPerService)
                return false;

            var rateKey = $"{Service.ServerType}:{clientIp}";
            var info = AcceptRates.GetOrAdd(rateKey, _ => new AcceptRateInfo());
            var now = Environment.TickCount64;
            var elapsed = now - Volatile.Read(ref info.WindowStarted);
            if (elapsed >= AcceptWindowMs)
            {
                Interlocked.Exchange(ref info.Count, 0);
                Volatile.Write(ref info.WindowStarted, now);
            }

            return Interlocked.Increment(ref info.Count) <= MaxAcceptsPerIpWindow;
        }

        private static void CleanupExpiredAcceptRates()
        {
            var now = Environment.TickCount64;
            foreach (var pair in AcceptRates)
            {
                if (now - Volatile.Read(ref pair.Value.WindowStarted) >= AcceptWindowMs * 2L)
                    AcceptRates.TryRemove(pair.Key, out _);
            }
        }

        private static bool IsTrustedLocalClient(string clientIp)
        {
            if (!IPAddress.TryParse(clientIp, out var ipAddress))
                return false;

            if (IPAddress.IsLoopback(ipAddress))
                return true;

            return IPAddress.TryParse(Program.MainMachineIP, out var mainMachineIp) &&
                   ipAddress.Equals(mainMachineIp);
        }

        private int CountActiveSessionsForIp(string clientIp)
        {
            ConcurrentSessionSet? sessions = Service.ServerType switch
            {
                ServerType.AgentServer => ServerManager.AgentSessions,
                ServerType.DownloadServer => ServerManager.DownloadSessions,
                ServerType.GatewayServer => ServerManager.GatewaySessions,
                _ => null
            };

            return sessions?.CountWhere(session =>
                session.ClientIp == clientIp && !session.IsStopped && !session.ClientDetached) ?? 0;
        }

        private int CountActiveSessions()
        {
            ConcurrentSessionSet? sessions = Service.ServerType switch
            {
                ServerType.AgentServer => ServerManager.AgentSessions,
                ServerType.DownloadServer => ServerManager.DownloadSessions,
                ServerType.GatewayServer => ServerManager.GatewaySessions,
                _ => null
            };
            return sessions?.Count ?? 0;
        }

        private int GetSessionCap() => Service.ServerType switch
        {
            ServerType.AgentServer => Program.RuntimeSettings.AgentSessionCap,
            ServerType.DownloadServer => Program.RuntimeSettings.DownloadSessionCap,
            ServerType.GatewayServer => Program.RuntimeSettings.GatewaySessionCap,
            _ => 1
        };

        private async Task HandleAcceptedClientAsync(TcpClient tcpClient)
        {
            try
            {
                tcpClient.NoDelay = true;
                var clientSession = new Session(tcpClient, this);
                AddSession(clientSession);
                await clientSession.Start();
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "{Service} failed to initialize an accepted client", Service.Name);
                tcpClient.Dispose();
            }
        }
        public void Stop()
        {
            try
            {
                Exit = true;
                _tcpServer?.Stop();
                Started = false;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Error stopping server {Service}", Service.Name);
            }
        }
        public virtual void Dispose()
        {
            Exit = true;
            _tcpServer?.Stop();
        }
        public async Task OnAccept(Task<TcpClient> task)
        {
            await HandleAcceptedClientAsync(await task);
        }

        public virtual void RemoveSession(ISession session)
        {
        }

        public virtual void AddSession(ISession session)
        {

        }


        public Task<int> GetHWIDCount(string hwid)
        {
            return ServerManager.GetHWIDCount(hwid);
        }

    }
}
