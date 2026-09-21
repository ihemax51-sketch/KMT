#include "CommandDispatchGuard.h"

#include <Windows.h>

namespace
{
    __declspec(thread) LONG s_broadcastCompleted = 0;
}

void CommandDispatchGuard::Reset()
{
    s_broadcastCompleted = 0;
}

void CommandDispatchGuard::MarkBroadcastCompleted()
{
    s_broadcastCompleted = 1;
}

bool CommandDispatchGuard::TryConsumeBroadcast()
{
    if (s_broadcastCompleted == 0)
        return false;
    s_broadcastCompleted = 0;
    return true;
}
