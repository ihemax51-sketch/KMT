//
// Created by Kurama on 12/19/2022.
//
#pragma once

#include "Win32SizeRuler.h"
#include "ServerChildWindowBase.h"

#define BODY_INFO_MASK_TYPE_NETENGINE_VER 1
#define BODY_INFO_MASK_TYPE_MSG_USAGE 2
#define BODY_INFO_MASK_TYPE_SERVER_DETAILS 4

namespace ServerFramework {
    class CServerFrameWindow : public CWin32SizeRuler {
    public:
        virtual bool InitializeServerWindow();

        virtual void DeInitializeServerWindow();

        virtual LRESULT OnModuleInfoBtnClick(WPARAM, LPARAM);

        virtual LRESULT OnExitBtnClick(WPARAM, LPARAM);

        virtual LRESULT OnBodyInfoBtnClick(WPARAM, LPARAM);

        virtual LRESULT OnWindowNotify(WPARAM, LPARAM);

        virtual LRESULT OnEraseBKGND(WPARAM, LPARAM); // idont have aclue wtf is this, anyway its empty

        virtual LRESULT OnSystemCommand(WPARAM, LPARAM); // like Esc as example

        virtual LRESULT OnQuestReloadBtnClick(WPARAM, LPARAM); // hidden btn

        virtual void Func_20(class CMsg *);

    private:
        HWND m_hWndMenu; //0x008C
        HGDIOBJ m_hGIOBJ; //0x0090 idont have a clue what is this
        HANDLE m_hThreadHandle; //0x0094
        std::list<CServerChildWindowBase *> m_listChildWnd; //0x0098
        std::list<CServerChildWindowBase *> m_listMainViewWnd; //0x00A4
        HWND m_hWndMoniterView; //0x00B0
        HWND m_hWndCurrentView; //0x00B4 // main view hWnd
        HWND m_hWndMenu2; //0x00B8
        class CServerMoniterView *m_pServerDetailsView; //0x00BC ofcourse iwill not reverse it lol!
        class CServerArchitectureView *m_pServerModulesView; //0x00C0 ofcourse iwill not reverse it lol!
        int m_nBodyInfoTypeMask; //0x00C4
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x00C8)
            ENSURE_OFFSET(m_hWndMenu, 0x008C)
            ENSURE_OFFSET(m_hGIOBJ, 0x0090)
            ENSURE_OFFSET(m_hThreadHandle, 0x0094)
            ENSURE_OFFSET(m_listChildWnd, 0x0098)
            ENSURE_OFFSET(m_listMainViewWnd, 0x00A4)
            ENSURE_OFFSET(m_hWndMoniterView, 0x00B0)
            ENSURE_OFFSET(m_hWndCurrentView, 0x00b4)
            ENSURE_OFFSET(m_hWndMenu2, 0x00b8)
            ENSURE_OFFSET(m_pServerDetailsView, 0x00BC)
            ENSURE_OFFSET(m_pServerModulesView, 0x00C0)
            ENSURE_OFFSET(m_nBodyInfoTypeMask, 0x00C4)
        END_FIXTURE()

        RUN_FIXTURE(CServerFrameWindow)
    };
}