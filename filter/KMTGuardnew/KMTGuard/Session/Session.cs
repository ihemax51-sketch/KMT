using KMTGuard.AsyncServerManager;
using Dapper;
using KMTGuard.Helpers;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;
using System;
using System.Collections.Concurrent;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Security.Cryptography;
using static System.Collections.Specialized.BitVector32;

namespace KMTGuard.SessionManager
{
    public sealed class Session : ISession
    {
        private readonly byte[] _clientBuffer = new byte[4096];
        private readonly Security _clientSecurity = new(maxMassiveFragments: 256, maxMassiveBytes: 1024 * 1024);

        private readonly byte[] _serverBuffer = new byte[4096];
        private readonly Security _serverSecurity = new(maxMassiveFragments: 4096, maxMassiveBytes: 16 * 1024 * 1024);
        private TcpClient _clientTcpClient;
        private int _stopped;
        private int _clientDetached;
        private TcpClient? _serverTcpClient;
        private static readonly bool IsTryCatchDebug = false;
        private readonly CancellationTokenSource _shutdown = new();
        private readonly SemaphoreSlim _clientSendLock = new(1, 1);
        private readonly SemaphoreSlim _serverSendLock = new(1, 1);
        private readonly ConcurrentQueue<Packet> _serverPacketFollowUps = new();
        private Task? _clientReceiveLoop;
        private Task? _serverReceiveLoop;
        private Task? _lifecycleWatchdog;
        private Task _shutdownCompletion = Task.CompletedTask;
        private int _clientHandshakeAccepted;
        private readonly ClientHandshakePacketQueue _clientHandshakePackets = new();
        private int _tracePacketCount;
        private const int ClientPacketGuardWindowMs = 3_000;
        private const int MaxClientPacketsPerWindow = 900;
        private const int MaxClientCustomPacketsPerWindow = 180;
        private const int MaxClientBytesPerWindow = 512 * 1024;
        private const int MaxClientPacketPayloadBytes = 32 * 1024;
        private int _clientGuardPacketCount;
        private int _clientGuardCustomPacketCount;
        private int _clientGuardBytes;
        private long _clientGuardWindowStarted = Environment.TickCount64;
        private long _lastClientActivity = Environment.TickCount64;
        private long _serverEgressBytes;
        private const int ServerPacketGuardWindowMs = 3_000;
        private int _serverGuardPacketCount;
        private int _serverGuardCustomPacketCount;
        private long _serverGuardBytes;
        private long _serverGuardWindowStarted = Environment.TickCount64;
        private int _serverGuardViolations;
        private static int _floodCleanupCounter;
        private int _malformedPacketCount;
        private long _malformedWindowStarted = Environment.TickCount64;

        public bool IsStopped => Volatile.Read(ref _stopped) != 0;
        public bool ClientDetached => Volatile.Read(ref _clientDetached) != 0;
        public Task ShutdownCompletion => Volatile.Read(ref _shutdownCompletion);
        public Session(TcpClient clientTcpClient, IAsyncServer asyncServer)
        {
            AsyncServer = asyncServer;
            SessionData = new SessionData();
            _clientTcpClient = clientTcpClient;
            _clientTcpClient.NoDelay = true;

            // generates a "unique" id from the address and port hashcode and safes ip
            if (!(_clientTcpClient.Client.RemoteEndPoint is IPEndPoint ep)) return;
            ClientGuid = Guid.NewGuid();
            ClientIp = ep.Address.ToString();
            //Log.Warning($"{ClientIp}");

        }

        public IAsyncServer AsyncServer { get; init; }
        public ISessionData SessionData { get; init; }

        public Guid ClientGuid { get; set; }
        public string ClientIp { get; set; } = string.Empty;
        public int ConnectedServerID { get; set; }
        public bool SecondaryCodeEntered { get; set; } = false;
        public Packet? TempPacket { get; set; }
        public string HwidChallenge { get; set; } = string.Empty;
        public string HwidChallengeRole { get; set; } = string.Empty;
        public long HwidChallengeIssuedAtUnix { get; set; }
        public DateTime HwidChallengeExpiresAt { get; set; }
        public string DeviceKeyThumbprint { get; set; } = string.Empty;
        public string DevicePublicKey { get; set; } = string.Empty;
        public string VerifiedHwidNonce { get; set; } = string.Empty;
        public bool QuickLoginNonceRefreshPending { get; set; }
        public bool PendingQuickLogin { get; set; }
        public bool PendingPrimaryLogin { get; set; }
        public long PendingPrimaryLoginStartedAt { get; set; }
        public GatewayAuthenticationState GatewayAuthenticationState { get; set; } =
            GatewayAuthenticationState.AwaitingHwid;
        public int SecondaryPasswordFailures { get; set; }
        public DateTime SecondaryPasswordBlockedUntil { get; set; }
        public string GameServerPacketKey { get; } =
            Convert.ToHexString(RandomNumberGenerator.GetBytes(32));
   
