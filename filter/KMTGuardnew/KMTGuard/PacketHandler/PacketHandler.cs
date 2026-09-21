using KMTGuard.SessionManager;
using Serilog;
using SilkroadSecurityAPI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.PacketHandlerManager
{
    public class PacketHandler : IPacketHandler
    {
        public PacketHandler(HashSet<ushort> clientWhitelist, HashSet<ushort> clientBlacklist)
        {
            SetDefaultHandler(HandleDefault);
            SetBlockHandler(HandleBlock);
            SetDisconnectHandler(HandleDisconnect);
            _clientWhitelist = clientWhitelist;
            _clientBlacklist = clientBlacklist;
        }

        public HashSet<ushort> _clientBlacklist { get; init; }
        public SortedDictionary<ushort, SortedDictionary<int, _PacketHandler>> _clientHandlers { get; init; } = new();
        public HashSet<ushort> _clientWhitelist { get; init; }
        public SortedDictionary<ushort, SortedDictionary<int, _PacketHandler>> _serverHandlers { get; init; } = new();

        public _PacketHandler _blockHandler { get; set; } = null!;
        public _PacketHandler _defaultHandler { get; set; } = null!;
        public _PacketHandler _disconnectHandler { get; set; } = null!;

        public void RegisterModuleHandler(ushort msgId, _PacketHandler handler)
        {
            RegisterModuleHandler(msgId, 1, handler);
        }

        public void RegisterModuleHandler(ushort msgId, int priority, _PacketHandler handler)
        {
            // make sure its not negative
            if (priority < 0)
            {
                priority = 1;
            }

            if (!_serverHandlers.ContainsKey(msgId))
            {
                _serverHandlers.TryAdd(msgId, new SortedDictionary<int, _PacketHandler>());
            }


            if (_serverHandlers[msgId].ContainsKey(priority))
            {
                foreach (var keyValuePair in _serverHandlers[msgId])
                {

                    if (keyValuePair.Key <= priority)
                    {
                        priority = keyValuePair.Key + 1;
                        continue;
                    }

                    if (_serverHandlers[msgId].ContainsKey(priority))
                    {
                        priority = keyValuePair.Key + 1;
                        continue;
                    }

                    break;
                }
            }

            _serverHandlers[msgId].TryAdd(priority, handler);
        }

        public void UnregisterModuleHandler(ushort msgId, _PacketHandler handler)
        {
            if (!_serverHandlers.ContainsKey(msgId)) return;

            var keysToRemove = _serverHandlers[msgId].Where(m => m.Value.Equals(handler)).Select(c => c.Key).ToList();
            keysToRemove.ForEach(key => _serverHandlers[msgId].Remove(key, out var tempObject));
        }

        public void UnregisterAllModuleHandler(ushort msgId)
        {
            if (!_serverHandlers.ContainsKey(msgId)) return;

            _serverHandlers[msgId].Clear();
        }

        public void RegisterClientHandler(ushort msgId, _PacketHandler handler)
        {
            RegisterClientHandler(msgId, 1, handler);
        }

        public void RegisterClientHandler(ushort msgId, int priority, _PacketHandler handler)
        {
            // make sure its not negative
            if (priority < 0)
            {
                priority = 1;
            }

            if (!_clientHandlers.ContainsKey(msgId))
            {
                _clientHandlers.TryAdd(msgId, new SortedDictionary<int, _PacketHandler>());
            }

            if (_clientHandlers[msgId].ContainsKey(priority))
            {
                foreach (var keyValuePair in _clientHandlers[msgId])
                {

                    if (keyValuePair.Key <= priority)
                    {
                        priority = keyValuePair.Key + 1;
                        continue;
                    }

                    if (_clientHandlers[msgId].ContainsKey(priority))
                    {
                        priority = keyValuePair.Key + 1;
                        continue;
                    }

                    break;
                }
            }

            _clientHandlers[msgId].TryAdd(priority, handler);
        }

        public void UnregisterClientHandler(ushort msgId, _PacketHandler handler)
        {
            if (!_clientHandlers.ContainsKey(msgId)) return;

            var keysToRemove = _clientHandlers[msgId].Where(m => m.Value.Equals(handler)).Select(c => c.Key).ToList();
            keysToRemove.ForEach(key => _clientHandlers[msgId].Remove(key, out var tempObject));
        }

        public void UnregisterAllClientHandler(ushort msgId)
        {
            if (!_clientHandlers.ContainsKey(msgId)) return;

            _clientHandlers[msgId].Clear();
        }

        public void SetDefaultHandler(_PacketHandler handler)
        {
            _defaultHandler = handler;
        }

        public Task<PacketResult> HandleDefault(Packet packet, ISession session, PacketData data)
        {
            return Task.FromResult(new PacketResult(data?.Data));
        }

        public void SetBlockHandler(_PacketHandler handler)
        {
            _blockHandler = handler;
        }

        public Task<PacketResult> HandleDisconnect(Packet packet, ISession session, PacketData data)
        {
            return Task.FromResult(new PacketResult(data?.Data, PacketResultType.Disconnect));
        }

        public void SetDisconnectHandler(_PacketHandler handler)
        {
            _disconnectHandler = handler;
        }

        public Task<PacketResult> HandleBlock(Packet packet, ISession session, PacketData data)
        {
            return Task.FromResult(new PacketResult(data?.Data, PacketResultType.Block));
        }

        public async Task<PacketResult> HandleClient(Packet packet, ISession session)
        {
            if (packet.Opcode == 0x9000 || packet.Opcode == 0x5000 || packet.Opcode == 0x2001)
                return await InvokeHandlerSafeAsync(_defaultHandler, packet, session, PacketData.Empty);

            // automatically blocks all packets that are not on the Whitelists!
            if (_clientBlacklist.Contains(packet.Opcode))
            {
                return await InvokeHandlerSafeAsync(_disconnectHandler, packet, session!, PacketData.Empty);
            }

            if (!_clientWhitelist.Contains(packet.Opcode))
                return await InvokeHandlerSafeAsync(_blockHandler, packet, session, PacketData.Empty);

            _clientHandlers.TryGetValue(packet.Opcode, out var handlers);
            return await ExecutePipeline(packet, session, handlers);
        }

        public async Task<PacketResult> HandleServer(Packet packet, ISession session)
        {
            _serverHandlers.TryGetValue(packet.Opcode, out var handlers);
            return await ExecutePipeline(packet, session, handlers);
        }

        private async Task<PacketResult> ExecutePipeline(
            Packet originalPacket,
            ISession session,
            SortedDictionary<int, _PacketHandler>? handlers)
        {
            var outcome = await InvokeHandlerSafeAsync(
                _defaultHandler,
                originalPacket,
                session,
                new PacketData(null, -1, PacketResultType.Nothing));

            if (handlers == null || handlers.Count == 0)
                return outcome;

            Packet currentPacket = originalPacket;
            object? carriedData = outcome.Data?.Data;
            int previousPriority = -1;
            PacketResultType previousResult = outcome.PacketResultType;
            bool hasOverride = false;

            foreach (var registeredHandler in handlers)
            {
                var readablePacket = new Packet(currentPacket);
                readablePacket.ToReadOnly();

                outcome = await InvokeHandlerSafeAsync(
                    registeredHandler.Value,
                    readablePacket,
                    session,
                    new PacketData(carriedData, previousPriority, previousResult));

                carriedData = outcome.Data?.Data;
                previousPriority = registeredHandler.Key;
                previousResult = outcome.PacketResultType;

                switch (outcome.PacketResultType)
                {
                    case PacketResultType.Disconnect:
                        return await _disconnectHandler(
                            currentPacket,
                            session,
                            new PacketData(carriedData, previousPriority, PacketResultType.Disconnect));

                    case PacketResultType.Block:
                        return await _blockHandler(
                            currentPacket,
                            session,
                            new PacketData(carriedData, previousPriority, PacketResultType.Block));

                    case PacketResultType.Override:
                        if (outcome.OverridePacket == null)
                        {
                            return await _disconnectHandler(
                                currentPacket,
                                session,
                                new PacketData(carriedData, previousPriority, PacketResultType.Disconnect));
                        }

                        currentPacket = outcome.OverridePacket;
                        hasOverride = true;
                        break;

                    case PacketResultType.Nothing:
                        break;

                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }

            if (hasOverride)
            {
                return new PacketResult(
                    new PacketData(carriedData, previousPriority, previousResult),
                    currentPacket,
                    PacketResultType.Override);
            }

            return outcome;
        }

        public void Dispose()
        {
            _clientHandlers.Clear();
            _serverHandlers.Clear();
            _clientWhitelist.Clear();
            _clientBlacklist.Clear();
        }

        private async Task<PacketResult> InvokeHandlerSafeAsync(
            _PacketHandler handler,
            Packet packet,
            ISession session,
            PacketData data)
        {
            try
            {
                var result = await handler(packet, session, data);
                if (result != null)
                    return result;

                Log.Warning(
                    "Packet handler returned null for {ClientIp} opcode 0x{Opcode:X4}; packet blocked",
                    session?.ClientIp ?? "Unknown",
                    packet.Opcode);
                return new PacketResult(data, null, PacketResultType.Block);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Packet handler failed for {ClientIp} opcode 0x{Opcode:X4}; packet blocked",
                    session?.ClientIp ?? "Unknown", packet.Opcode);
                return new PacketResult(data, null, PacketResultType.Block);
            }
        }
    }
}
