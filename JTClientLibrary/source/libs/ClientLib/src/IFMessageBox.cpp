#include "IFMessageBox.h"
#include <imgui/imgui.h>
#include <IFSystemMessage.h>
#include <GInterface.h>
#include <ICPlayer.h>
#include <IFStatic.h>
#include <ClientNet/MsgStreamBuffer.h>
#include <GlobalHelpersThatHaveNoHomeYet.h>

#include "Game.h"
#include "CharacterDependentData.h"
#include <TextStringManager.h>
#include <CustomInterface/IFMovePartyMember.h>
#include <CustomData/CustomCICPlayer.h>
#include <CustomInterface/IFSavedLocation.h>
#define GDR_MBIN_BTN_YES 200
#define GDR_MBIN_BTN_CANCEL 201
#define GDR_DISCONNECT_RESTART 9002
#define GDR_DISCONNECT_AUTORESTART_CHECK 9003
#define GDR_DISCONNECT_AUTORESTART_LABEL 9004
#define GDR_DISCONNECT_AUTORESTART_MARK 9005
#define GDR_DISCONNECT_AUTORESTART_BOX 9010
#define GDR_STALL_PRICE_TYPE_TOGGLE 9006
#define GDR_STALL_PRICE_TYPE_BOX 9007
#define GDR_STALL_PRICE_TYPE_MARK 9008
#define GDR_STALL_PRICE_TYPE_LABEL 9009
#define REVERSE_MOVETOPARTY 8000
#define REVERSE_MOVETOLOCATION 8001

bool g_bCurrentStallUsesSilk = false;
bool g_bCurrentMessageBoxIsStallName = false;
CIFMessageBox* g_pCurrentStallNameMessageBox = 0;
CIFButton* g_pStallPriceTypeToggleButton = 0;
CIFStatic* g_pStallPriceTypeBox = 0;
CIFStatic* g_pStallPriceTypeMark = 0;
CIFStatic* g_pStallPriceTypeLabel = 0;
CIFMessageBox* g_pDisconnectRestartMessageBox = 0;
CIFButton* g_pDisconnectAutoRestartButton = 0;
CIFStatic* g_pDisconnectAutoRestartBox = 0;
CIFStatic* g_pDisconnectAutoRestartMark = 0;
bool g_bDisconnectAutoRestart = false;

namespace {
    const wchar_t* GetCommandLineArgsOnly()
    {
        const wchar_t* commandLine = GetCommandLineW();
        if (commandLine == 0)
            return L"";

        if (*commandLine == L'"')
        {
            ++commandLine;
            while (*commandLine != L'\0' && *commandLine != L'"')
                ++commandLine;
            if (*commandLine == L'"')
                ++commandLine;
        }
        else
        {
            while (*commandLine != L'\0' && *commandLine != L' ' && *commandLine != L'\t')
                ++commandLine;
        }

        while (*commandLine == L' ' || *commandLine == L'\t')
            ++commandLine;

        return commandLine;
    }

    void RestartClientAndExit()
    {
        wchar_t target[MAX_PATH];
        if (GetModuleFileNameW(NULL, target, MAX_PATH) == 0)
        {
            MessageBoxW(NULL, KmtGetText(L"UIIT_KMT_COULD_NOT_FIND_THE_CURRENT_CLIENT_EXECUTABLE"),
        KmtGetText(L"UIIT_KMT_RESTART_FAILED"), MB_OK | MB_ICONERROR);
            ExitProcess(0);
        }

        wchar_t workDir[MAX_PATH];
        wcscpy_s(workDir, MAX_PATH, target);
        size_t length = wcslen(workDir);
        while (length > 0 && workDir[length - 1] != L'\\' && workDir[length - 1] != L'/')
            --length;

        if (length > 0)
        {
            workDir[length] = L'\0';
        }
        else
        {
            workDir[0] = L'\0';
        }

        const wchar_t* args = GetCommandLineArgsOnly();
        wchar_t commandLine[MAX_PATH * 3];

        if (args != 0 && args[0] != L'\0')
        {
            swprintf_s(commandLine, MAX_PATH * 3,
                       L"cmd.exe /C timeout /t 2 /nobreak >nul & start \"\" \"%s\" %s",
                       target,
                       args);
        }
        else
        {
            swprintf_s(commandLine, MAX_PATH * 3,
                       L"cmd.exe /C timeout /t 2 /nobreak >nul & start \"\" \"%s\"",
                       target);
        }

        STARTUPINFOW startupInfo;
        PROCESS_INFORMATION processInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        ZeroMemory(&processInfo, sizeof(processInfo));
        startupInfo.cb = sizeof(startupInfo);

        if (CreateProcessW(NULL, commandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, workDir[0] ? workDir : NULL, &startupInfo, &processInfo))
        {
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
        }
        else
        {
            MessageBoxW(NULL, KmtGetText(L"UIIT_KMT_COULD_NOT_START_THE_GAME_AGAIN"),
        KmtGetText(L"UIIT_KMT_RESTART_FAILED"), MB_OK | MB_ICONERROR);
        }

        ExitProcess(0);
    }

