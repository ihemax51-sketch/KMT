//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "Data.h"

#include "Test/Test.h"

#include "NavMesh/RTNavCellQuad.h"

class GameWorldContext {
public:
    virtual ~GameWorldContext();

public:
    SWorldID m_sWorldId; //0x0004
    CRTNavCellQuad *m_pCellQuad; //0x0008
    char pad_000c[4]; //0x000c
    SPosInfo m_sPosData; //0x0010
    char pad_0020[4]; //0x0020
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0024) // stil not sure about this size! BUT MY XOY SELF TELLING ME THAT  so dont care about it
        ENSURE_OFFSET(m_sWorldId, 0x0004)
        ENSURE_OFFSET(m_pCellQuad, 0x0008)
        ENSURE_OFFSET(m_sPosData, 0x0010)
    END_FIXTURE()

    RUN_FIXTURE(GameWorldContext)
};