        public void Stop()
        {
            Stop("Unknown reason");
        }

        public void Stop(string reason)
        {
   
            // double socket close prevention
            if (Interlocked.Exchange(ref _stopped, 1) != 0)
                return;

            Log.Verbose("{Service} session {ClientIp} stopped: {Reason}",
                AsyncServer.Service.Name, ClientIp, reason);

            SessionData.ClearFellowRuntimeState();
            SessionData.GatewayCredential.Clear();
            PvpChallengeService.HandleSessionStopping(this, reason);
            OfflineStallService.OnSessionStopping(this, reason);
            _shutdown.Cancel();

            if (SessionData.Charid > 0)
                Volatile.Write(ref _shutdownCompletion, CleanupDatabaseStateAsync());

            AsyncServer.RemoveSession(this);

            SilkStallService.QueueCancellation(this, reason);

            try { _clientTcpClient?.Close(); } catch { }
            try { _serverTcpClient?.Close(); } catch { }
        }

        public void ReportMalformedPacket(ushort opcode, string direction, string reason)
        {
            long now = Environment.TickCount64;
            if (now - _malformedWindowStarted >= 60_000)
            {
                _malformedWindowStarted = now;
                Interlocked.Exchange(ref _malformedPacketCount, 0);
            }

            int count = Interlocked.Increment(ref _malformedPacketCount);
            if (count == 1 || count == 5 || count % 20 == 0)
            {
                Log.Warning(
                    "Malformed packet session={Session} ip={ClientIp} direction={Direction} opcode=0x{Opcode:X4} count={Count}/60s reason={Reason}",
                    ClientGuid,
                    ClientIp,
                    direction,
                    opcode,
                    count,
                    reason);
            }
        }

        public bool TryDetachClientTransport(string reason)
        {
            if (IsStopped || !SessionData.OfflineStall)
                return false;
            if (Interlocked.Exchange(ref _clientDetached, 1) != 0)
                return true;

            Log.Information("{Service} downstream client detached for Offline Stall {CharName}: {Reason}",
                AsyncServer.Service.Name, SessionData.Charname, reason);
            PvpChallengeService.HandleSessionStopping(this, reason);
            try { _clientTcpClient.Close(); } catch { }
            OfflineStallService.NotifyClientDetached(this);
            return true;
        }

        private async Task CleanupDatabaseStateAsync()
        {
            try
            {
                int characterId = SessionData.Charid;
                if (SessionData.IsInParty && characterId > 0)
                {
                    var partyCleanupCompleted = new TaskCompletionSource<bool>(
                        TaskCreationOptions.RunContinuationsAsynchronously);
                    bool queued = KeyedDatabaseJobQueue.TryQueueBackground(characterId, async cancellationToken =>
                    {
                        try
                        {
                            await using var partyConnection = new SqlConnection(Program.Connectionstring);
                            await partyConnection.OpenAsync(cancellationToken);
                            await partyConnection.ExecuteAsync(new CommandDefinition(
                                "DELETE FROM [dbo].[Party_Members] WHERE CharID = @CharID",
                                new { CharID = characterId },
                                commandTimeout: 30,
                                cancellationToken: cancellationToken));
                        }
                        finally
                        {
                            partyCleanupCompleted.TrySetResult(true);
                        }
                    }, operation: "ordered party disconnect cleanup");

                    if (queued)
                        await partyCleanupCompleted.Task.ConfigureAwait(false);
                }

                await DatabaseJobQueue.RunIdempotentAsync(async cancellationToken =>
                {
                    await using var connection = new SqlConnection(Program.Connectionstring);
                    await connection.OpenAsync(cancellationToken);

                    if (characterId > 0)
                    {
                        await connection.ExecuteAsync(
                            new CommandDefinition(
                                "UPDATE [dbo].[Auth_HWIDs] SET Active = 0 WHERE CharID = @CharID",
                                new { CharID = characterId },
                                commandTimeout: 10,
                                cancellationToken: cancellationToken));
                    }
                }, operation: "session disconnect cleanup");
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Session cleanup failed for CharID {CharID}", SessionData.Charid);
            }
        }

