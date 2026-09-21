//
// Created by Kurama on 12/20/2022.
//
#pragma once

#include "Windows.h"
#include <list>

#include "Test/Test.h"

namespace ServerFramework {
    class CWindowBase {
    public:
        virtual ~CWindowBase();

    public:
        virtual LRESULT OnWndProc(UINT Msg, WPARAM wParam, LPARAM lParam);

    private:
        char pad_0004[16]; //0x0004
    public:
        HWND m_hWnd; //0x0014
    private:
        HINSTANCE m_hInstance; //0x0018
        bool m_bIgnoreWindowsMSGS; //0x001C WM original Windows msg
        char pad_0020[4]; //0x0020
        std::list<std::pair<DWORD, LRESULT (__thiscall ServerFramework::CWindowBase::*)(WPARAM,
                                                                                        LPARAM)>> m_listWindowMsgHandle; //0x0024
        char pad_0030[28]; //0x0030 vector???
        std::list<std::pair<DWORD, LRESULT (__thiscall ServerFramework::CWindowBase::*)(WPARAM,
                                                                                        LPARAM)>> m_listMenuCommandHandle; //0x004c
        char pad_0058[24];
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x0070)
            ENSURE_OFFSET(m_hWnd, 0x0014)
            ENSURE_OFFSET(m_hInstance, 0x0018)
            ENSURE_OFFSET(m_bIgnoreWindowsMSGS, 0x001c)
            ENSURE_OFFSET(m_listWindowMsgHandle, 0x0024)
            ENSURE_OFFSET(m_listMenuCommandHandle, 0x004c)
        END_FIXTURE()

        RUN_FIXTURE(CWindowBase)
    };
}