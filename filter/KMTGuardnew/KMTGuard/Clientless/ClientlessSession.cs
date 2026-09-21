using System.Net.Sockets;
using KMTGuard.Database.Models;
using KMTGuard.ServerManagers;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Clientless;

public sealed class ClientlessSession : IDisposable
{
    private static readonly TimeSpan InitialWorldSynchronizationTimeout = TimeSpan.FromSeconds(10);
    private static readonly TimeSpan InitialWorldQuietPeriod = TimeSpan.FromMilliseconds(750);

    private readonly ClientlessAccount _account;
    private readonly __ProxyServices _gatewayService;
    private readonly byte[] _buffer = new byte[8192];
    private readonly Queue<Packet> _pendingPackets = new();
    private readonly CancellationTokenSource _sessionShutdown = new();
    private readonly object _securitySync = new();
    private readonly SemaphoreSlim _sendLock = new(1, 1);
    private readonly ClientlessTeleportProtocol _teleportProtocol = new();
    private readonly MemoryStream _characterDataBytes = new();
    private readonly ClientlessHuntEngine _huntEngine;
    private TcpClient? _tcpClient;
    private Security? _security;
    private string _phase = "Created";
    private ushort _lastReceivedOpcode;
    private ushort _lastSentOpcode;
    private volatile bool _partyFormRequested;
    private volatile bool _partyFormDesired;
    private volatile bool _partyFormDeleteRequested;
    private volatile bool _isOnline;
    private ClientlessAccount? _pendingHuntingConfiguration;
    private uint _partyMatchingId;
    private uint _managedPartyId;
    private int _partyFormAttemptCount;
    private uint _expectedManagedPartyLeaderUniqueId;
    private volatile int _managedPartyMemberCount;
    private ClientlessManager.PartyFormPolicy? _partyFormPolicy;

    private enum AgentAuthMode
    {
        Full,
        EmptyCredentials,
        TokenOnly,
        TokenUserLocale
    }

    public ClientlessSession(ClientlessAccount account, __ProxyServices gatewayService)
    {
        _account = account;
        _gatewayService = gatewayService;
        _huntEngine = new ClientlessHuntEngine(account, SendAsync);
    }

    public int AccountId => _account.ID;
    public string AccountName => _account.AccountName;
    public string CharacterName => _account.CharacterName;
    public string City => _account.City;
    public string SystemRole => _account.SystemRole;
    internal int? HuntAreaId => _account.HuntAreaID;
    internal uint SelfUniqueId => _huntEngine.SelfUniqueId;
    internal bool IsReadyForManagedParty => _isOnline && _huntEngine.SelfUniqueId != 0;
    internal bool HasManagedPartyForm => _partyMatchingId != 0 || _partyFormRequested;
    internal bool HasManagedPartyMembership => _managedPartyId != 0 || _managedPartyMemberCount > 0;
    internal int ManagedPartyMemberCount => _managedPartyMemberCount;
    internal uint ManagedPartyId => _managedPartyId;

    public bool Matches(ClientlessAccount account)
    {
        return _account.ID == account.ID &&
               string.Equals(_account.AccountName, account.AccountName, StringComparison.OrdinalIgnoreCase) &&
               string.Equals(_account.AccountPassword, account.AccountPassword, StringComparison.Ordinal) &&
               string.Equals(_account.CharacterName, account.CharacterName, StringComparison.OrdinalIgnoreCase) &&
               string.Equals(_account.City, account.City, StringComparison.OrdinalIgnoreCase) &&
               _account.Locale == account.Locale &&
               _account.ShardID == account.ShardID &&
               string.Equals(_account.AgentAuthMode, account.AgentAuthMode, StringComparison.OrdinalIgnoreCase) &&
               _account.HuntEnabled == account.HuntEnabled &&
               _account.HuntAreaID == account.HuntAreaID &&
               _account.HuntRegionID == account.HuntRegionID &&
               Math.Abs(_account.HuntX - account.HuntX) < 0.1f &&
               Math.Abs(_account.HuntY - account.HuntY) < 0.1f &&
               Math.Abs(_account.HuntZ - account.HuntZ) < 0.1f &&
               Math.Abs(_account.HuntRadius - account.HuntRadius) < 0.1f;
    }

    public void RefreshHuntingConfiguration(ClientlessAccount account)
    {
        if (_account.ID != account.ID)
            throw new InvalidOperationException("Cannot apply hunting settings from a different Clientless account.");

        // Dashboard refreshes arrive on an Admin/HTTP thread while all packet and
        // hunting state is owned by KeepOnlineAsync. Queue the immutable database
        // snapshot and apply it on that single owner loop to avoid modifying the
        // engine's dictionaries while a group spawn or target tick is in progress.
        Interlocked.Exchange(ref _pendingHuntingConfiguration, account);
    }

