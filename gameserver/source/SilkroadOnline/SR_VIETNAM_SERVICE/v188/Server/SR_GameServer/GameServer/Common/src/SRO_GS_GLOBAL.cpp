///******************************************************************************
/// \File SRO_GS_GLOBAL.cpp
///
/// \Desc
///
/// \Author kyuubi09 on 6/9/2023.
///
/// \Copyright Unpublished Work © 2023 Kyuubi.
///
///******************************************************************************

#include "SRO_GS_GLOBAL.h"
#include "Helpers.h"

#include "BSNet/NetEngine.h"

CMsg *AllocMsgToBroadcast(WORD wMsgId) {
    CMsg *pMsg = g_pNetEngine->AllocNewMsg(false);
    pMsg->SetMsgID(wMsgId);

    return pMsg;
}
