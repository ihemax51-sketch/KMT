//
// Created by Kurama on 12/14/2022.
//

#include "MainProcess.h"

#include "Game.h"
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <Objects/GObjMob.h>

void CMainProcess::_OnProcessMessage(MSG_HANDLE) {
    if (!GameServerRuntimeSafety::IsInitializationReady())
        return;
    GameServerTelemetry::ScopedGameLoopTimer gameLoopTimer;
    if (g_pCGame == NULL || pMsg == NULL)
        return;

    g_pCGame->ProcessMessage(pMsg);
}

void CMainProcess::_OnQueueTimer(MSG_HANDLE)
{
    GameServerTelemetry::ScopedQueueTick telemetryTick;
    try
    {
        CGObjMob::FlushLiveDpsBatch();
        CGObjPC::FlushItemLockCompletions();
    }
    catch (...)
    {
        GameServerTelemetry::RecordRuntimeError();
    }
    reinterpret_cast<void(__thiscall*)(
        CMainProcess*,
        CMsg*,
        DWORD,
        LPVOID,
        ServerFramework::CMassiveMsg*)>(0x0094CE60)(
            this,
            pMsg,
            dwOverlappedJobID,
            lpParam,
            pMassiveMsg);
}
