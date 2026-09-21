//
// Created by Kurama on 3/7/2023.
//
#pragma once

#include "Base.h"

#include "BSNet/NetEngine.h"
#include "Msg.h"
#include "SRO_MSG_DEF.h"

enum LOG_TYPE : int {
    LOG_TYPE_NOTIFY = 0,
    LOG_TYPE_WARNING = 0x1000000,
    LOG_TYPE_FATAL = 0x2000001
};

namespace ServerFramework {
    static void DispatchMsgToIO(CMsg *pMsg) {
#if defined(g_hIoCompletionPort) && defined(g_pNetEngine)
        if (PostQueuedCompletionStatus(g_hIoCompletionPort, sizeof(pMsg), /* Seassion Id? */1, (LPOVERLAPPED) pMsg) ==
            0)
            g_pNetEngine->FreeMsg(pMsg);
#endif // defined(g_hIoCompletionPort) && defined(g_pNetEngine)
    }

    static void __ReportLog(LOG_TYPE nType, const char *lpszMsg) {
#if defined(g_hIoCompletionPort) && defined(g_pNetEngine) && defined(g_hMainWindow)
        if (lpszMsg != NULL && g_pNetEngine != NULL &&
            g_hMainWindow != NULL && g_hIoCompletionPort != NULL) {
            CMsg *pLogMsg = g_pNetEngine->AllocNewMsg(false);
            if (pLogMsg == NULL)
                return;

            pLogMsg->SetMsgID(FRAMEWORKMSG_LOG);
            if (!pLogMsg->TryWrite<int>(static_cast<int>(nType)) ||
                !pLogMsg->TryWriteString(std::string(lpszMsg))) {
                g_pNetEngine->FreeMsg(pLogMsg);
                return;
            }

            DispatchMsgToIO(pLogMsg);
            return;
        }
#endif // defined(g_hIoCompletionPort) && defined(g_pNetEngine) && defined(g_hMainWindow)
    }

    static void ReportLog(LOG_TYPE nType, const char *lpszMsg, ...) {
        char buffer[8192] = {0};

        va_list args;
                va_start(args, lpszMsg);

        int count = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, lpszMsg, args);
        va_end(args);
        if (count > -1 && count < sizeof(buffer)) {
            if (count == sizeof(buffer) - 1)
                buffer[sizeof(buffer) - 1] = '\0';

            __ReportLog(nType, buffer);
            return;
        }

        buffer[sizeof(buffer) - 1] = '\0';
    }

    static CMsg *MakeRelayMsgToClient(WORD wMsgId, DWORD dwClientSessionId) {
#if defined(g_pNetEngine)
        CMsg *pMsg = g_pNetEngine->AllocNewMsg(false);
        pMsg->SetMsgID(FRAMEWORKMSG_RELAY_MSG_TO_CLIENT_SINGLE);
        *pMsg << dwClientSessionId << wMsgId;

        return pMsg;
#endif // defined(g_pNetEngine)
    }


}
