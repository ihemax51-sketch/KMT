#pragma once

#include <string>

namespace UniqueLogQueue
{
    bool Initialize(const std::wstring& connectionString);
    void Shutdown();
    bool EnqueueSpawn(unsigned long refObjectId);
    bool EnqueueKill(unsigned long refObjectId, const std::string& killerName);
}