        public async Task SendToClient(Packet packet)
        {
            if (_clientTcpClient == null || IsStopped || ClientDetached)
                return;

            try
            {
                await SendOrQueueClientPacketAsync(packet);
            }
            catch
            {
                if (!TryDetachClientTransport("send to client failed"))
                    Stop("send to client");
            }
        }

        public async Task SendToServer(Packet packet)
        {
            if (_serverTcpClient == null || IsStopped)
                return;

            try
            {
                await SendServerPacketInternal(packet);
            }
            catch
            {
                Stop("send to server");
            }
        }

        public async Task SendNotice(string message)
        {
            var notice = new Packet(0x3026, false, false);
            notice.WriteByte(7);
            notice.WriteAscii(message);
            await SendToClient(notice);
        }

        public async Task Start()
        {

            _clientSecurity.GenerateSecurity(true, true, true);
            await FlushClientAsync();
            // creates a new server socket and connects it according to the remote addr. and port 
            //_serverTcpClient = new TcpClient(new IPEndPoint(IPAddress.Parse("78.135.85.104"), 0));
            _serverTcpClient = new TcpClient();
            try
            {

                using var connectTimeout = CancellationTokenSource.CreateLinkedTokenSource(_shutdown.Token);
                connectTimeout.CancelAfter(TimeSpan.FromSeconds(
                    Program.RuntimeSettings.UpstreamConnectTimeoutSeconds));
                await _serverTcpClient.ConnectAsync(
                    AsyncServer.RemoteEndPoint.Address,
                    AsyncServer.RemoteEndPoint.Port,
                    connectTimeout.Token);
                Log.Verbose("{Service} connected remote for {ClientIp}: {RemoteEndPoint}",
                    AsyncServer.Service.Name, ClientIp, AsyncServer.RemoteEndPoint);

            }
            catch
            {
                Stop("remote server connection failed");
                return;
            }

            _serverTcpClient.NoDelay = true;
            ConfigureKeepAlive(_clientTcpClient);
            ConfigureKeepAlive(_serverTcpClient);
            _clientReceiveLoop = RunClientReceiveLoopAsync(_shutdown.Token);
            _serverReceiveLoop = RunServerReceiveLoopAsync(_shutdown.Token);
            _lifecycleWatchdog = RunLifecycleWatchdogAsync(_shutdown.Token);
            await Task.CompletedTask;
        }

        private static readonly ConcurrentDictionary<string, FloodInfo> _ipFloodMap = new();

        private class FloodInfo
        {
            public int PacketCount;
            public long WindowStarted = Environment.TickCount64;
            public long LastSeen = Environment.TickCount64;
        }

        private string FloodKey => $"{AsyncServer.Service.ServerType}:{ClientIp}";

        private static void ConfigureKeepAlive(TcpClient client)
        {
            try
            {
                client.Client.SetSocketOption(
                    SocketOptionLevel.Socket, SocketOptionName.KeepAlive, true);
                client.Client.SetSocketOption(
                    SocketOptionLevel.Tcp, SocketOptionName.TcpKeepAliveTime, 60);
                client.Client.SetSocketOption(
                    SocketOptionLevel.Tcp, SocketOptionName.TcpKeepAliveInterval, 15);
                client.Client.SetSocketOption(
                    SocketOptionLevel.Tcp, SocketOptionName.TcpKeepAliveRetryCount, 4);
            }
            catch (SocketException ex) when (ex.SocketErrorCode == SocketError.ProtocolOption)
            {
                // SO_KEEPALIVE was enabled before the optional per-socket tuning.
                // Some supported Windows versions do not expose those tuning options.
            }
            catch (SocketException ex)
            {
                Log.Debug(ex, "TCP keepalive tuning is unavailable for {Service}",
                    client.Client.RemoteEndPoint);
            }
        }

        private static void CleanupExpiredFloodEntries()
        {
            var now = Environment.TickCount64;
            foreach (var entry in _ipFloodMap)
            {
                if (now - Volatile.Read(ref entry.Value.LastSeen) >= 10 * 60 * 1000L)
                    _ipFloodMap.TryRemove(entry.Key, out _);
            }
        }

