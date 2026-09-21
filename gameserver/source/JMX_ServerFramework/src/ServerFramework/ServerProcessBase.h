//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include "ServiceObject.h"
#include "MassiveMsg.h"
#include "OverlapJobMgr.h"

#include <set>

struct stQueueTimerData {
public:
    bool m_bParam; //0x0000
    DWORD m_dwMillSec; //0x0004
    DWORD m_dwPeriodTime; //0x0008
    void *pfunctionCallBack; //0x000C
};

#define MSG_HANDLE CMsg *pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, ServerFramework::CMassiveMsg *pMassiveMsg

namespace ServerFramework {
    class CServerProcessBase : public CServiceObject {
        typedef void (__thiscall ServerFramework::CServerProcessBase::*fnHandleMsg)(CMsg *pMsg,
                                                                                    DWORD dwOverlappedJobID,
                                                                                    LPVOID lpParam,
                                                                                    CMassiveMsg *pMassiveMsg);

        CServerProcessBase();

    public:
    public:
        virtual void RegisterMsgHandlers();

        virtual void SetMsgHnadler(fnHandleMsg *pfn);

        virtual void CallBackMsgHandler(MSG_HANDLE);

        virtual void OnProcessMessage(MSG_HANDLE);

        // it send packet 0x2007 thats call func 6 in CGameWorld in Shard per 5 secs so its maybe checks for the dead monster?
        virtual void Func_10();

        // well thats the func who call func 6 per 5 secs
        virtual void Func_11();

        virtual void Func_12();

        virtual void Func_13();

        virtual void Func_14();

        virtual void Func_15();

        virtual void Func_16();

        virtual void Func_17();

        virtual void Func_18();

        virtual void Func_19();

    private:
        fnHandleMsg *m_pfnMsHandle[0xFFFF]; //0x00004C
        HANDLE *m_phQueueTimer; //0x40048
        DWORD m_dwAbleTorRunQueueTimer; //0x4004C
        std::set<stQueueTimerData> m_setQueueFuncTimer; //0x40050
        DWORD m_dwAbleToPushTimer; //0x4005C
        COverlapJobManager m_overlapJobMgr; //0x4060
        class CTask *m_pProcessTask; //0x0008
        char pad_400b4[4]; //0x400B4 using to see if t0x6009 msg id got registed
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x400B8)
            ENSURE_OFFSET(m_pfnMsHandle, 0x0004C)
            ENSURE_OFFSET(m_phQueueTimer, 0x40048)
            ENSURE_OFFSET(m_dwAbleTorRunQueueTimer, 0x4004C)
            ENSURE_OFFSET(m_setQueueFuncTimer, 0x40050)
            ENSURE_OFFSET(m_dwAbleToPushTimer, 0x4005C)
            ENSURE_OFFSET(m_overlapJobMgr, 0x40060)
        END_FIXTURE()

        RUN_FIXTURE(CServerProcessBase)
    };
}