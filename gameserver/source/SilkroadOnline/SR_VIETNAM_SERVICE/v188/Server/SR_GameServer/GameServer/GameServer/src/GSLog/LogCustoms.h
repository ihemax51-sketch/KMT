#pragma once

#include <Rpc.h>
#include <string>
#include <SqlConnection/AutoCriticalSection.h>
#include "MsgCustom.h"


class CLogCustoms
{
private:
    typedef void* (__thiscall* FN_SERVERAPP_ON_LOG_MSG)(void* pSelf, CMsgStreamBufferCustom* pMsg);
    static FN_SERVERAPP_ON_LOG_MSG s_pfnServerApp_OnLogMsg;
    static CAutoCriticalSection s_criticalSection;

    static void* __fastcall MyServerApp_OnLogMsg(void* pSelf, void* /* dummy edx*/, CMsgStreamBufferCustom* pMsg);

public:
    static bool Setup(const char* productVersion);
    static void Shutdown();
};
