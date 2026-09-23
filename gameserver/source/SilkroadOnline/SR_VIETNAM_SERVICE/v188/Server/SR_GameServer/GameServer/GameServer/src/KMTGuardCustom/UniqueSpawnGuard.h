#pragma once

#include <Windows.h>
#include <cstddef>

class CGObjPC;
struct SWorldID;
struct SPosInfo;

namespace UniqueSpawnGuard
{
    bool Initialize();
    void Shutdown();
    bool AuthorizeAndConsume(CGObjPC* player, BYTE uniqueType,
                             const SWorldID& world, const SPosInfo& position,
                             const BYTE* replayToken, size_t replayTokenLength);
    void ForgetPlayer(CGObjPC* player);
}
