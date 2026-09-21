#pragma once


#include "MsgStreamBuffer.h"
class CNetEngine;


//Custom, adding ability to manage shard manager network.
class CShardNetManager
{
private:
    typedef void* (__stdcall* FN_BROADCAST_MSG_TO_GAMESERVERS)(CMsgStreamBuffer* pMsg);
    typedef CMsgStreamBuffer* (__stdcall* FN_ALLOC_MSG_FOR_GS)(CNetEngine* pNetEngine, int a2);

    static FN_BROADCAST_MSG_TO_GAMESERVERS s_pfnBroadcastMsgToGameServers;
    static FN_ALLOC_MSG_FOR_GS s_pfnAllocMsgForGS;
public:
    static bool Setup();

    static void BroadcastMsgToGameServers(CMsgStreamBuffer* pMsg);
    static CMsgStreamBuffer* AllocMsgForGS();
};