    public async Task RunAsync(CancellationToken cancellationToken)
    {
        using var linkedShutdown =
            CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, _sessionShutdown.Token);
        var runToken = linkedShutdown.Token;
        var policyBlockedStatusWritten = false;
        var consecutiveFailures = 0;
        while (!runToken.IsCancellationRequested)
        {
            var botConfig = await BotProtectionService.GetConfigAsync();
            if (!botConfig.AllowBotLogin && string.IsNullOrWhiteSpace(_account.SystemRole))
            {
                if (!policyBlockedStatusWritten)
                {
                    await ClientlessManager.UpdateStatusAsync(
                        _account.ID,
                        "PolicyBlocked",
                        "AllowBotLogin is disabled");
                    policyBlockedStatusWritten = true;
                }
                await Task.Delay(TimeSpan.FromSeconds(5), runToken);
                continue;
            }

            policyBlockedStatusWritten = false;

            try
            {
                await LoginOnceAsync(runToken);
                consecutiveFailures = 0;
            }
            catch (OperationCanceledException) when (runToken.IsCancellationRequested)
            {
                break;
            }
            catch (Exception ex)
            {
                consecutiveFailures++;
                Log.Warning("Clientless character {Character} disconnected. Phase={Phase}, LastSent=0x{LastSent:X4}, LastReceived=0x{LastReceived:X4}, Reason={Reason}",
                    _account.CharacterName,
                    _phase,
                    _lastSentOpcode,
                    _lastReceivedOpcode,
                    ex.Message);
                await ClientlessManager.UpdateStatusAsync(_account.ID, "Disconnected", ex.Message);
            }
            finally
            {
                CloseSocket();
            }

            var baseDelay = Math.Clamp(_account.ReconnectDelaySeconds, 5, 600);
            var multiplier = 1 << Math.Min(consecutiveFailures, 4);
            var delay = Math.Clamp(baseDelay * multiplier, 5, 600);
            delay = Math.Clamp(delay + Random.Shared.Next(0, Math.Max(2, baseDelay / 3)), 5, 600);
            await Task.Delay(TimeSpan.FromSeconds(delay), runToken);
        }
    }

    private async Task LoginOnceAsync(CancellationToken cancellationToken)
    {
        _teleportProtocol.Reset();
        await ClientlessManager.UpdateStatusAsync(_account.ID, "Connecting", "Connecting gateway");
        _phase = "Gateway login";

        var redirect = await LoginGatewayAsync(cancellationToken);
        CloseSocket();

        await ClientlessManager.UpdateStatusAsync(_account.ID, "Connecting", $"Connecting agent {redirect.Host}:{redirect.Port}");
        _phase = "Agent connect";
        await ConnectAsync(redirect.Host, redirect.Port, cancellationToken);
        await CompleteInitialHandshakeAsync(cancellationToken);

        _phase = "Agent auth";
        var authMode = ResolveAuthMode();
        var authDelayMs = Math.Clamp(_account.AgentAuthDelayMs, 0, 10000);
        if (authDelayMs > 0)
        {
            Log.Verbose("Clientless {Account}/{Character} waiting {DelayMs}ms before Agent auth.",
                _account.AccountName,
                _account.CharacterName,
                authDelayMs);
            await Task.Delay(authDelayMs, cancellationToken);
        }

        await SendAgentAuthAsync(redirect.Token, authMode, cancellationToken);
        _phase = "Agent character selection";
        await WaitForAuthAndEnterGameAsync(cancellationToken);
        await SynchronizeInitialWorldAsync(cancellationToken);
        await ClientlessManager.UpdateStatusAsync(_account.ID, "Online", "Character entered game");
        _isOnline = true;
        Log.Information("Clientless character {Character} is online.", _account.CharacterName);
        // Coalesce the login burst, then apply the selected policy: either one
        // standalone form per character or real parties of up to eight.
        ClientlessManager.ScheduleManagedPartyBuild();
        // Combat-profile SQL can take several seconds when many accounts start
        // together. Keep the socket receive/keepalive pump alive while it loads;
        // otherwise the GameServer can close a perfectly authenticated character
        // before automation even gets a chance to run.
        var huntInitialization = _huntEngine.BeginAsync(cancellationToken);
        await KeepOnlineAsync(huntInitialization, cancellationToken);
    }

    private async Task SynchronizeInitialWorldAsync(CancellationToken cancellationToken)
    {
        var tcpClient = _tcpClient ?? throw new InvalidOperationException("Clientless socket is not connected.");
        var deadline = DateTime.UtcNow + InitialWorldSynchronizationTimeout;
        DateTime? quietDeadline = null;
        var sawGroupSpawnBegin = false;
        var initialGroupSpawnComplete = false;
        _phase = "Initial world synchronization";

        while (true)
        {
            var now = DateTime.UtcNow;
            if (initialGroupSpawnComplete && quietDeadline.HasValue && now >= quietDeadline.Value)
                break;

            if (now >= deadline)
            {
                if (initialGroupSpawnComplete)
                    break;

                throw new TimeoutException(
                    "Timed out waiting for the initial vSRO world-spawn cycle after 0x3012.");
            }

            // Never cancel an in-flight NetworkStream read just to detect the end
            // of the entry burst. On the supported Windows runtime that can abort
            // the socket and make the first online read fail even though the
            // character entered the world successfully. Poll only until either a
            // decrypted packet is queued or TCP bytes are ready, then perform a
            // normal session-lifetime read.
            if (!HasInitialWorldDataReady(_pendingPackets.Count, tcpClient.Available))
            {
                var nextDeadline = quietDeadline.HasValue && quietDeadline.Value < deadline
                    ? quietDeadline.Value
                    : deadline;
                var pollDelay = nextDeadline - now;
                if (pollDelay > TimeSpan.FromMilliseconds(25))
                    pollDelay = TimeSpan.FromMilliseconds(25);
                if (pollDelay <= TimeSpan.Zero)
                    continue;

                await Task.Delay(pollDelay, cancellationToken);
                continue;
            }

            var packet = await ReadPacketAsync(cancellationToken);
            await HandleOnlinePacketAsync(packet, cancellationToken);

            if (AdvanceInitialWorldSynchronization(packet.Opcode, ref sawGroupSpawnBegin))
                initialGroupSpawnComplete = true;

            // Once 0x3017/0x3018 has completed, keep consuming the remainder of
            // the entry burst. Sending a custom hunt/town teleport while the
            // GameServer is still finishing that burst can close the session.
            if (initialGroupSpawnComplete)
                quietDeadline = DateTime.UtcNow + InitialWorldQuietPeriod;
        }

        Log.Information(
            "Clientless character {Character} completed initial world synchronization; automation may start safely.",
            _account.CharacterName);
    }

    internal static bool HasInitialWorldDataReady(int pendingPacketCount, int availableSocketBytes)
    {
        return pendingPacketCount > 0 || availableSocketBytes > 0;
    }

    internal static bool AdvanceInitialWorldSynchronization(
        ushort opcode,
        ref bool sawGroupSpawnBegin)
    {
        if (opcode == 0x3017)
        {
            sawGroupSpawnBegin = true;
            return false;
        }

        return opcode == 0x3018 && sawGroupSpawnBegin;
    }

    private async Task<GatewayRedirect> LoginGatewayAsync(CancellationToken cancellationToken)
    {
        await ConnectAsync(GetProxyConnectHost(_gatewayService.BindIP), _gatewayService.BindPort, cancellationToken);

        var login = new Packet(0x6102, true, false);
        login.WriteUInt8(_account.Locale);
        login.WriteAscii(_account.AccountName.ToLowerInvariant());
        login.WriteAscii(_account.AccountPassword);
        login.WriteUInt16(_account.ShardID);
        await SendAsync(login, cancellationToken);

        while (true)
        {
            var packet = await ReadPacketAsync(cancellationToken);
            if (packet.Opcode == 0xA102)
            {
                var result = packet.ReadUInt8();
                if (result != 1)
                {
                    var errorCode = packet.RemainingRead() > 0
                        ? packet.ReadUInt8()
                        : (byte?)null;
                    var errorDetail = errorCode.HasValue
                        ? $", error code 0x{errorCode.Value:X2}"
                        : ", no error code supplied";
                    throw new InvalidOperationException(
                        $"Gateway login failed with result {result}{errorDetail}; response={Convert.ToHexString(packet.GetBytes())}");
                }

                var token = packet.ReadUInt32();
                var host = packet.ReadAscii();
                var port = packet.ReadUInt16();
                Log.Verbose("Clientless {Account}/{Character} gateway redirect token={Token} agent={AgentHost}:{AgentPort}",
                    _account.AccountName,
                    _account.CharacterName,
                    token,
                    host,
                    port);
                return new GatewayRedirect(token, host, port);
            }

            if (packet.Opcode == 0xA100)
                throw new InvalidOperationException("Gateway requested patch/download instead of login.");

            if (packet.Opcode == 0x2322 && _serverSettings.RemoveCaptcha)
            {
                var captcha = new Packet(0x6323);
                captcha.WriteAscii(_serverSettings.CaptchaValue);
                await SendAsync(captcha, cancellationToken);
            }
        }
    }

    private async Task SendAgentAuthAsync(uint token, AgentAuthMode mode, CancellationToken cancellationToken)
    {
        var auth = new Packet(0x6103, true, false);
        auth.WriteUInt32(token);
        switch (mode)
        {
            case AgentAuthMode.Full:
                auth.WriteAscii(_account.AccountName.ToLowerInvariant());
                auth.WriteAscii(_account.AccountPassword);
                auth.WriteUInt8(_account.Locale);
                WriteAgentAuthPadding(auth, writeDefaultTail: true);
                break;

            case AgentAuthMode.EmptyCredentials:
                auth.WriteAscii(string.Empty);
                auth.WriteAscii(string.Empty);
                auth.WriteUInt8(_account.Locale);
                WriteAgentAuthPadding(auth, writeDefaultTail: true);
                break;

            case AgentAuthMode.TokenOnly:
                break;

            case AgentAuthMode.TokenUserLocale:
                auth.WriteAscii(_account.AccountName.ToLowerInvariant());
                auth.WriteUInt8(_account.Locale);
                WriteAgentAuthPadding(auth, writeDefaultTail: false);
                break;
        }

        Log.Verbose("Clientless {Account}/{Character} using AgentAuthMode={AgentAuthMode}",
            _account.AccountName,
            _account.CharacterName,
            mode);
        await SendAsync(auth, cancellationToken);
    }

    private void WriteAgentAuthPadding(Packet auth, bool writeDefaultTail)
    {
        var raw = (_account.AgentAuthPaddingHex ?? string.Empty).Trim();
        if (raw.Length == 0)
        {
            if (writeDefaultTail)
            {
                auth.WriteUInt32(0u);
                auth.WriteUInt16(0);
            }

            return;
        }

        raw = raw.Replace(" ", string.Empty).Replace("-", string.Empty);
        if (raw.Length % 2 != 0)
            throw new InvalidOperationException("AgentAuthPaddingHex must contain an even number of hex characters.");

        var bytes = Convert.FromHexString(raw);
        if (bytes.Length > 128)
            throw new InvalidOperationException("AgentAuthPaddingHex is too long.");

        auth.WriteUInt8Array(bytes);
    }

    private async Task WaitForAuthAndEnterGameAsync(CancellationToken cancellationToken)
    {
        var charListRequested = false;
        var characterSelected = false;
        var gameReadySent = false;
        var collectingCharacterData = false;

        while (true)
        {
            var packet = await ReadPacketAsync(cancellationToken);
            switch (packet.Opcode)
            {
                case 0xA103:
                    if (packet.ReadUInt8() != 1)
                        throw new InvalidOperationException("Agent auth failed.");

                    if (!charListRequested)
                    {
                        var requestList = new Packet(0x7007);
                        requestList.WriteUInt8(2);
                        await SendAsync(requestList, cancellationToken);
                        charListRequested = true;
                    }
                    break;

                case 0xB007:
                    if (!characterSelected)
                    {
                        if (TryReadCharacterList(packet, out var exists, out var names) && !exists)
                        {
                            var available = names.Count == 0 ? "none" : string.Join(", ", names);
                            throw new InvalidOperationException(
                                $"Character '{_account.CharacterName}' was not found on account '{_account.AccountName}'. Available: {available}");
                        }

                        var select = new Packet(0x7001, true, false);
                        select.WriteAscii(_account.CharacterName);
                        _phase = "Character select";
                        await SendAsync(select, cancellationToken);
                        characterSelected = true;
                    }
                    break;

                case 0xB001:
                    var selectResult = packet.ReadUInt8();
                    if (selectResult != 1)
                        throw new InvalidOperationException($"Character select failed with result {selectResult}");
                    break;

                case ClientlessTeleportProtocol.ServerCharacterDataBegin:
                    _characterDataBytes.SetLength(0);
                    collectingCharacterData = true;
                    _huntEngine.BeginCharacterDataTransfer();
                    _phase = "Loading character data";
                    break;

                case 0x3013:
                    if (collectingCharacterData)
                        _characterDataBytes.Write(packet.GetBytes());
                    break;

                case ClientlessTeleportProtocol.ServerCharacterDataEnd:
                    if (collectingCharacterData && !gameReadySent)
                    {
                        collectingCharacterData = false;
                        var characterData = new Packet(
                            ClientlessTeleportProtocol.ServerCharacterData,
                            false,
                            false,
                            _characterDataBytes.ToArray());
                        characterData.ToReadOnly();
                        _huntEngine.CompleteCharacterDataTransfer(characterData);
                        _characterDataBytes.SetLength(0);
                        // vSRO emits 0x34A6 only after the complete massive
                        // character payload has finished. Confirming on 0x3013
                        // is too early and can leave the player connected but
                        // absent from the world for nearby clients.
                        _phase = "Game ready";
                        await SendAsync(new Packet(0x3012), cancellationToken);
                        gameReadySent = true;
                        return;
                    }
                    break;
            }
        }
    }

    private async Task KeepOnlineAsync(
        Task huntInitialization,
        CancellationToken cancellationToken)
    {
        Task<Packet>? receiveTask = null;
        Task pingDelay = Task.Delay(TimeSpan.FromSeconds(5), cancellationToken);
        Task huntDelay = Task.Delay(ClientlessHuntEngine.TickInterval, cancellationToken);
        var huntReady = false;

        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                receiveTask ??= ReadPacketAsync(cancellationToken);
                if (!huntReady)
                    await Task.WhenAny(receiveTask, pingDelay, huntDelay, huntInitialization);
                else
                    await Task.WhenAny(receiveTask, pingDelay, huntDelay);

                if (receiveTask.IsCompleted)
                {
                    _phase = "Online receive";
                    var packet = await receiveTask;
                    receiveTask = null;
                    await HandleOnlinePacketAsync(packet, cancellationToken);
                }

                if (!huntReady && huntInitialization.IsCompleted)
                {
                    _phase = "Online automation ready";
                    await huntInitialization;
                    huntReady = true;
                }

                if (huntDelay.IsCompleted)
                {
                    await huntDelay;
                    ApplyPendingHuntingConfiguration();
                    if (huntReady)
                    {
                        _phase = "Online hunting";
                        await _huntEngine.TickAsync(cancellationToken);
                    }
                    huntDelay = Task.Delay(ClientlessHuntEngine.TickInterval, cancellationToken);
                }

                if (pingDelay.IsCompleted)
                {
                    await pingDelay;
                    _phase = "Online keepalive";
                    await SendAsync(new Packet(0x2002), cancellationToken);
                    pingDelay = Task.Delay(TimeSpan.FromSeconds(5), cancellationToken);
                }
            }
        }
        finally
        {
            if (receiveTask != null && cancellationToken.IsCancellationRequested)
            {
                try { await receiveTask; }
                catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
                catch { }
            }
        }
    }

    private void ApplyPendingHuntingConfiguration()
    {
        var pending = Interlocked.Exchange(ref _pendingHuntingConfiguration, null);
        if (pending == null)
            return;

        var assignedAreaChanged = ClientlessManager.CopyHuntingConfiguration(_account, pending);
        _huntEngine.RefreshConfiguration(assignedAreaChanged);
    }

    private async Task ConnectAsync(string host, int port, CancellationToken cancellationToken)
    {
        _security = new Security();
        _security.ChangeIdentity("SR_Client", 0);
        _tcpClient = new TcpClient { NoDelay = true };
        _pendingPackets.Clear();
        _lastReceivedOpcode = 0;
        _lastSentOpcode = 0;
        _partyFormRequested = false;
        _partyFormDesired = false;
        _partyFormDeleteRequested = false;
        _partyMatchingId = 0;
        _partyFormAttemptCount = 0;
        _partyFormPolicy = null;
        _huntEngine.Reset();
        await _tcpClient.ConnectAsync(host, port, cancellationToken);
        Log.Verbose("Clientless {Account}/{Character} connected to {Host}:{Port}",
            _account.AccountName,
            _account.CharacterName,
            host,
            port);
    }

    private static string GetProxyConnectHost(string bindHost)
    {
        if (string.IsNullOrWhiteSpace(bindHost))
            return System.Net.IPAddress.Loopback.ToString();

        if (!System.Net.IPAddress.TryParse(bindHost, out var address))
            return bindHost;

        return System.Net.IPAddress.Any.Equals(address) || System.Net.IPAddress.IPv6Any.Equals(address)
            ? System.Net.IPAddress.Loopback.ToString()
            : bindHost;
    }

    private async Task<Packet> ReadPacketAsync(CancellationToken cancellationToken)
    {
        var tcpClient = _tcpClient ?? throw new InvalidOperationException("Clientless socket is not connected.");
        var security = _security ?? throw new InvalidOperationException("Clientless security is not initialized.");

        while (true)
        {
            if (_pendingPackets.Count > 0)
                return _pendingPackets.Dequeue();

            var count = await tcpClient.GetStream().ReadAsync(_buffer, cancellationToken);
            if (count == 0)
                throw new IOException(
                    $"Remote server closed the connection at {_phase}. Last sent=0x{_lastSentOpcode:X4}, last received=0x{_lastReceivedOpcode:X4}.");

            lock (_securitySync)
                security.Recv(_buffer, 0, count);
            await FlushAsync(cancellationToken);

            List<Packet>? packets;
            lock (_securitySync)
                packets = security.TransferIncoming();
            if (packets == null)
                continue;

            foreach (var packet in packets)
            {
                if (packet.Opcode is 0x5000 or 0x9000 or 0x2001 or 0x2005 or 0x6005)
                    continue;

                _lastReceivedOpcode = packet.Opcode;
                Log.Verbose("Clientless {Account}/{Character} received 0x{Opcode:X4} len={Length} at {Phase}",
                    _account.AccountName,
                    _account.CharacterName,
                    packet.Opcode,
                    packet.GetBytes().Length,
                    _phase);
                _pendingPackets.Enqueue(packet);
            }
        }
    }

    private async Task CompleteInitialHandshakeAsync(CancellationToken cancellationToken)
    {
        var tcpClient = _tcpClient ?? throw new InvalidOperationException("Clientless socket is not connected.");
        var security = _security ?? throw new InvalidOperationException("Clientless security is not initialized.");
        var deadline = DateTime.UtcNow.AddSeconds(5);
        _phase = "Agent handshake";

        while (DateTime.UtcNow < deadline)
        {
            if (tcpClient.Available == 0)
            {
                await Task.Delay(25, cancellationToken);
                continue;
            }

            var count = await tcpClient.GetStream().ReadAsync(_buffer, cancellationToken);
            if (count == 0)
                throw new IOException("Remote server closed the connection during Agent handshake.");

            lock (_securitySync)
                security.Recv(_buffer, 0, count);
            await FlushAsync(cancellationToken);

            List<Packet>? packets;
            lock (_securitySync)
                packets = security.TransferIncoming();
            if (packets == null)
                continue;

            foreach (var packet in packets)
            {
                Log.Verbose("Clientless {Account}/{Character} handshake received 0x{Opcode:X4} len={Length}",
                    _account.AccountName,
                    _account.CharacterName,
                    packet.Opcode,
                    packet.GetBytes().Length);

                if (packet.Opcode is 0x5000 or 0x9000 or 0x2001 or 0x2005 or 0x6005)
                    continue;

                _lastReceivedOpcode = packet.Opcode;
                _pendingPackets.Enqueue(packet);
            }

            // 0x2001 is emitted after accepted handshake. Give delayed module packets
            // one small read window, then continue with Agent auth.
            if (packets.Any(p => p.Opcode == 0x2001))
            {
                await Task.Delay(100, cancellationToken);
                while (tcpClient.Available > 0)
                {
                    count = await tcpClient.GetStream().ReadAsync(_buffer, cancellationToken);
                    if (count == 0)
                        throw new IOException("Remote server closed the connection after Agent handshake.");

                    lock (_securitySync)
                        security.Recv(_buffer, 0, count);
                    await FlushAsync(cancellationToken);

                    List<Packet>? extraPackets;
                    lock (_securitySync)
                        extraPackets = security.TransferIncoming();
                    if (extraPackets == null)
                        continue;

                    foreach (var packet in extraPackets)
                    {
                        Log.Verbose("Clientless {Account}/{Character} post-handshake received 0x{Opcode:X4} len={Length}",
                            _account.AccountName,
                            _account.CharacterName,
                            packet.Opcode,
                            packet.GetBytes().Length);

                        if (packet.Opcode is 0x5000 or 0x9000 or 0x2001 or 0x2005 or 0x6005)
                            continue;

                        _lastReceivedOpcode = packet.Opcode;
                        _pendingPackets.Enqueue(packet);
                    }
                }

                return;
            }
        }

        throw new TimeoutException("Timed out waiting for Agent handshake.");
    }

    private async Task SendAsync(Packet packet, CancellationToken cancellationToken)
    {
        var security = _security ?? throw new InvalidOperationException("Clientless security is not initialized.");
        _lastSentOpcode = packet.Opcode;
        Log.Verbose("Clientless {Account}/{Character} sending 0x{Opcode:X4} len={Length} at {Phase}",
            _account.AccountName,
            _account.CharacterName,
            packet.Opcode,
            packet.GetBytes().Length,
            _phase);
        lock (_securitySync)
            security.Send(packet);
        await FlushAsync(cancellationToken);
    }

    public async Task<bool> ApplyPartyFormPolicyAsync(
        ClientlessManager.PartyFormPolicy policy)
    {
        if (!string.IsNullOrWhiteSpace(SystemRole))
            return false;

        try
        {
            _partyFormPolicy = policy;
            _partyFormDesired = policy.Enabled;
            if (!_isOnline || _tcpClient == null || _security == null)
                return false;

            if (!policy.Enabled)
            {
                if (_partyMatchingId == 0)
                {
                    _partyFormRequested = false;
                    return false;
                }
                if (_partyFormDeleteRequested)
                    return false;

                var delete = new Packet(0x706B, true, false);
                delete.WriteUInt32(_partyMatchingId);
                await SendAsync(delete, _sessionShutdown.Token);
                _partyFormDeleteRequested = true;
                Log.Information("Clientless {Account}/{Character} requested Party Form removal (MatchingID={MatchingID}).",
                    _account.AccountName,
                    _account.CharacterName,
                    _partyMatchingId);
                return true;
            }

            if (_partyFormRequested || _partyMatchingId != 0)
                return false;

            var title = ResolvePartyFormTitle(policy, _account.CharacterName);
            var (effectiveMinLevel, effectiveMaxLevel) = ResolvePartyFormLevelRange(policy);
            var form = BuildPartyFormPacket(policy, _account.CharacterName, _managedPartyId);
            await SendAsync(form, _sessionShutdown.Token);
            _partyFormRequested = true;
            _partyFormAttemptCount++;
            Log.Information(
                "Clientless {Account}/{Character} requested Party Form creation: {Title} (attempt={Attempt}, levels={MinLevel}-{MaxLevel}, purpose={Purpose}, settings={Settings}).",
                _account.AccountName,
                _account.CharacterName,
                title,
                _partyFormAttemptCount,
                effectiveMinLevel,
                effectiveMaxLevel,
                policy.Purpose,
                policy.SettingsFlag);
            return true;
        }
        catch (OperationCanceledException) when (_sessionShutdown.IsCancellationRequested)
        {
            return false;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Account}/{Character} Party Form action failed.",
                _account.AccountName,
                _account.CharacterName);
            return false;
        }
    }

    internal void ExpectManagedPartyInvite(uint leaderUniqueId)
    {
        _expectedManagedPartyLeaderUniqueId = leaderUniqueId;
    }

    internal async Task LeaveManagedPartyAsync(CancellationToken cancellationToken)
    {
        _expectedManagedPartyLeaderUniqueId = 0;
        if (!_isOnline || _managedPartyMemberCount <= 0)
            return;

        await SendAsync(BuildManagedPartyLeavePacket(), cancellationToken);
        _managedPartyMemberCount = 0;
        _managedPartyId = 0;
    }

    internal async Task RemoveManagedPartyFormAsync(CancellationToken cancellationToken)
    {
        _partyFormDesired = false;
        _partyFormRequested = false;
        if (!_isOnline || _partyMatchingId == 0 || _partyFormDeleteRequested)
            return;

        var delete = new Packet(0x706B, true, false);
        delete.WriteUInt32(_partyMatchingId);
        await SendAsync(delete, cancellationToken);
        _partyFormDeleteRequested = true;
    }

    internal async Task WaitForManagedPartyFormRemovalAsync(
        TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        var deadline = DateTime.UtcNow + timeout;
        while (_partyMatchingId != 0 && DateTime.UtcNow < deadline)
            await Task.Delay(100, cancellationToken);
    }

    internal async Task<bool> PublishManagedPartyFormAsync(
        ClientlessManager.PartyFormPolicy policy,
        CancellationToken cancellationToken)
    {
        if (!_isOnline || _managedPartyId == 0 || _managedPartyMemberCount < 2)
            return false;

        _partyFormPolicy = policy;
        _partyFormDesired = true;
        _partyFormRequested = false;
        if (_partyMatchingId != 0)
            return false;

        var form = BuildPartyFormPacket(policy, _account.CharacterName, _managedPartyId);
        await SendAsync(form, cancellationToken);
        _partyFormRequested = true;
        _partyFormAttemptCount++;
        return true;
    }

    internal async Task InviteManagedPartyMemberAsync(
        uint targetUniqueId,
        byte settingsFlag,
        CancellationToken cancellationToken)
    {
        if (!_isOnline || targetUniqueId == 0)
            throw new InvalidOperationException("Both Clientless party members must be fully online before an invite can be sent.");

        var invite = BuildManagedPartyInvitePacket(
            targetUniqueId,
            creatingParty: _managedPartyMemberCount <= 0,
            settingsFlag: settingsFlag);
        await SendAsync(invite, cancellationToken);
    }

    internal async Task<bool> WaitForManagedPartySizeAsync(
        int minimumMembers,
        TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        var deadline = DateTime.UtcNow + timeout;
        while (DateTime.UtcNow < deadline && !cancellationToken.IsCancellationRequested)
        {
            if (_managedPartyMemberCount >= minimumMembers)
                return true;
            await Task.Delay(100, cancellationToken);
        }
        return _managedPartyMemberCount >= minimumMembers;
    }

    private async Task HandleOnlinePacketAsync(Packet packet, CancellationToken cancellationToken)
    {
        if (packet.Opcode == ClientlessTeleportProtocol.ServerCharacterDataBegin)
        {
            _characterDataBytes.SetLength(0);
            _huntEngine.BeginCharacterDataTransfer();
        }
        else if (packet.Opcode == ClientlessTeleportProtocol.ServerCharacterData)
        {
            _characterDataBytes.Write(packet.GetBytes());
        }
        else if (packet.Opcode == ClientlessTeleportProtocol.ServerCharacterDataEnd &&
                 _characterDataBytes.Length > 0)
        {
            var characterData = new Packet(
                ClientlessTeleportProtocol.ServerCharacterData,
                false,
                false,
                _characterDataBytes.ToArray());
            characterData.ToReadOnly();
            _huntEngine.CompleteCharacterDataTransfer(characterData);
            _characterDataBytes.SetLength(0);
        }
        else
        {
            await _huntEngine.HandlePacketAsync(packet, cancellationToken);
        }

        var teleportResponseOpcode = _teleportProtocol.HandleServerOpcode(packet.Opcode);
        if (teleportResponseOpcode.HasValue)
        {
            _phase = ClientlessTeleportProtocol.IsTeleportReadyResponse(teleportResponseOpcode.Value)
                ? "Teleport ready"
                : "Teleport game ready";
            await SendAsync(new Packet(teleportResponseOpcode.Value), cancellationToken);

            if (ClientlessTeleportProtocol.IsTeleportReadyResponse(teleportResponseOpcode.Value))
            {
                Log.Information(
                    "Clientless character {Character} acknowledged vSRO teleport reset with 0x{Opcode:X4}.",
                    _account.CharacterName,
                    teleportResponseOpcode.Value);
            }
            else if (teleportResponseOpcode.Value == ClientlessTeleportProtocol.ClientCharacterConfirmSpawn)
            {
                Log.Information(
                    "Clientless character {Character} completed the teleport loading cycle.",
                    _account.CharacterName);
            }
            return;
        }

        if (await TryHandleManagedPartyPacketAsync(packet, cancellationToken))
            return;

        if (packet.Opcode == 0xB06B)
        {
            var result = packet.ReadUInt8();
            _partyFormDeleteRequested = false;
            if (result == 1)
            {
                _partyMatchingId = 0;
                _partyFormRequested = false;
                if (_partyFormDesired && _partyFormPolicy != null)
                    await ApplyPartyFormPolicyAsync(_partyFormPolicy);
            }
            else
            {
                Log.Warning("Clientless {Account}/{Character} Party Form removal was rejected (result={Result}).",
                    _account.AccountName,
                    _account.CharacterName,
                    result);
            }
            return;
        }

        if (packet.Opcode != 0xB069)
            return;

        try
        {
            var responseHex = Convert.ToHexString(packet.GetBytes());
            var result = packet.ReadUInt8();
            if (result == 1)
            {
                _partyMatchingId = packet.ReadUInt32();
                _partyFormRequested = true;
                _partyFormAttemptCount = 0;
                Log.Information("Clientless {Account}/{Character} Party Form created (MatchingID={MatchingID}).",
                    _account.AccountName,
                    _account.CharacterName,
                    _partyMatchingId);
                if (!_partyFormDesired)
                {
                    var delete = new Packet(0x706B, true, false);
                    delete.WriteUInt32(_partyMatchingId);
                    await SendAsync(delete, _sessionShutdown.Token);
                    _partyFormDeleteRequested = true;
                }
            }
            else
            {
                ushort? errorCode = result == 2 ? packet.ReadUInt16() : null;
                _partyFormRequested = false;
                var shouldRetry = _partyFormDesired &&
                                  _partyFormPolicy != null &&
                                  _partyFormAttemptCount < 3;
                Log.Warning(
                    "Clientless {Account}/{Character} Party Form was rejected (result={Result}, errorCode={ErrorCode}, attempt={Attempt}, retry={Retry}, payload={Payload}).",
                    _account.AccountName,
                    _account.CharacterName,
                    result,
                    errorCode,
                    _partyFormAttemptCount,
                    shouldRetry,
                    responseHex);

                if (shouldRetry)
                {
                    _ = RetryPartyFormAsync();
                }
            }
        }
        catch (Exception ex)
        {
            _partyFormRequested = false;
            Log.Warning(ex, "Clientless {Account}/{Character} Party Form response could not be parsed.",
                _account.AccountName,
                _account.CharacterName);
        }
    }

    private async Task<bool> TryHandleManagedPartyPacketAsync(
        Packet source,
        CancellationToken cancellationToken)
    {
        if (source.Opcode == 0x3080)
        {
            var packet = new Packet(source);
            packet.ToReadOnly();
            var requestType = packet.ReadUInt8();
            var inviterUniqueId = packet.ReadUInt32();
            if (requestType is not 2 and not 3)
                return false;

            var expectedLeader = _expectedManagedPartyLeaderUniqueId;
            if (expectedLeader == 0 || inviterUniqueId != expectedLeader)
            {
                Log.Warning(
                    "Clientless {Character} ignored an unmanaged party invitation from UID {InviterUniqueId}; expected UID {ExpectedUniqueId}.",
                    _account.CharacterName,
                    inviterUniqueId,
                    expectedLeader);
                return true;
            }

            var accept = BuildManagedPartyAcceptPacket();
            await SendAsync(accept, cancellationToken);
            Log.Information(
                "Clientless {Character} accepted its managed party invitation from UID {LeaderUniqueId}.",
                _account.CharacterName,
                inviterUniqueId);
            return true;
        }

        if (source.Opcode == 0x3065)
        {
            try
            {
                var packet = new Packet(source);
                packet.ToReadOnly();
                packet.ReadUInt8(); // 0xFF
                _managedPartyId = packet.ReadUInt32(); // party id (vSRO)
                packet.ReadUInt32(); // leader member id
                packet.ReadUInt8(); // settings
                _managedPartyMemberCount = packet.ReadUInt8();
            }
            catch (Exception ex)
            {
                _managedPartyMemberCount = 0;
                _managedPartyId = 0;
                Log.Warning(ex, "Clientless {Character} could not parse vSRO party creation data.", _account.CharacterName);
            }
            return true;
        }

        if (source.Opcode == 0x3864)
        {
            try
            {
                var packet = new Packet(source);
                packet.ToReadOnly();
                var updateType = packet.ReadUInt8();
                switch (updateType)
                {
                    case 1: // dismissed
                        _managedPartyMemberCount = 0;
                        _managedPartyId = 0;
                        break;
                    case 2: // member joined
                        _managedPartyMemberCount = Math.Max(1, _managedPartyMemberCount + 1);
                        break;
                    case 3: // member left
                        _managedPartyMemberCount = Math.Max(0, _managedPartyMemberCount - 1);
                        break;
                }
            }
            catch (Exception ex)
            {
                Log.Debug(ex, "Clientless {Character} could not parse a party update.", _account.CharacterName);
            }
            return true;
        }

        return false;
    }

    internal static Packet BuildManagedPartyInvitePacket(
        uint targetUniqueId,
        bool creatingParty,
        byte settingsFlag)
    {
        var invite = new Packet(creatingParty ? (ushort)0x7060 : (ushort)0x7062);
        invite.WriteUInt32(targetUniqueId);
        if (creatingParty)
            invite.WriteUInt8((byte)(settingsFlag | 0x04));
        return invite;
    }

    internal static Packet BuildManagedPartyAcceptPacket()
    {
        var accept = new Packet(0x3080);
        accept.WriteUInt8(1);
        accept.WriteUInt8(1);
        return accept;
    }

    internal static Packet BuildManagedPartyLeavePacket() => new(0x7061);

    private async Task RetryPartyFormAsync()
    {
        try
        {
            await Task.Delay(TimeSpan.FromSeconds(5), _sessionShutdown.Token);
            if (_partyFormDesired && _partyFormPolicy != null && _isOnline)
                await ApplyPartyFormPolicyAsync(_partyFormPolicy);
        }
        catch (OperationCanceledException) when (_sessionShutdown.IsCancellationRequested)
        {
        }
        catch (Exception ex)
        {
            Log.Warning(ex,
                "Clientless {Account}/{Character} Party Form retry failed without interrupting the online packet loop.",
                _account.AccountName,
                _account.CharacterName);
        }
    }

    internal static Packet BuildPartyFormPacket(
        ClientlessManager.PartyFormPolicy policy,
        string characterName,
        uint partyId = 0)
    {
        var title = ResolvePartyFormTitle(policy, characterName);
        var (minLevel, maxLevel) = ResolvePartyFormLevelRange(policy);

        var form = new Packet(0x7069, true, false);
        form.WriteUInt32(0);
        form.WriteUInt32(partyId);
        form.WriteUInt8(policy.SettingsFlag);
        form.WriteUInt8(policy.Purpose);
        form.WriteUInt8(minLevel);
        form.WriteUInt8(maxLevel);
        form.WriteAscii(title);
        return form;
    }

    internal static string ResolvePartyFormTitle(
        ClientlessManager.PartyFormPolicy policy,
        string characterName)
    {
        var title = policy.Title
            .Replace("{CharacterName}", characterName, StringComparison.OrdinalIgnoreCase)
            .Trim();
        if (title.Length > 64)
            title = title[..64];
        return title;
    }

    internal static (byte MinLevel, byte MaxLevel) ResolvePartyFormLevelRange(
        ClientlessManager.PartyFormPolicy policy)
    {
        var configuredServerCap = _serverSettings.ServerMaxLevel;
        var maxLevel = configuredServerCap is > 0 and <= byte.MaxValue
            ? Math.Min(policy.MaxLevel, configuredServerCap)
            : policy.MaxLevel;
        var minLevel = Math.Min(policy.MinLevel, maxLevel);
        return (checked((byte)minLevel), checked((byte)maxLevel));
    }

    private AgentAuthMode ResolveAuthMode()
    {
        var configured = (_account.AgentAuthMode ?? "Auto").Trim();
        if (configured.Equals("Auto", StringComparison.OrdinalIgnoreCase) ||
            string.IsNullOrWhiteSpace(configured))
        {
            return AgentAuthMode.Full;
        }

        if (Enum.TryParse<AgentAuthMode>(configured, ignoreCase: true, out var explicitMode))
            return explicitMode;

        return AgentAuthMode.Full;
    }

    private bool TryReadCharacterList(Packet packet, out bool targetExists, out List<string> names)
    {
        targetExists = false;
        names = new List<string>();

        try
        {
            var reader = new Packet(packet);
            reader.ToReadOnly();

            var action = reader.ReadUInt8();
            if (action != 0x02)
                return false;

            var result = reader.ReadUInt8();
            if (result != 0x01)
                throw new InvalidOperationException($"Character list request failed with result {result}");

            var count = reader.ReadUInt8();
            for (var i = 0; i < count; i++)
            {
                reader.ReadUInt32();
                var name = reader.ReadAscii();
                names.Add(name);
                if (name.Equals(_account.CharacterName, StringComparison.OrdinalIgnoreCase))
                    targetExists = true;

                reader.ReadUInt8();
                reader.ReadUInt8();
                reader.ReadUInt64();
                reader.ReadUInt16();
                reader.ReadUInt16();
                reader.ReadUInt16();
                reader.ReadUInt32();
                reader.ReadUInt32();

                var deleteFlag = reader.ReadUInt8();
                if (deleteFlag == 1)
                    reader.ReadUInt32();

                reader.ReadUInt8();
                reader.ReadUInt8();
                reader.ReadUInt8();

                var itemCount = reader.ReadUInt8();
                for (var item = 0; item < itemCount; item++)
                {
                    reader.ReadUInt32();
                    reader.ReadUInt8();
                }

                var avatarCount = reader.ReadUInt8();
                for (var avatar = 0; avatar < avatarCount; avatar++)
                {
                    reader.ReadUInt32();
                    reader.ReadUInt8();
                }
            }

            Log.Verbose("Clientless {Account}/{Character} character list count={Count}, targetFound={Found}",
                _account.AccountName,
                _account.CharacterName,
                names.Count,
                targetExists);
            return true;
        }
        catch (InvalidOperationException)
        {
            throw;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Account}/{Character} could not parse character list; continuing with configured character name.",
                _account.AccountName,
                _account.CharacterName);
            return false;
        }
    }

    private async Task FlushAsync(CancellationToken cancellationToken)
    {
        var tcpClient = _tcpClient;
        var security = _security;
        if (tcpClient == null || security == null)
            return;

        await _sendLock.WaitAsync(cancellationToken);
        try
        {
            List<KeyValuePair<TransferBuffer, Packet>>? outgoing;
            lock (_securitySync)
                outgoing = security.TransferOutgoing();
            if (outgoing == null)
                return;

            foreach (var pair in outgoing)
                await tcpClient.GetStream().WriteAsync(pair.Key.Buffer.AsMemory(0, pair.Key.Size), cancellationToken);
        }
        finally
        {
            _sendLock.Release();
        }
    }

    private void CloseSocket()
    {
        _isOnline = false;
        _managedPartyMemberCount = 0;
        _managedPartyId = 0;
        _expectedManagedPartyLeaderUniqueId = 0;
        _characterDataBytes.SetLength(0);
        try { _tcpClient?.Close(); } catch { }
        _tcpClient = null;
        _security = null;
        _pendingPackets.Clear();
    }

    public void Dispose()
    {
        try { _sessionShutdown.Cancel(); } catch { }
        _huntEngine.Reset();
        CloseSocket();
    }

    private readonly record struct GatewayRedirect(uint Token, string Host, ushort Port);
}
