#include <Rpc.h>
#include "ShardNetManager.h"
#include "../Runtime/CommandDispatchGuard.h"
#include "../Utils/BSObj.h"
#include <stdexcept>
//#include "ClientManager.h"

CShardNetManager::FN_BROADCAST_MSG_TO_GAMESERVERS CShardNetManager::s_pfnBroadcastMsgToGameServers;
CShardNetManager::FN_ALLOC_MSG_FOR_GS CShardNetManager::s_pfnAllocMsgForGS;

class CNetEngine;

#define NET_ENGINE_FOR_GS_REF_OFFSET				0x00858868
#define FN_BROADCAST_MSG_TO_GAMESERVERS_OFFSET		0x004121E0
//CNetEngine VFT + 0x48
#define FN_ALLOC_MSG_FOR_GS_OFFSET					0x006B8260

#define g_pNetEngineForGS (*(CNetEngine**)NET_ENGINE_FOR_GS_REF_OFFSET)

namespace
{
    bool IsCommittedAddress(const void* address)
    {
        MEMORY_BASIC_INFORMATION info;
        if (VirtualQuery(address, &info, sizeof(info)) != sizeof(info))
            return false;
        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
            return false;
        return true;
    }
}

bool CShardNetManager::Setup()
{
    if (!IsCommittedAddress(reinterpret_cast<const void*>(NET_ENGINE_FOR_GS_REF_OFFSET)) ||
        !IsCommittedAddress(reinterpret_cast<const void*>(FN_BROADCAST_MSG_TO_GAMESERVERS_OFFSET)) ||
        !IsCommittedAddress(reinterpret_cast<const void*>(FN_ALLOC_MSG_FOR_GS_OFFSET)))
    {
        BS_ERROR("Unsupported ShardManager network layout");
        return false;
    }

    s_pfnBroadcastMsgToGameServers = reinterpret_cast<FN_BROADCAST_MSG_TO_GAMESERVERS>(
        FN_BROADCAST_MSG_TO_GAMESERVERS_OFFSET
        );

    s_pfnAllocMsgForGS = reinterpret_cast<FN_ALLOC_MSG_FOR_GS>(
        FN_ALLOC_MSG_FOR_GS_OFFSET
        );



    //CreateThread(NULL, 0, MyTestWorker2, NULL, 0, NULL);

#ifdef __SHARDNET_DEMO__
    CreateThread(NULL, 0, MyTestWorkerThread, NULL, 0, NULL);
#endif


    return true;
}

void CShardNetManager::BroadcastMsgToGameServers(CMsgStreamBuffer* pMsg)
{
    if (pMsg == NULL || s_pfnBroadcastMsgToGameServers == NULL)
        throw std::runtime_error("Invalid ShardManager broadcast request");
    s_pfnBroadcastMsgToGameServers(pMsg);
    CommandDispatchGuard::MarkBroadcastCompleted();
}

CMsgStreamBuffer* CShardNetManager::AllocMsgForGS()
{
    if (s_pfnAllocMsgForGS == NULL || g_pNetEngineForGS == NULL)
        throw std::runtime_error("GameServer network engine is unavailable");
    CMsgStreamBuffer* message = s_pfnAllocMsgForGS(g_pNetEngineForGS, 0);
    if (message == NULL)
        throw std::bad_alloc();
    return message;
}
