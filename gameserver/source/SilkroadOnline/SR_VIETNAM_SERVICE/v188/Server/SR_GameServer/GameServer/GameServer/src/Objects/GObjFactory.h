//
// Created by Kurama on 12/19/2022.
//
#pragma once

#include "STL/SingletonT.h"
#include "GObj.h"

#include <list>

#define FACTORY_OBJ_COUNT_ITEM 0
#define FACTORY_OBJ_COUNT_NONUSER 1
#define FACTORY_OBJ_COUNT_PC 2
#define FACTORY_OBJ_COUNT_STRUCT 3
#define FACTORY_OBJ_COUNT_SKILL 4

#define g_pObjFactory (*reinterpret_cast<CGObjFactory **>(0x00d6a960))

class CGObjFactory : public CSingletonT<CGObjFactory> {
public:
public:
    virtual ~CGObjFactory();

public:
private:
    char pad_0004[8]; //0x0004
    std::list<std::pair<DWORD /* Game ID */, CGObj *>> m_listGameObjects; //0x000C // Val 2 is NULL but Val 1 haz the GameID
    char pad_0018[24]; //0x0018
    int m_nObjCount[5]; //0x0030
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0044)
        ENSURE_OFFSET(m_listGameObjects, 0x000C)
        ENSURE_OFFSET(m_nObjCount, 0x0030)
    END_FIXTURE()

    RUN_FIXTURE(CGObjFactory)
};