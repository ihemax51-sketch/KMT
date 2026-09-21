using SilkroadSecurityAPI;

namespace KMTGuard.SessionManager;

internal enum ClientPacketQueueResult
{
    Ready,
    Queued,
    Overflow
}

/// <summary>
/// Holds application packets outside SilkroadSecurityAPI until the downstream
/// client handshake is complete. Keeping them outside the security object's
/// outgoing queue prevents application packets from blocking handshake packets.
/// </summary>
internal sealed class ClientHandshakePacketQueue
{
    private const int MaximumPendingPackets = 512;
    private const int MaximumPendingBytes = 4 * 1024 * 1024;

    private readonly object _gate = new();
    private readonly Queue<Packet> _pending = new();
    private int _pendingBytes;
    private QueueState _state;

    public ClientPacketQueueResult QueueOrReady(Packet packet)
    {
        ArgumentNullException.ThrowIfNull(packet);

        lock (_gate)
        {
            if (_state == QueueState.Ready)
                return ClientPacketQueueResult.Ready;

            var packetBytes = packet.GetBytes().Length;
            if (_pending.Count >= MaximumPendingPackets ||
                packetBytes > MaximumPendingBytes - _pendingBytes)
            {
                return ClientPacketQueueResult.Overflow;
            }

            _pending.Enqueue(packet);
            _pendingBytes += packetBytes;
            return ClientPacketQueueResult.Queued;
        }
    }

    public bool BeginFlush()
    {
        lock (_gate)
        {
            if (_state != QueueState.WaitingForHandshake)
                return false;

            _state = QueueState.Flushing;
            return true;
        }
    }

    public Packet[] TakePendingBatchOrComplete()
    {
        lock (_gate)
        {
            if (_state != QueueState.Flushing)
                return Array.Empty<Packet>();

            if (_pending.Count == 0)
            {
                _state = QueueState.Ready;
                return Array.Empty<Packet>();
            }

            var batch = _pending.ToArray();
            _pending.Clear();
            _pendingBytes = 0;
            return batch;
        }
    }

    private enum QueueState
    {
        WaitingForHandshake,
        Flushing,
        Ready
    }
}