    void SendCurrentStallPriceType()
    {
        CMsgStreamBuffer buf(0x186D);
        buf << static_cast<BYTE>(g_bCurrentStallUsesSilk ? 1 : 0);
        SendMsg(buf);
    }

}

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFMessageBox, 0x00EE95A0)

GFX_MSGMAP* CIFMessageBox::MessageMap()
{
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
    {
        {GFX_WM_COMMAND, 0, REVERSE_MOVETOPARTY, REVERSE_MOVETOPARTY, BSSig_u12, 0,
            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFMessageBox::FUN_BTNPARTY))},

        {GFX_WM_COMMAND, 0, REVERSE_MOVETOLOCATION, REVERSE_MOVETOLOCATION, BSSig_u12, 0,
            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFMessageBox::FUN_BTNLOCATION))},

        {GFX_WM_COMMAND, 0, GDR_STALL_PRICE_TYPE_TOGGLE, GDR_STALL_PRICE_TYPE_TOGGLE, BSSig_u12, 0,
            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFMessageBox::OnChooseSilkStall))},

        {GFX_WM_COMMAND, 0, GDR_DISCONNECT_RESTART, GDR_DISCONNECT_RESTART, BSSig_u12, 0,
            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFMessageBox::OnClickRestart))},

        {GFX_WM_COMMAND, 0, GDR_DISCONNECT_AUTORESTART_CHECK, GDR_DISCONNECT_AUTORESTART_CHECK, BSSig_u12, 0,
            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFMessageBox::OnClickAutoRestart))},
    };

    static GFX_MSGMAP newmap =
    {
        reinterpret_cast<const GFX_MSGMAP *>(0x00d9b174),
        skillBoardMessageEntries,
    };

    return &newmap;
}

void CIFMessageBox::MsgBoxStore()
{
    reinterpret_cast<void (__thiscall *)(CIFMessageBox *)>(0x0063e700)(this);

    CIFStatic* priceTypeLabel = m_IRM.GetResObj<CIFStatic>(7, 1);
    if (priceTypeLabel != 0)
    {
        priceTypeLabel->SetText(g_bCurrentStallUsesSilk ? KmtGetText(L"UIIT_KMT_SILK") : KmtGetText(L"UIIT_KMT_GOLD"));
    }
}

void CIFMessageBox::MsgBoxStoreMoney()
{
    reinterpret_cast<void (__thiscall *)(CIFMessageBox *)>(0x0063e940)(this);

    CIFStatic* priceTypeLabel = m_IRM.GetResObj<CIFStatic>(7, 1);
    if (priceTypeLabel != 0)
    {
        priceTypeLabel->SetText(g_bCurrentStallUsesSilk ? KmtGetText(L"UIIT_KMT_SILK") : KmtGetText(L"UIIT_KMT_GOLD"));
    }
}