        internal static bool IsClientActivityDeadlineExceeded(
            long now,
            long lastClientActivity,
            int timeoutSeconds)
        {
            return now - lastClientActivity >= timeoutSeconds * 1000L;
        }

        private async Task RunLifecycleWatchdogAsync(CancellationToken cancellationToken)
        {
            var connectedAt = Environment.TickCount64;
            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken);
                    if (SessionData.OfflineStall)
                        continue;

                    var now = Environment.TickCount64;
                    if (Volatile.Read(ref _clientHandshakeAccepted) == 0 &&
                        now - connectedAt >= Program.RuntimeSettings.HandshakeTimeoutSeconds * 1000L)
                    {
                        Stop("client handshake deadline exceeded");
                        return;
                    }

                    var authenticated = AsyncServer.Service.ServerType switch
                    {
                        ServerType.GatewayServer =>
                            GatewayAuthenticationState == GatewayAuthenticationState.Released,
                        ServerType.AgentServer => UserLoggedIn,
                        ServerType.DownloadServer => Volatile.Read(ref _clientHandshakeAccepted) != 0,
                        _ => false
                    };

                    if (!authenticated &&
                        now - connectedAt >= Program.RuntimeSettings.LoginTimeoutSeconds * 1000L)
                    {
                        Stop("authentication deadline exceeded");
                        return;
                    }

                    var lastClientActivity = Volatile.Read(ref _lastClientActivity);
                    if (!authenticated && IsClientActivityDeadlineExceeded(
                            now,
                            lastClientActivity,
                            Program.RuntimeSettings.UnauthenticatedIdleTimeoutSeconds))
                    {
                        Stop("unauthenticated idle deadline exceeded");
                        return;
                    }

