///******************************************************************************
/// \File GameWorldMgr.cpp
///
/// \Desc
///
///******************************************************************************

#include "GameWorldMgr.h"

#include "BSNet/NetEngine.h"

#include "Helpers.h"

void CGameWorldMgr::SendMsgToAllGameWorldsUser(CMsg *pMsg) {
    std::map<DWORD, CGameWorld *>::const_iterator it = m_mapOnlineGameWorlds.begin();
    for (;; it++) {
        if (it == m_mapOnlineGameWorlds.end()) {
            pMsg->FlushRemainingBytes();
            g_pNetEngine->FreeMsg(pMsg);

            return;
        }

        if ((*it).second == NULL)
            continue;

        (*it).second->SendMsgToAllGameWorldUser(pMsg);
    }
}