void CIFMessageBox::RefreshStallPriceTypeToggle()
{
    const int dialogX = GetPos().x;
    const int dialogY = GetPos().y;
    const int checkSize = 16;
    const int labelWidth = 42;
    const int checkLabelGap = -6;
    const int checkButtonGap = 16;
    const int buttonWidth = 76;
    const int buttonHeight = 24;
    const int buttonGap = 8;
    const int buttonGroupWidth = (buttonWidth * 2) + buttonGap;
    const int checkX = dialogX + 10;

    int buttonStartX = dialogX + ((GetSize().width - buttonGroupWidth) / 2);
    const int minButtonStartX = checkX + checkSize + checkLabelGap + labelWidth + checkButtonGap;
    if (buttonStartX < minButtonStartX)
        buttonStartX = minButtonStartX;

    CIFButton* silkCheckBox = g_pStallPriceTypeToggleButton;
    if (silkCheckBox == 0)
        silkCheckBox = m_IRM.GetResObj<CIFButton>(GDR_STALL_PRICE_TYPE_TOGGLE, 1);

    bool createdSilkCheckBox = false;
    if (silkCheckBox == 0)
    {
        RECT checkRect = {checkX, dialogY + 99, checkSize + checkLabelGap + labelWidth, buttonHeight};
        silkCheckBox = (CIFButton*)CGWnd::CreateInstance(
            this,
            GFX_RUNTIME_CLASS(CIFButton),
            checkRect,
            GDR_STALL_PRICE_TYPE_TOGGLE,
            0);
        createdSilkCheckBox = silkCheckBox != 0;
    }

    if (silkCheckBox == 0)
        return;

    g_pStallPriceTypeToggleButton = silkCheckBox;

    CIFButton* confirmButton = m_IRM.GetResObj<CIFButton>(GDR_MBIN_BTN_YES, 1);
    CIFButton* cancelButton = m_IRM.GetResObj<CIFButton>(GDR_MBIN_BTN_CANCEL, 1);
    CIFStatic* silkBox = g_pStallPriceTypeBox;
    CIFStatic* silkMark = g_pStallPriceTypeMark;
    CIFStatic* silkLabel = g_pStallPriceTypeLabel;

    const int buttonY = dialogY + 99;
    const int checkY = buttonY + 7;
    const int labelY = buttonY + 6;

    if (silkBox == 0)
    {
        RECT boxRect = {checkX, checkY, checkSize, checkSize};
        silkBox = (CIFStatic*)CGWnd::CreateInstance(
            this,
            GFX_RUNTIME_CLASS(CIFStatic),
            boxRect,
            GDR_STALL_PRICE_TYPE_BOX,
            0);
    }

    if (silkBox != 0)
    {
        g_pStallPriceTypeBox = silkBox;
        silkBox->MoveGWnd(checkX, checkY);
        silkBox->SetGWndSize(checkSize, checkSize);
        silkBox->TB_Func_13("interface\\ifcommon\\com_checkbutton_off.ddj", 0, 0);
        silkBox->SetClickable(false);
        silkBox->ShowGWnd(true);
        silkBox->BringToFront();
    }

    if (silkMark == 0)
    {
        RECT markRect = {checkX, labelY, checkSize, checkSize};
        silkMark = (CIFStatic*)CGWnd::CreateInstance(
            this,
            GFX_RUNTIME_CLASS(CIFStatic),
            markRect,
            GDR_STALL_PRICE_TYPE_MARK,
            0);
    }

    if (silkMark != 0)
    {
        g_pStallPriceTypeMark = silkMark;
        silkMark->MoveGWnd(checkX, labelY);
        silkMark->SetGWndSize(checkSize, checkSize);
        silkMark->SetText(L"X");
        silkMark->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
        silkMark->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        silkMark->SetClickable(false);
        silkMark->ShowGWnd(g_bCurrentStallUsesSilk);
        silkMark->BringToFront();
    }

    if (silkLabel == 0)
    {
        RECT labelRect = {checkX + checkSize + checkLabelGap, labelY, labelWidth, 16};
        silkLabel = (CIFStatic*)CGWnd::CreateInstance(
            this,
            GFX_RUNTIME_CLASS(CIFStatic),
            labelRect,
            GDR_STALL_PRICE_TYPE_LABEL,
            0);
    }

    if (silkLabel != 0)
    {
        g_pStallPriceTypeLabel = silkLabel;
        silkLabel->MoveGWnd(checkX + checkSize + checkLabelGap, labelY);
        silkLabel->SetGWndSize(labelWidth, 16);
        silkLabel->SetText(KmtGetText(L"UIIT_KMT_SILK"));
        silkLabel->SetClickable(false);
        silkLabel->ShowGWnd(true);
        silkLabel->BringToFront();
    }

    if (confirmButton != 0)
    {
        confirmButton->MoveGWnd(buttonStartX, buttonY);
        confirmButton->SetGWndSize(buttonWidth, buttonHeight);
        confirmButton->ShowGWnd(true);
        confirmButton->BringToFront();
    }

    if (cancelButton != 0)
    {
        cancelButton->MoveGWnd(buttonStartX + buttonWidth + buttonGap, buttonY);
        cancelButton->SetGWndSize(buttonWidth, buttonHeight);
        cancelButton->SetEnabledState(true);
        cancelButton->SetClickable(true);
        cancelButton->ShowGWnd(true);
        cancelButton->BringToFront();
    }

    silkCheckBox->MoveGWnd(checkX, buttonY);
    silkCheckBox->SetGWndSize(checkSize + checkLabelGap + labelWidth, buttonHeight);
    if (createdSilkCheckBox)
    {
        silkCheckBox->TB_Func_13("", 0, 0);
        silkCheckBox->FUN_00656590(std::n_string(""));
        silkCheckBox->FUN_00656640(std::n_string(""));
    }
    silkCheckBox->SetText(L"");
    silkCheckBox->SetEnabledState(true);
    silkCheckBox->SetClickable(true);
    silkCheckBox->ShowGWnd(true);
    silkCheckBox->BringToFront();
}

