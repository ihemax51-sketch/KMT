//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include "Que.h"

#include <list>

// TODO : in future we will need it, so better do the ctor and dector and this macro too, later
#define DECLARE_CQUE(t)

// Unknown name class
// but why i called it CQuePool?
// u see "MsgPool", "PassiveSocketPoo", "ActiveSocketPool", "NetEngine::SessionPool" so im sure the class name haz "Pool"
template<typename T>
class CQuePool {
public:
public:
    void *m_pfnEraseObjData; //0x0000 maybe its call per using the class?
    void *m_pfnAssertObjData; //0x0004 maybe its call per using the class?
    char pad_0008[4]; //0x0008
    CQue<T *> m_qMyObj; //0x000c
    std::list<T *> m_listMyObj; //0x0028 a dummy allocated object get push, just it
    char pad_0034[12]; //0x0034
    std::string m_strObjectName; //0x0040 im wondering for why iwill need it.
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x005c) // my foxy self is sure about this size
        ENSURE_OFFSET(m_pfnEraseObjData, 0x0000)
        ENSURE_OFFSET(m_pfnAssertObjData, 0x0004)
        ENSURE_OFFSET(m_qMyObj, 0x000C)
        ENSURE_OFFSET(m_listMyObj, 0x0028)
        ENSURE_OFFSET(m_strObjectName, 0x0040)
    END_FIXTURE()

    RUN_FIXTURE(CQuePool)
};