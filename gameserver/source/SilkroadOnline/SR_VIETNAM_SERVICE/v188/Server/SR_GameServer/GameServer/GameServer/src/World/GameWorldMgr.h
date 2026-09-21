///******************************************************************************
/// \File GameWorldMgr.h
///
/// \Desc
///
///******************************************************************************

#pragma once

#include <map>

#include "STL/SingletonT.h"

#include "GameWorld.h"

class CGameWorldMgr : public CSingletonT<CGameWorldMgr> {
public:
    virtual ~CGameWorldMgr();

public:
    void SendMsgToAllGameWorldsUser(CMsg *pMsg);

private:
    std::map<DWORD, CGameWorld *> m_mapOnlineGameWorlds; //0x0004
    // ... there is more
};

#define g_pGameWorldMgr (*((CGameWorldMgr**)0x00d6a9a4))
