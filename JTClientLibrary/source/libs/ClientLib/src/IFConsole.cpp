#include "IFConsole.h"

#include "ICPlayer.h"

#include "Game.h"

BOOL CIFConsole::IsConsolePermissioned() {
    if (g_pMyPlayerObj != NULL && g_pMyPlayerObj->IsGameMaster())
        return TRUE;

    return (Fun_GetCfgGame()->m_bDebugMode);
}

void CIFConsole::SetVisibleMode(DWORD dwMode) {
    // Do we have the permission to toggle it?
    if (IsConsolePermissioned() == FALSE)
        return;

    // If it trying to close or trying to open by checking if Shift was pressed.
    // Replace "(GetKeyState(VK_SHIFT) & SHIFTED)" with checking "dwMode" == "VISIBLE_MODE_OPENING"
    // if u planing to change the HotKey.
    if ((dwMode == VISIBLE_MODE_INVISIBLE) || (GetKeyState(VK_SHIFT) & SHIFTED)) {
        switch (m_CurVisibleMode) {
            // if we are already closed
            case VISIBLE_MODE_INVISIBLE:
                if (IsVisible())
                    break;
                // let's show it
                ShowGWnd(true);
            // if we are in closing
            case VISIBLE_MODE_CLOSING:
                // open it back
                m_CurVisibleMode = VISIBLE_MODE_OPENING;
                break;
            // if we are opening or already opened
            case VISIBLE_MODE_OPENING:
            case VISIBLE_MODE_VISIBLE:
                // close it back
                m_CurVisibleMode = VISIBLE_MODE_CLOSING;
                break;
        }
    }
}

BOOL CIFConsole::SetFocusOnInputBox() {
    if (!IsVisible())
        return FALSE;

    m_pInputBox->SetFocus_MAYBE();
    return TRUE;
}