void CIFMessageBox::SetEditMsgBoxHandler(char Id)
{
    reinterpret_cast<void (__thiscall *)(CIFMessageBox *, char)>(0x00641f40)(this, Id);

    g_bCurrentMessageBoxIsStallName = Id == 0;
    g_pCurrentStallNameMessageBox = g_bCurrentMessageBoxIsStallName ? this : 0;
    g_pStallPriceTypeToggleButton = 0;
    g_pStallPriceTypeBox = 0;
    g_pStallPriceTypeMark = 0;
    g_pStallPriceTypeLabel = 0;
    g_bCurrentStallUsesSilk = false;

    if (!g_bCurrentMessageBoxIsStallName)
        return;

    RefreshStallPriceTypeToggle();
}

int CIFMessageBox::OnClickConfirm(const char *a2)
{
    if (g_pDisconnectRestartMessageBox == this)
    {
        if (g_bDisconnectAutoRestart)
            RestartClientAndExit();
    }

    if (g_bCurrentMessageBoxIsStallName && g_pCurrentStallNameMessageBox == this)
    {
        CMsgStreamBuffer buf(0x186D);
        buf << static_cast<BYTE>(g_bCurrentStallUsesSilk ? 1 : 0);
        SendMsg(buf);
    }

    return reinterpret_cast<int (__thiscall *)(CIFMessageBox *, const char *)>(0x00641DB0)(this, a2);
}

void CIFMessageBox::OnChooseGoldStall()
{
    g_bCurrentStallUsesSilk = false;
}

void CIFMessageBox::OnChooseSilkStall()
{
    if (!g_bCurrentMessageBoxIsStallName || g_pCurrentStallNameMessageBox != this)
    {
        this->Close();
        return;
    }

    g_bCurrentStallUsesSilk = !g_bCurrentStallUsesSilk;

    RefreshStallPriceTypeToggle();
}

void CIFMessageBox::OnClickRestart()
{
    if (g_pDisconnectRestartMessageBox == this)
    {
        OnClickAutoRestart();
        return;
    }

    OnChooseSilkStall();
}

