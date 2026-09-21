///******************************************************************************
/// \File CmdSrcNet.h
///
/// \Desc Handling the client msgs and haz the client context
///
///******************************************************************************

#pragma once

#include "CmdSource.h"

struct sClientContext {
public:
    DWORD m_dwJID; //0x0000
    char pad_0004[8]; //0x0004
    DWORD m_dwAgentSessionId; //0x000C
    DWORD m_dwClientSessionId; //0x0010
    BYTE m_btSecurityPrimaryGroup; //0x0014
    BYTE m_btSecurityContentGroup; //0x0015
    // Unknown size
private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(m_dwJID, 0x0000)
        ENSURE_OFFSET(m_dwAgentSessionId, 0x000C)
        ENSURE_OFFSET(m_dwClientSessionId, 0x0010)
        ENSURE_OFFSET(m_btSecurityPrimaryGroup, 0x0014)
        ENSURE_OFFSET(m_btSecurityContentGroup, 0x0015)
    END_FIXTURE()

    RUN_FIXTURE(sClientContext)
};

class CCmdSrcNet : public CCmdSource {
private:
    int m_nTickCount; //0x0024
public:
    sClientContext m_ClientContext; //0x0028
private:
    char pad_003C[276]; //0x003C
public:
    CMsg *NewMsg(WORD wMsgId) override;

    void FreeMsg(CMsg *pMsg) override;

    void Delete() override;

    void Func_9() override;

    void Func_10() override;

    void Func_11() override;

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0154)
    END_FIXTURE()

    RUN_FIXTURE(CCmdSrcNet)
};