                    // 0x2002 is an idle heartbeat, not the only proof that an authenticated
                    // client is alive. Active clients can legitimately suppress that heartbeat
                    // while sending movement, combat, or other traffic.
                    if (authenticated && IsClientActivityDeadlineExceeded(
                            now,
                            lastClientActivity,
                            Program.RuntimeSettings.AuthenticatedHeartbeatTimeoutSeconds))
                    {
                        Stop("authenticated client inactivity deadline exceeded");
                        return;
                    }
                }
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
            }
        }

        private async Task RunClientReceiveLoopAsync(CancellationToken cancellationToken)
        {
            try
            {
                while (!cancellationToken.IsCancellationRequested && !ClientDetached)
                    await DoReceiveFromClient(cancellationToken);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
            finally
            {
                if (!ClientDetached)
                    Stop("client receive loop ended");
            }
        }

        public void QueueClientPacketAfterCurrentServerPacket(Packet packet)
        {
            ArgumentNullException.ThrowIfNull(packet);
            _serverPacketFollowUps.Enqueue(packet);
        }

        private async Task RunServerReceiveLoopAsync(CancellationToken cancellationToken)
        {
            try
            {
                while (!cancellationToken.IsCancellationRequested)
                    await DoReceiveFromServer(cancellationToken);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
            finally
            {
                Stop("server receive loop ended");
            }
        }

        private async Task DoReceiveFromClient(CancellationToken cancellationToken)
        {
            try
            {
                var clientBufferMemory = new Memory<byte>(_clientBuffer);
                var recvCount = await _clientTcpClient.GetStream().ReadAsync(clientBufferMemory, cancellationToken);

                if (recvCount == 0)
                {
                    if (!TryDetachClientTransport("client disconnected"))
                        Stop("receive count 0");
                    return;
                }

                Volatile.Write(ref _lastClientActivity, Environment.TickCount64);

                _clientSecurity.Recv(_clientBuffer, 0, recvCount);
                var receivedPackets = _clientSecurity.TransferIncoming();
                await FlushClientAsync();

                if (receivedPackets == null)
                {
                    return;
                }

                foreach (var packet in receivedPackets)
                {
                    TraceEarlyPacket("C->S", packet);

                    if (packet.Opcode == 0x9000)
                        await AcceptClientHandshakeAndFlushAsync();

                    if (packet.Opcode == 0x9000 || packet.Opcode == 0x5000)
                        continue;

                    // Once Offline Stall is armed the owner is no longer an interactive
                    // player. Only the keep-alive may pass while the short ACK delay runs.
                    if (SessionData.OfflineStall && packet.Opcode != 0x2002)
                        continue;

                    if (!TryPassClientPacketGuards(packet, out var guardReason))
                    {
                        Log.Warning("{Service} emergency packet guard blocked {ClientIp}: {Reason} opcode=0x{Opcode:X4} len={Length}",
                            AsyncServer.Service.Name,
                            ClientIp,
                            guardReason,
                            packet.Opcode,
                            packet.GetBytes().Length);

                        Stop("emergency packet guard: " + guardReason);
                        return;
                    }

                    //_packetCount++;

                    //var currentTime = (DateTime.Now - _lastPacketReset).TotalSeconds;
                    //if (currentTime >= 1)
                    //{
                    //    if (_packetCount > 10)
                    //    {
                    //        Stop("byte limit");
                    //        return;
                    //    }

                    //    _packetCount = 0;
                    //    _lastPacketReset = DateTime.Now;
                    //}

                    #region Flood Protection
                    if (CharScreen)
                    {
                        var ip = ClientIp;
                        var floodKey = FloodKey;
                        var floodInfo = _ipFloodMap.GetOrAdd(floodKey, _ => new FloodInfo());
                        Volatile.Write(ref floodInfo.LastSeen, Environment.TickCount64);

                        var packetCount = Interlocked.Increment(ref floodInfo.PacketCount);
                        var elapsedMs = Environment.TickCount64 - Volatile.Read(ref floodInfo.WindowStarted);

                        if (packetCount > 120 && elapsedMs < 3000)
                        {
                            Log.Information($"Kicked for byte limit {this.ClientIp}");
                            Stop($"Flood detected from {ip}");


                            return;
                        }

                        if (elapsedMs >= 3000)
                        {
                            Interlocked.Exchange(ref floodInfo.PacketCount, 1);
                            Volatile.Write(ref floodInfo.WindowStarted, Environment.TickCount64);
                        }

                        if ((Interlocked.Increment(ref _floodCleanupCounter) & 0xFF) == 0)
                            CleanupExpiredFloodEntries();
                    }
                    #endregion

                    PacketLength = packet.GetBytes().Length;

                    PacketResult packetResult;
                    if (IsTryCatchDebug)
                    {
                        try
                        {
                            packetResult = await AsyncServer.PacketHandler.HandleClient(packet, this);
                        }
                        catch (Exception e)
                        {
                            Log.Warning(e.Message);
                            packetResult = new PacketResult(PacketResultType.Block);
                        }
                    }
                    else
                    {
                        packetResult = await AsyncServer.PacketHandler.HandleClient(packet, this);
                    }

                    switch (packetResult.PacketResultType)
                    {
                        case PacketResultType.Override:
                            if (packetResult.OverridePacket != null)
                                await SendServerPacketInternal(packetResult.OverridePacket);
                            break;
                        case PacketResultType.Block:
                            break;
                        case PacketResultType.Disconnect:
                            Stop("receive from client disconnect");
                            break;
                        case PacketResultType.Nothing:
                            await SendServerPacketInternal(packet);
                            break;
                        default:
                            throw new ArgumentOutOfRangeException();
                    }

                    await FlushServerPacketFollowUpsAsync();
                }

            }
            catch (Exception e)
            {
                if (!TryDetachClientTransport("client receive failed"))
                    Stop("receive from client catch " + e?.Message + "\n" + e?.InnerException?.Message);
            }
        }

        private async Task DoReceiveFromServer(CancellationToken cancellationToken)
        {
            try
            {
                var serverTcpClient = _serverTcpClient;
                if (serverTcpClient == null)
                {
                    Stop("server connection is not available");
                    return;
                }

                // receive stuff
                var serverBufferMemory = new Memory<byte>(_serverBuffer);
                var recvCount = await serverTcpClient.GetStream().ReadAsync(serverBufferMemory, cancellationToken);
                if (recvCount == 0)
                {
                    Stop("receive from server disconnect");
                    return;
                }

                // starts receiving again
                _serverSecurity.Recv(_serverBuffer, 0, recvCount);

                // transfers the incoming packets to a list
                var receivedPackets = _serverSecurity.TransferIncoming();
                await FlushServerAsync();

                // if there are no packets start receiving again
                if (receivedPackets == null)
                {
                    return;
                }

                // loop through all received packets
                foreach (var packet in receivedPackets)
                {
                    TraceEarlyPacket("S->C", packet);

                    if (!TryPassServerPacketGuards(packet, out var guardReason))
                    {
                        int violations = Interlocked.Increment(ref _serverGuardViolations);
                        Log.Warning(
                            "Blocked server packet opcode=0x{Opcode:X4} size={Size} session={Session} service={Service} reason={Reason} violations={Violations}",
                            packet.Opcode,
                            packet.GetBytes().Length,
                            ClientGuid,
                            AsyncServer.Service.Name,
                            guardReason,
                            violations);
                        if (violations >= 5)
                            Stop("repeated unsafe server packets");
                        continue;
                    }

                    //Log.Warning(2, "Opcode:{Iwa0x" + packet.Opcode.ToString("X") + "}");
                    // ignore handshake
                    if (packet.Opcode == 0x5000 || packet.Opcode == 0x9000 ||
                        (packet.Opcode == 0x2001 && Volatile.Read(ref _clientHandshakeAccepted) == 0))
                    {
                        continue;
                    }

                    // debug
                    //WriteLine(1, $"Server Packet {packet.Opcode.ToString("X")}");
                    PacketResult packetResult;
                    if (IsTryCatchDebug)
                    {
                        try
                        {
                            packetResult = await AsyncServer.PacketHandler.HandleServer(packet, this);
                        }
                        catch (Exception e)
                        {
                            Log.Warning(e.Message.ToString());
                            packetResult = new PacketResult(PacketResultType.Block);
                        }
                    }
                    else
                    {
                        packetResult = await AsyncServer.PacketHandler.HandleServer(packet, this);
                    }

                    switch (packetResult.PacketResultType)
                    {
                        case PacketResultType.Override:
                            if (packetResult.OverridePacket != null)
                                await SendOrQueueClientPacketAsync(packetResult.OverridePacket);
                            break;
                        case PacketResultType.Block:
                            break;
                        case PacketResultType.Disconnect:
                            Stop("receive from server disconnect");
                            break;
                        case PacketResultType.Nothing:
                            await SendOrQueueClientPacketAsync(packet);
                            break;
                        default:
                            throw new ArgumentOutOfRangeException();
                    }

                    await FlushServerPacketFollowUpsAsync();
                }

                // transfers packets to the client and starts receiving from server again
            }
            catch (Exception e)
            {
                Stop("receive from server catch " + e.Message + "\n" + e.InnerException?.Message);
            }
        }

        private async Task SendClientPacketInternal(Packet packet)
        {
            if (ClientDetached)
                return;
            await FlushClientAsync(packet);
        }

        private async Task SendOrQueueClientPacketAsync(Packet packet)
        {
            if (!ExternalBotPacketCompatibility.TryAdaptServerPacket(
                    IsExternalBot,
                    packet,
                    out var compatiblePacket))
            {
                Log.Warning(
                    "Suppressed malformed custom notice for external bot session. Account={AccountName} IP={ClientIp}",
                    PlayerUserID,
                    ClientIp);
                return;
            }

            packet = compatiblePacket!;

            switch (_clientHandshakePackets.QueueOrReady(packet))
            {
                case ClientPacketQueueResult.Ready:
                    await SendClientPacketInternal(packet);
                    return;
                case ClientPacketQueueResult.Queued:
                    return;
                case ClientPacketQueueResult.Overflow:
                    Stop("client handshake packet queue overflow");
                    return;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private async Task FlushServerPacketFollowUpsAsync()
        {
            while (_serverPacketFollowUps.TryDequeue(out var packet))
                await SendOrQueueClientPacketAsync(packet);
        }

        private async Task AcceptClientHandshakeAndFlushAsync()
        {
            Interlocked.Exchange(ref _clientHandshakeAccepted, 1);
            if (!_clientHandshakePackets.BeginFlush())
                return;

            while (!IsStopped && !ClientDetached)
            {
                var batch = _clientHandshakePackets.TakePendingBatchOrComplete();
                if (batch.Length == 0)
                    return;

                foreach (var packet in batch)
                    await SendClientPacketInternal(packet);
            }
        }

        private void TraceEarlyPacket(string direction, Packet packet)
        {
            if (AsyncServer.Service.ServerType != ServerType.AgentServer)
                return;

            if (Interlocked.Increment(ref _tracePacketCount) > 30)
                return;

            Log.Verbose("{Service} packet {Direction} {ClientIp}: 0x{Opcode:X4} len={Length}",
                AsyncServer.Service.Name, direction, ClientIp, packet.Opcode, packet.GetBytes().Length);
        }

        private bool TryPassClientPacketGuards(Packet packet, out string reason)
        {
            reason = string.Empty;
            var length = packet.GetBytes().Length;

            if (length > MaxClientPacketPayloadBytes)
            {
                reason = $"oversized client packet {length} bytes";
                return false;
            }

            var now = Environment.TickCount64;
            if (now - _clientGuardWindowStarted >= ClientPacketGuardWindowMs)
            {
                _clientGuardWindowStarted = now;
                _clientGuardPacketCount = 0;
                _clientGuardCustomPacketCount = 0;
                _clientGuardBytes = 0;
            }

            _clientGuardPacketCount++;
            _clientGuardBytes += length;

            if (_clientGuardPacketCount > MaxClientPacketsPerWindow)
            {
                reason = $"packet flood {_clientGuardPacketCount}/{ClientPacketGuardWindowMs}ms";
                return false;
            }

            if (_clientGuardBytes > MaxClientBytesPerWindow)
            {
                reason = $"packet byte flood {_clientGuardBytes}/{ClientPacketGuardWindowMs}ms";
                return false;
            }

            if (IsCustomClientOpcode(packet.Opcode))
            {
                if (RequiresReadyCharacterForCustomOpcode(packet.Opcode) && !IsReadyForInGameCustomPacket())
                {
                    reason = $"custom opcode before ready character opcode=0x{packet.Opcode:X4}";
                    return false;
                }

                _clientGuardCustomPacketCount++;
                if (_clientGuardCustomPacketCount > MaxClientCustomPacketsPerWindow)
                {
                    reason = $"custom opcode flood {_clientGuardCustomPacketCount}/{ClientPacketGuardWindowMs}ms";
                    return false;
                }
            }

            return true;
        }

        private bool TryPassServerPacketGuards(Packet packet, out string reason)
        {
            reason = string.Empty;
            int length = packet.GetBytes().Length;
            ServerType serviceType = AsyncServer.Service.ServerType;

            // Massive character/inventory/guild/party/stall snapshots are a
            // normal part of vSRO loading.  The Security reassembler already
            // enforces the 16 MiB hard ceiling; ordinary packets remain much
            // smaller and receive a tighter independent limit.
            int maxPacketBytes = packet.Massive ? 16 * 1024 * 1024 : 256 * 1024;
            if (serviceType == ServerType.DownloadServer)
                maxPacketBytes = 32 * 1024 * 1024;
            if (length > maxPacketBytes)
            {
                reason = $"packet size {length} exceeds {maxPacketBytes}";
                return false;
            }

            long now = Environment.TickCount64;
            if (now - _serverGuardWindowStarted >= ServerPacketGuardWindowMs)
            {
                _serverGuardWindowStarted = now;
                _serverGuardPacketCount = 0;
                _serverGuardCustomPacketCount = 0;
                _serverGuardBytes = 0;
            }

            _serverGuardPacketCount++;
            _serverGuardBytes += length;
            if (IsCustomServerOpcode(packet.Opcode))
                _serverGuardCustomPacketCount++;

            bool loadingBurst = !CharacterGameReady || packet.Massive;
            int maxPackets = serviceType == ServerType.DownloadServer ? 30_000 : loadingBurst ? 18_000 : 9_000;
            long maxBytes = serviceType == ServerType.DownloadServer
                ? 256L * 1024 * 1024
                : loadingBurst ? 128L * 1024 * 1024 : 64L * 1024 * 1024;
            int maxCustomPackets = loadingBurst ? 3_000 : 1_500;

            if (_serverGuardPacketCount > maxPackets)
                reason = $"packet rate {_serverGuardPacketCount}/{ServerPacketGuardWindowMs}ms";
            else if (_serverGuardBytes > maxBytes)
                reason = $"byte rate {_serverGuardBytes}/{ServerPacketGuardWindowMs}ms";
            else if (_serverGuardCustomPacketCount > maxCustomPackets)
                reason = $"custom opcode rate {_serverGuardCustomPacketCount}/{ServerPacketGuardWindowMs}ms";

            return reason.Length == 0;
        }

        private static bool IsCustomServerOpcode(ushort opcode) =>
            opcode is >= 0x1200 and <= 0x22FF or >= 0x5000 and <= 0x50FF;

        private static bool IsCustomClientOpcode(ushort opcode)
        {
            return opcode is
                0x1211 or
                0x165B or
                0x166A or
                0x169A or
                0x169B or
                0x169C or
                0x180A or
                0x181C or
                0x185A or
                0x185C or
                0x186D or
                0x187E or
                0x18D0 or
                0x189A or
                0x189B or
                0x200C or
                0x200D or
                0x201F or
                0x207B or
                0x210A or
                0x400B or
                0xA150 or
                0xA400 or
                0xB296 or
                0xB297 or
                0xB298 or
                0xB299 or
                0xB300 or
                0xB301 or
                0xCC1E;
        }

        private bool IsReadyForInGameCustomPacket()
        {
            return AsyncServer.Service.ServerType != ServerType.AgentServer ||
                   (CharacterGameReady && SessionData.Charid > 0);
        }

        private static bool RequiresReadyCharacterForCustomOpcode(ushort opcode)
        {
            return opcode is not (
                0x1211 or // gateway secondary password
                0x165B or // client HWID/DLL handshake
                0x166A or // gateway account register
                0xA150 or // DLL settings request
                0xA400);  // in-game interface bootstrap can arrive before 0x3012
        }

        private async Task FlushClientAsync(Packet? packet = null)
        {
            if (IsStopped || ClientDetached) return;
            var lockTaken = false;
            try
            {
                await _clientSendLock.WaitAsync(_shutdown.Token);
                lockTaken = true;

                if (packet != null)
                {
                    _clientSecurity.Send(packet);
                }
                var kvp = _clientSecurity.TransferOutgoing();

                if (kvp == null) return;

                foreach (var t in kvp)
                    //transfers the client packets to the client
                    await _clientTcpClient.GetStream().WriteAsync(t.Key.Buffer, _shutdown.Token);
            }
            catch
            {
                if (!IsStopped && !TryDetachClientTransport("transfer to client failed"))
                    Stop("transfer to client catch");
            }
            finally
            {
                if (lockTaken)
                    _clientSendLock.Release();
            }
        }

        private async Task SendServerPacketInternal(Packet packet)
        {
            await FlushServerAsync(packet);
        }

        private async Task FlushServerAsync(Packet? packet = null)
        {
            if (IsStopped) return;
            var lockTaken = false;
            try
            {
                await _serverSendLock.WaitAsync(_shutdown.Token);
                lockTaken = true;

                if (packet != null)
                {
                    _serverSecurity.Send(packet);
                }
                var kvp = _serverSecurity.TransferOutgoing();
                if (kvp == null) return;
                foreach (var t in kvp)
                {
                    if (t.Key == null || t.Key.Buffer == null)
                    {
                        Log.Warning("Key or Buffer is null");
                        continue;
                    }
                    Interlocked.Add(ref _serverEgressBytes, t.Key.Buffer.Length);
                    //transfers the server packets to the server
                    if (_serverTcpClient?.GetStream() != null)
                    {
                        await _serverTcpClient.GetStream().WriteAsync(t.Key.Buffer, _shutdown.Token);
                    }
                    else
                    {
                        Log.Warning("_serverTcpClient or its stream is null");
                    }
                }
            }
            catch (Exception ex)
            {
                if (!IsStopped)
                {
                    Stop("transfer to server catch");
                    Log.Warning(ex.Message.ToString());
                }
            }
            finally
            {
                if (lockTaken)
                    _serverSendLock.Release();
            }
        }



        #region Features

        //public ITimerManager TimerManager { get; set; }
        //public ICountdownManager CountdownManager { get; set; }
        //public ICharInfo CharInfo { get; init; }
        public string PlayerUserID { get; set; } = string.Empty;
        public bool CharacterGameReady { get; set; } = false;
        public bool IsManagedClientless { get; set; }
        public bool IsSystemClientless { get; set; }
        public bool IsExternalBot { get; set; }
        //public ISessionData SessionData { get; init; }

        #endregion

        #region Protection

        // Packet Modification
        public int PacketLength { get; set; }

        // timer
        public DateTime LastPing { get; set; } = DateTime.Now;

        // False Packets
        public bool CharnameSent { get; set; } = false;
        public bool CharScreen { get; set; } = false;
        public bool UserLoggedIn { get; set; } = false;

        #endregion

    }
}