void CIFMessageBox::OnClickAutoRestart()
{
    if (g_pDisconnectRestartMessageBox != this)
        return;

    g_bDisconnectAutoRestart = !g_bDisconnectAutoRestart;

    CIFButton* autoRestartCheckBox = g_pDisconnectAutoRestartButton;
    if (autoRestartCheckBox == 0)
        autoRestartCheckBox = this->m_IRM.GetResObj<CIFButton>(GDR_DISCONNECT_AUTORESTART_CHECK, 1);

    if (autoRestartCheckBox != 0)
    {
        autoRestartCheckBox->SetText(L"");
        autoRestartCheckBox->BringToFront();
    }

    CIFStatic* autoRestartBox = g_pDisconnectAutoRestartBox;
    if (autoRestartBox == 0)
        autoRestartBox = this->m_IRM.GetResObj<CIFStatic>(GDR_DISCONNECT_AUTORESTART_BOX, 1);

    if (autoRestartBox != 0)
    {
        autoRestartBox->ShowGWnd(true);
        autoRestartBox->BringToFront();
    }

    CIFStatic* autoRestartMark = g_pDisconnectAutoRestartMark;
    if (autoRestartMark == 0)
        autoRestartMark = this->m_IRM.GetResObj<CIFStatic>(GDR_DISCONNECT_AUTORESTART_MARK, 1);

    if (autoRestartMark != 0)
    {
        autoRestartMark->SetText(L"X");
        autoRestartMark->ShowGWnd(g_bDisconnectAutoRestart);
        autoRestartMark->BringToFront();
    }

    if (autoRestartCheckBox != 0)
        autoRestartCheckBox->BringToFront();

    if (g_bDisconnectAutoRestart)
        RestartClientAndExit();
}

void CIFMessageBox::Close()
{
    reinterpret_cast<void*(__thiscall *)(CIFMessageBox *)>(0x0063b8b0)(this);
}

void CIFMessageBox::Test()
{
    reinterpret_cast<void*(__thiscall *)(CIFMessageBox *)>(0x0063c320)(this);
}

void CIFMessageBox::OpenItemMall()
{
    reinterpret_cast<void*(__thiscall *)(CIFMessageBox *)>(0x0063b6e0)(this);
}

void CIFMessageBox::ReverseMap()
{
    reinterpret_cast<void*(__thiscall *)(CIFMessageBox *)>(0x0063b9a0)(this);
}

void CIFMessageBox::DeadPoint()
{
    reinterpret_cast<void*(__thiscall *)(CIFMessageBox *)>(0x00641eb0)(this);
}

void CIFMessageBox::OnUpdateIMPL()
{
    switch (m_nMessageBoxStyleType)
    {
        case 15:
            if (GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFItemMall)))
                return;
            break;
    }

    reinterpret_cast<void (__thiscall *)(CIFMessageBox *)>(0x0063d340)(this);
}

void CIFMessageBox::SetMessageBoxStyle(int Id)
{
    reinterpret_cast<void(__thiscall *)(CIFMessageBox *, int)>(0x0063EA80)(this, Id);
}

void CIFMessageBox::CreateMessageBox(int Id)
{
    reinterpret_cast<void(__thiscall *)(CIFMessageBox *, int)>(0x00643C20)(this, Id);
}

void CIFMessageBox::SetMessageBoxParent(CIFWnd *pWnd)
{
    reinterpret_cast<void(__thiscall *)(CIFMessageBox *, CIFWnd *)>(0x0063B5B0)(this, pWnd);
}

