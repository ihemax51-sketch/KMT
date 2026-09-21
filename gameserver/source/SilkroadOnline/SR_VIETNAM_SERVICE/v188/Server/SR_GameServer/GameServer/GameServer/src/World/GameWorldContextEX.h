//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "GameWorldContext.h"
#include "GameWorld.h"

class GameWorldContextEX : public GameWorldContext {
public:
private:
    CGameWorld *m_pMyWorld; //0x0024
    class CGameWorldLayer *m_pMyWorldLayer; //0x0028
    char pad_002c[20]; //0x002C
    GameWorldContext m_pervWorldContext; //0x0040 // maybe its the pervious one ithink
    class CWorldContextController *m_pMyCtrl; //0x0064
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0068)
        ENSURE_OFFSET(m_pMyWorld, 0x0024)
        ENSURE_OFFSET(m_pMyWorldLayer, 0x0028)
        ENSURE_OFFSET(m_pervWorldContext, 0x0040)
        ENSURE_OFFSET(m_pMyCtrl, 0x0064)
    END_FIXTURE()

    RUN_FIXTURE(GameWorldContextEX)
};