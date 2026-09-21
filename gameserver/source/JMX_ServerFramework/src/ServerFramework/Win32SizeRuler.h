//
// Created by Kurama on 12/20/2022.
//
#pragma once

#include "WindowBase.h"

namespace ServerFramework {
    class CWin32SizeRuler : public CWindowBase {
    public:
        virtual bool CreateMainFrameWnd(
                DWORD dwExStyle,
                LPCWSTR lpClassName,
                LPCWSTR lpWindowName,
                DWORD dwStyle,
                int X,
                int Y,
                int nWidth,
                int nHeight,
                HWND hWndParent,
                HMENU hMenu,
                HINSTANCE hInstance);

        virtual LRESULT OnDraw(WPARAM, LPARAM);

        virtual LRESULT OnSize(WPARAM, LPARAM);

        virtual LRESULT OnLeftMouseClickUp(WPARAM, LPARAM);

        virtual LRESULT OnLeftMouseClickDown(WPARAM, LPARAM);

        virtual LRESULT OnMouseMove(WPARAM, LPARAM);

        virtual LRESULT OnMouseCursorChange(WPARAM, LPARAM);

    private:
        char pad_0070[24]; //0x0070 paint stuff
        HCURSOR m_hCursor; //0x0088
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x008C) // im not usre if menu hwnd belong to this class
            ENSURE_OFFSET(m_hCursor, 0x0088)
        END_FIXTURE()

        RUN_FIXTURE(CWin32SizeRuler)
    };
}