///******************************************************************************
/// \File CmdSource.h
///
/// \Desc Client Context base
///
///******************************************************************************

#pragma once

#include <Windows.h>

#include "STL/Que.h"

class CMsg;
class CGObjChar;

class CCmdSource {
protected:
    CGObjChar *m_pClient; //0x0004
    CQue<CMsg *> m_queMsg; //0x0008
public:
    virtual ~CCmdSource();

    virtual void EnqueueMsg(CMsg *pMsg);

    virtual void SetObject(CGObjChar *pObj);

    virtual void Func_3();

    virtual void DoWork();

    virtual CMsg *NewMsg(WORD wMsgId);

    virtual void FreeMsg(CMsg *pMsg);

    virtual void Func_7();

    virtual void Delete();

    virtual void Func_9();

    virtual void Func_10();

    virtual void Func_11();

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0024)
        ENSURE_OFFSET(m_pClient, 0x0004)
        ENSURE_OFFSET(m_queMsg, 0x0008)
    END_FIXTURE()

    RUN_FIXTURE(CCmdSource)
};