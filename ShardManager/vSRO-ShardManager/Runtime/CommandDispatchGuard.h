#pragma once

namespace CommandDispatchGuard
{
    void Reset();
    void MarkBroadcastCompleted();
    bool TryConsumeBroadcast();
}
