//
// Created by Kurama on 12/20/2022.
//
#pragma once

#include "WindowBase.h"

namespace ServerFramework {
    class CServerChildWindowBase : public CWindowBase {
    public:
        virtual void OnDeInitialize() = 0;

        virtual const char *GetWindowName();

    private:
        std::string m_strWindowName; //0x0070
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x008C)
            ENSURE_OFFSET(m_strWindowName, 0x0070)
        END_FIXTURE()

        RUN_FIXTURE(CServerChildWindowBase)
    };
}