bool CIFMessageBox::OnCreateIMPL(long ln)
{
    bool a = reinterpret_cast<bool (__thiscall *)(const CIFMessageBox *, long)>(0x0063e380)(this, ln);

    CIFButton* reversePartyButton = this->m_IRM.GetResObj<CIFButton>(REVERSE_MOVETOPARTY, 1);
    CIFButton* reverseLocationButton = this->m_IRM.GetResObj<CIFButton>(REVERSE_MOVETOLOCATION, 1);

    if (reversePartyButton != 0)
    {
        reversePartyButton->SetText(KmtGetText(L"UIIT_KMT_MOVE_TO_PARTY_MEMBER"));
        reversePartyButton->ShowGWnd(false);
    }

    if (reverseLocationButton != 0)
    {
        reverseLocationButton->SetText(KmtGetText(L"UIIT_KMT_MOVE_TO_THE_USER_S_SET_LOCATION"));
        reverseLocationButton->ShowGWnd(false);
    }

    return a;
}
void CIFMessageBox::SetMsgBoxHandler(int Id, int a3)
{
    g_bCurrentMessageBoxIsStallName = false;
    g_pCurrentStallNameMessageBox = 0;
    g_pStallPriceTypeToggleButton = 0;
    g_pStallPriceTypeBox = 0;
    g_pStallPriceTypeMark = 0;
    g_pStallPriceTypeLabel = 0;

    if (g_pDisconnectRestartMessageBox == this)
        g_pDisconnectRestartMessageBox = 0;
    g_pDisconnectAutoRestartButton = 0;
    g_pDisconnectAutoRestartBox = 0;
    g_pDisconnectAutoRestartMark = 0;

    // Log the message box creation to a file in the game client directory
    FILE* logFile = fopen("KMTGuardKit_MsgBox.log", "a");
    if (logFile) {
        fprintf(logFile, "SetMsgBoxHandler: Id = %d, a3 = %d\n", Id, a3);
        fclose(logFile);
    }

    if (Id == 33)
    {
        reinterpret_cast<void(__thiscall*)(CIFMessageBox*, int, int)>(0x00644c90)(this, Id, a3);

        CIFMainPopup* popup = g_pCGInterface->GetMainPopup();
        if (popup == 0)
            return;

        CIFInventory* inventory = popup->GetInventory();
        if (inventory == 0)
            return;

        CSOItem* item = inventory->GetItemBySlot(a3);
        if (item == 0)
            return;

        if (item->m_blValid == 0)
            return;

        if (item->m_refObjItemId == 3795)
        {
            CIFMovePartyMember* movePartyWnd = g_pCGInterface->m_IRM.GetResObj<CIFMovePartyMember>(MovePartyMember, 1);
            CIFSavedLocation* savedLocationWnd = g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1);
            CIFButton* reversePartyButton = this->m_IRM.GetResObj<CIFButton>(REVERSE_MOVETOPARTY, 1);
            CIFButton* reverseLocationButton = this->m_IRM.GetResObj<CIFButton>(REVERSE_MOVETOLOCATION, 1);
            CIFButton* cancelButton = this->m_IRM.GetResObj<CIFButton>(GDR_MBIN_BTN_CANCEL, 1);

            if (movePartyWnd == 0 || savedLocationWnd == 0 ||
                reversePartyButton == 0 || reverseLocationButton == 0 || cancelButton == 0)
                return;

            if (movePartyWnd->IsVisible())
            {
                m_Player->ReverseSlot = 9999;
                movePartyWnd->ShowGWnd(false);
                CGEffSoundBody::get()->PlaySound(L"snd_window_close");
            }

            if (savedLocationWnd->IsVisible())
            {
                m_Player->ReverseSlot = 9999;
                savedLocationWnd->ShowGWnd(false);
                CGEffSoundBody::get()->PlaySound(L"snd_window_close");
            }

            m_Player->ReverseSlot = a3;
            this->SetGWndSize(300, 280);

            reverseLocationButton->MoveGWnd(reversePartyButton->GetPos().x, reversePartyButton->GetPos().y + 31);

            reversePartyButton->ShowGWnd(true);
            reverseLocationButton->ShowGWnd(true);

            cancelButton->MoveGWnd(reverseLocationButton->GetPos().x, reverseLocationButton->GetPos().y + 31);

            if (!g_CCharacterDependentData.IsInParty())
            {
                reversePartyButton->SetEnabledState(false);
            }
        }
    }
    else
    {
        reinterpret_cast<void(__thiscall*)(CIFMessageBox*, int, int)>(0x00644c90)(this, Id, a3);

        if (Id == 1 && a3 == 0)
        {
            g_pDisconnectRestartMessageBox = this;
            g_pDisconnectAutoRestartButton = 0;
            g_pDisconnectAutoRestartBox = 0;
            g_pDisconnectAutoRestartMark = 0;
            g_bDisconnectAutoRestart = false;

            CIFButton* confirmButton = this->m_IRM.GetResObj<CIFButton>(GDR_MBIN_BTN_YES, 1);

            if (confirmButton != 0)
            {
                confirmButton->SetText(KmtGetText(L"UIIT_KMT_CONFIRM"));

                const int btnWidth = confirmButton->GetSize().width;
                const int btnHeight = confirmButton->GetSize().height;

                int dlgWidth = this->GetSize().width;
                int dlgHeight = this->GetSize().height;
                const int dlgX = this->GetPos().x;
                const int dlgY = this->GetPos().y;

                int buttonX = dlgX + ((dlgWidth - btnWidth) / 2);

                if (buttonX < dlgX + 20)
                {
                    dlgWidth = btnWidth + 40;
                    this->SetGWndSize(dlgWidth, dlgHeight);
                    buttonX = dlgX + 20;
                }

                int confirmY = confirmButton->GetPos().y;
                if (confirmY < dlgY || confirmY > dlgY + dlgHeight - btnHeight)
                    confirmY = dlgY + dlgHeight - btnHeight - 18;

                confirmButton->MoveGWnd(buttonX, confirmY);

                CIFButton* oldAutoRestartButton = this->m_IRM.GetResObj<CIFButton>(GDR_DISCONNECT_RESTART, 1);
                if (oldAutoRestartButton != 0)
                    oldAutoRestartButton->ShowGWnd(false);

                const int checkSize = 16;
                const int labelWidth = 82;
                const int checkSpacing = -6;
                const int autoWidth = checkSize + checkSpacing + labelWidth;
                const int autoHeight = checkSize;
                const int autoX = dlgX + ((dlgWidth - autoWidth) / 2);
                int autoY = confirmY - autoHeight - 8;
                if (autoY < dlgY + 54)
                    autoY = dlgY + 54;

                wnd_rect checkRect;
                checkRect.pos.x = autoX;
                checkRect.pos.y = autoY - 4;
                checkRect.size.width = checkSize + checkSpacing + labelWidth;
                checkRect.size.height = 24;

                CIFButton* autoRestartCheckBox = this->m_IRM.GetResObj<CIFButton>(GDR_DISCONNECT_AUTORESTART_CHECK, 1);
                if (autoRestartCheckBox == 0)
                {
                    autoRestartCheckBox = (CIFButton*)CGWnd::CreateInstance(
                        this,
                        GFX_RUNTIME_CLASS(CIFButton),
                        checkRect,
                        GDR_DISCONNECT_AUTORESTART_CHECK,
                        0);
                }

                if (autoRestartCheckBox != 0)
                {
                    autoRestartCheckBox->MoveGWnd(checkRect.pos.x, checkRect.pos.y);
                    autoRestartCheckBox->SetGWndSize(checkRect.size.width, checkRect.size.height);
                    autoRestartCheckBox->TB_Func_13("", 0, 0);
                    autoRestartCheckBox->FUN_00656590(std::n_string(""));
                    autoRestartCheckBox->FUN_00656640(std::n_string(""));
                    autoRestartCheckBox->SetText(L"");
                    autoRestartCheckBox->SetClickable(true);
                    autoRestartCheckBox->ShowGWnd(true);
                    g_pDisconnectAutoRestartButton = autoRestartCheckBox;
                }

                wnd_rect boxRect;
                boxRect.pos.x = autoX;
                boxRect.pos.y = autoY;
                boxRect.size.width = checkSize;
                boxRect.size.height = checkSize;

                CIFStatic* autoRestartBox = this->m_IRM.GetResObj<CIFStatic>(GDR_DISCONNECT_AUTORESTART_BOX, 1);
                if (autoRestartBox == 0)
                {
                    autoRestartBox = (CIFStatic*)CGWnd::CreateInstance(
                        this,
                        GFX_RUNTIME_CLASS(CIFStatic),
                        boxRect,
                        GDR_DISCONNECT_AUTORESTART_BOX,
                        0);
                }

                if (autoRestartBox != 0)
                {
                    autoRestartBox->MoveGWnd(boxRect.pos.x, boxRect.pos.y);
                    autoRestartBox->SetGWndSize(boxRect.size.width, boxRect.size.height);
                    autoRestartBox->TB_Func_13("interface\\ifcommon\\com_checkbutton_off.ddj", 0, 0);
                    autoRestartBox->SetClickable(false);
                    autoRestartBox->ShowGWnd(true);
                    autoRestartBox->BringToFront();
                    g_pDisconnectAutoRestartBox = autoRestartBox;
                }

                wnd_rect markRect;
                markRect.pos.x = autoX;
                markRect.pos.y = autoY - 1;
                markRect.size.width = checkSize;
                markRect.size.height = checkSize;

                CIFStatic* autoRestartMark = this->m_IRM.GetResObj<CIFStatic>(GDR_DISCONNECT_AUTORESTART_MARK, 1);
                if (autoRestartMark == 0)
                {
                    autoRestartMark = (CIFStatic*)CGWnd::CreateInstance(
                        this,
                        GFX_RUNTIME_CLASS(CIFStatic),
                        markRect,
                        GDR_DISCONNECT_AUTORESTART_MARK,
                        0);
                }

                if (autoRestartMark != 0)
                {
                    autoRestartMark->MoveGWnd(markRect.pos.x, markRect.pos.y);
                    autoRestartMark->SetGWndSize(markRect.size.width, markRect.size.height);
                    autoRestartMark->SetText(L"X");
                    autoRestartMark->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
                    autoRestartMark->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
                    autoRestartMark->SetClickable(false);
                    autoRestartMark->ShowGWnd(false);
                    autoRestartMark->BringToFront();
                    g_pDisconnectAutoRestartMark = autoRestartMark;
                }

                wnd_rect labelRect;
                labelRect.pos.x = autoX + checkSize + checkSpacing;
                labelRect.pos.y = autoY - 1;
                labelRect.size.width = labelWidth;
                labelRect.size.height = 16;

                CIFStatic* autoRestartLabel = this->m_IRM.GetResObj<CIFStatic>(GDR_DISCONNECT_AUTORESTART_LABEL, 1);
                if (autoRestartLabel == 0)
                {
                    autoRestartLabel = (CIFStatic*)CGWnd::CreateInstance(
                        this,
                        GFX_RUNTIME_CLASS(CIFStatic),
                        labelRect,
                        GDR_DISCONNECT_AUTORESTART_LABEL,
                        0);
                }

                if (autoRestartLabel != 0)
                {
                    autoRestartLabel->MoveGWnd(labelRect.pos.x, labelRect.pos.y);
                    autoRestartLabel->SetGWndSize(labelWidth, 16);
                    autoRestartLabel->SetText(KmtGetText(L"UIIT_KMT_AUTO_RESTART"));
                    autoRestartLabel->SetClickable(false);
                    autoRestartLabel->ShowGWnd(true);
                    autoRestartLabel->BringToFront();
                }

                if (autoRestartCheckBox != 0)
                    autoRestartCheckBox->BringToFront();
            }
        }
    }
}

