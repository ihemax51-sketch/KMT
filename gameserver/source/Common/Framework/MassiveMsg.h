//
// Created by Kurama on 12/15/2022.
//
#pragma once

#include "Windows.h"
#include <vector>

#include "Test/Test.h"
#include "Msg.h"

#ifndef CLIENT_BUILD
namespace ServerFramework {
#endif // CLIENT_BUILD

    class CMassiveMsg {
    public:
        virtual ~CMassiveMsg();

    private:
        std::vector<CMsg *> m_vecMsgList; //0x0004
        int m_nCurrentReadMsg; //0x0014
        char pad_00018[5]; //0x0018
        bool m_bEncrypted; //0x001D
        WORD m_wMsgId; //0x001E
        char pad_0020[8]; //0x0020
        CMsg *m_pMsg; //0x0028
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x002C)
            ENSURE_OFFSET(m_vecMsgList, 0x0004)
            ENSURE_OFFSET(m_nCurrentReadMsg, 0x0014)
            ENSURE_OFFSET(m_bEncrypted, 0x001D)
            ENSURE_OFFSET(m_wMsgId, 0x001E)
            ENSURE_OFFSET(m_pMsg, 0x0028)
        END_FIXTURE()

        RUN_FIXTURE(CMassiveMsg)
    };

#ifndef CLIENT_BUILD
}
#endif // CLIENT_BUILD
