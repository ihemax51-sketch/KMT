#include "LogCustoms.h"
#include "Logger.h"
#include <fstream>
#include <SqlConnection/sqlCon.h>
#include <BSObj/BSObj.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include "Helpers.h"
#include "ServerFramework/ServerFramework.h"


#define FN_SERVERAPP_ON_LOG_MSG_OFFSET		0x00935BD0
CLogCustoms::FN_SERVERAPP_ON_LOG_MSG CLogCustoms::s_pfnServerApp_OnLogMsg;
CAutoCriticalSection CLogCustoms::s_criticalSection;
namespace
{
    bool s_logDetourInstalled = false;
    volatile LONG s_readyStampWritten = 0;
    char s_productVersion[32] = "unknown";

    void WriteNativeReadyStamp()
    {
        ServerFramework::ReportLog(
            LOG_TYPE_NOTIFY, "========== KMTGuard v%s ==========", s_productVersion);
        ServerFramework::ReportLog(
            LOG_TYPE_NOTIFY, "Protection, packet guards and telemetry are ACTIVE");
        ServerFramework::ReportLog(
            LOG_TYPE_NOTIFY, "GameServer startup completed successfully");
    }
}

bool CLogCustoms::Setup(const char* productVersion)
{
    if (productVersion == NULL || productVersion[0] == '\0')
        return false;
    lstrcpynA(s_productVersion, productVersion, sizeof(s_productVersion));

    s_pfnServerApp_OnLogMsg = reinterpret_cast<FN_SERVERAPP_ON_LOG_MSG>(FN_SERVERAPP_ON_LOG_MSG_OFFSET);

    s_logDetourInstalled = GameServerRuntimeSafety::AttachDetour(
        reinterpret_cast<PVOID*>(&s_pfnServerApp_OnLogMsg),
        reinterpret_cast<PVOID>(CLogCustoms::MyServerApp_OnLogMsg),
        "server log detour");
    return s_logDetourInstalled;
}

void CLogCustoms::Shutdown()
{
    if (s_logDetourInstalled)
    {
        GameServerRuntimeSafety::DetachDetour(
            reinterpret_cast<PVOID*>(&s_pfnServerApp_OnLogMsg),
            reinterpret_cast<PVOID>(CLogCustoms::MyServerApp_OnLogMsg),
            "server log detour");
        s_logDetourInstalled = false;
    }
}

void* __fastcall CLogCustoms::MyServerApp_OnLogMsg(void* pSelf, void* /* dummy edx*/, CMsgStreamBufferCustom* pMsg)
{
    bool gameServerReady = false;
    if (pMsg != NULL && pMsg->IsValid() && pMsg->GetID() == 0x200A)
    {
        const WORD nOldReadPos = pMsg->GetReadPos();
        const WORD nOldWritePos = pMsg->GetWritePos();
        try
        {
            pMsg->SetReadPos(6);
            pMsg->Read<int>();
            const std::string msg = pMsg->ReadStringA();

            gameServerReady = msg == "SR_GameServer is initialized successfully";
        }
        catch (...)
        {
            GameServerTelemetry::RecordMalformedPacket();
        }

        try
        {
            pMsg->SetWritePos(nOldWritePos);
            pMsg->SetReadPos(nOldReadPos);
        }
        catch (...)
        {
            GameServerTelemetry::RecordMalformedPacket();
        }
    }
    void* result = s_pfnServerApp_OnLogMsg != NULL
        ? s_pfnServerApp_OnLogMsg(pSelf, pMsg)
        : NULL;

    if (gameServerReady && InterlockedCompareExchange(&s_readyStampWritten, 1, 0) == 0)
    {
        s_criticalSection.Enter();
        try
        {
            WriteNativeReadyStamp();
            CSqlCon::GameServerInitialized();
        }
        catch (...)
        {
            GameServerTelemetry::RecordRuntimeError();
        }
        s_criticalSection.Leave();
    }

    return result;
}