void CIFMessageBox::FUN_BTNPARTY()
{
    CIFMovePartyMember* movePartyWnd = g_pCGInterface->m_IRM.GetResObj<CIFMovePartyMember>(MovePartyMember, 1);

    if (movePartyWnd != 0 && !movePartyWnd->IsVisible())
    {
        movePartyWnd->UpdateMenuSize();
        movePartyWnd->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }

    this->Close();
}

void CIFMessageBox::FUN_BTNLOCATION()
{
    CIFSavedLocation* savedLocationWnd = g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1);

    if (savedLocationWnd != 0 && !savedLocationWnd->IsVisible())
    {
        savedLocationWnd->LoadLocations();
        savedLocationWnd->UpdateMenuSize();
        savedLocationWnd->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }

    this->Close();
}

int CIFMessageBox::OnMouseLeftDownIMPL(int a1, int x, int y)
{
    return reinterpret_cast<int(__thiscall *)(CIFWnd *, int, int, int)>(0x0046fd60)(this, a1, x, y);
}

void CIFMessageBox::ShowGWndIMPL(bool bVisible)
{
    if (!bVisible)
    {
        if (g_pCGInterface->IsVisible())
            CIFWnd::ShowGWnd(bVisible);

        CIFWnd::BringToFront();
        return;
    }

    if (g_pCGInterface->IsVisible())
    {
        CIFWnd::BringToFront();
        CIFWnd::ShowGWnd(bVisible);
    }
}
