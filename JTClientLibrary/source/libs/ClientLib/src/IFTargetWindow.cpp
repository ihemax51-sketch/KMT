#include <CustomData/CustomDataManager.h>
#include "IFTargetWindow.h"
#include "GInterface.h"
#include "GlobalHelpersThatHaveNoHomeYet.h"
#include "IFButton.h"
#include "ICPlayer.h"
#include "ICMonster.h"
#include "unsorted.h"
#include <Windows.h>

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFTargetWindow, 0x00eea57c)

GFX_IMPLEMENT_DYNCREATE_FN(CIFTargetWindow, CIFWnd)

namespace {

const int GDR_TARGET_POSSIBLE_DROPS_OVERLAY = 13870;
bool g_possibleDropsMouseDown = false;

CIFButton* GetPossibleDropsOverlayButton()
{
    if (g_pCGInterface == NULL)
        return NULL;

    CIFButton* button = g_pCGInterface->GetGuiFromList<CIFButton>(GDR_TARGET_POSSIBLE_DROPS_OVERLAY);
    if (button != NULL)
        return button;

    RECT rect = {0, 0, 26, 20};
    button = (CIFButton*)g_pCGInterface->CreateInstance(g_pCGInterface, GFX_RUNTIME_CLASS(CIFButton), rect, GDR_TARGET_POSSIBLE_DROPS_OVERLAY, 0);
    if (button != NULL) {
        button->TB_Func_13("interface\\mall\\mall_coice_button.ddj", 0, 1);
        button->SetText(L">");
        button->SetGWndSize(26, 20);
        button->SetClickable(true);
        button->ShowGWnd(false);
    }

    return button;
}

int GetMouseTargetId()
{
    CGWndBase* target = g_CurrentIfUnderCursor;
    int id = target != NULL ? target->UniqueID() : -1;

    if (id != GDR_TARGET_POSSIBLE_DROPS_OVERLAY && g_pOnMouseDownClickCtrl != NULL)
        id = g_pOnMouseDownClickCtrl->UniqueID();

    return id;
}

bool IsSelectedTargetMonster()
{
    if (g_pCGInterface == NULL)
        return false;

    const int selectedObjectId = g_pCGInterface->Get_SelectedObjectId();
    if (selectedObjectId == 0)
        return false;

    CICharactor* selectedObject = GetCharacterObjectByID_MAYBE(selectedObjectId);
    return selectedObject != NULL && selectedObject->IsSame(GFX_RUNTIME_CLASS(CICMonster));
}

}

enum {
    GDR_TW_BUFF = 100, // CIFBuffViewer
    GDR_TW_CLOSE = 11, // CIFCloseButton
    GDR_TW_FORTRESSSTRUCTER = 8, // CIFTargetWindowFortressStructure
    GDR_TW_COMMONENEMY = 4, // CIFTargetWindowCommonEnemy
    GDR_TW_SPECIALMOBWND = 2, // CIFTargetWindowSpecialMob
    GDR_TW_JOB_PLAYERWND = 1, // CIFTargetWindowJobPlayer
    GDR_TW_PLAYERWND = 0, // CIFTargetWindowPlayer
};

GFX_BEGIN_MESSAGE_MAP(CIFTargetWindow, CIFWnd)
                    ONG_COMMAND(GDR_TW_CLOSE, &CIFTargetWindow::OnClick_11)
GFX_END_MESSAGE_MAP()

void CIFTargetWindow::OnTimer(int timerId) {
    //printf("%s\n", __FUNCTION__);
    reinterpret_cast<void (__thiscall *)(const CIFTargetWindow*, int)>(0x006993a0)(this, timerId);
}

bool CIFTargetWindow::OnCreate(long ln) {
    //printf("%s\n", __FUNCTION__);
    // return reinterpret_cast<bool (__thiscall *)(const CIFTargetWindow*, long)>(0x00699080)(this, ln);

    m_IRM.LoadFromFile("resinfo\\iftargetwindow.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_pGDR_TW_PLAYERWND = m_IRM.GetResObj<CIFTargetWindowPlayer>(GDR_TW_PLAYERWND, 1); // 0x0
    m_pGDR_TW_JOB_PLAYERWND = m_IRM.GetResObj<CIFTargetWindowJobPlayer>(GDR_TW_JOB_PLAYERWND, 1); // 0x1
    m_pGDR_TW_SPECIALMOBWND = m_IRM.GetResObj<CIFTargetWindowSpecialMob>(GDR_TW_SPECIALMOBWND, 1); // 0x2
    m_pGDR_TW_COMMONENEMY = m_IRM.GetResObj<CIFTargetWindowCommonEnemy>(GDR_TW_COMMONENEMY, 1); // 0x4
    m_pCIFBuffViewer = m_IRM.GetResObj<CIFBuffViewer>(GDR_TW_BUFF, 1); // 0x64

    m_pGDR_TW_PLAYERWND->ShowGWnd(false);
    m_pGDR_TW_JOB_PLAYERWND->ShowGWnd(false);
    m_pGDR_TW_SPECIALMOBWND->ShowGWnd(false);
    m_pGDR_TW_COMMONENEMY->ShowGWnd(false);
    m_pCIFBuffViewer->ShowGWnd(false);

    m_pGDR_TW_FORTRESSSTRUCTER = m_IRM.GetResObj<CIFTargetWindowFortressStructure>(GDR_TW_FORTRESSSTRUCTER, 1); // 0x8
    m_pGDR_TW_FORTRESSSTRUCTER->ShowGWnd(false);

    return true;
}

void CIFTargetWindow::OnUpdate() {
    //printf("%s\n", __FUNCTION__);
    reinterpret_cast<void (__thiscall *)(const CIFTargetWindow*)>(0x00698ae0)(this);
}

void CIFTargetWindow::ShowGWndIMPL(bool bVisible) {
    //printf("%s\n", __FUNCTION__);
    reinterpret_cast<void (__thiscall *)(const CIFTargetWindow*, bool)>(0x00698b50)(this, bVisible);
    if(bVisible == 1)
    {
        if(g_pMyPlayerObj != NULL)
        {
            if(m_CustomDataManager->m_EventMapSettings.find(g_pMyPlayerObj->GetRegion().r) != m_CustomDataManager->m_EventMapSettings.end())
            {
                if(m_CustomDataManager->m_EventMapSettings[g_pMyPlayerObj->GetRegion().r].HideBuffViewer)
                {
                    if(m_pCIFBuffViewer != NULL)
                    {
                        if(m_pCIFBuffViewer->IsVisible())
                        {
                            m_pCIFBuffViewer->ShowGWnd(false);
                        }
                    }
                }
            }
        }

    }

   // CIFWnd::ShowGWnd(bVisible);
}

undefined1 CIFTargetWindow::OnCloseWnd() {
    //printf("%s\n", __FUNCTION__);
    return reinterpret_cast<undefined1 (__thiscall *)(const CIFTargetWindow*)>(0x0069a300)(this);
}

void CIFTargetWindow::OnClick_11() {
    //printf("%s\n", __FUNCTION__);
    reinterpret_cast<undefined1 (__thiscall *)(const CIFTargetWindow*)>(0x00698be0)(this);
}

void CIFTargetWindow::OnClickPossibleDrops() {
    if (!IsSelectedTargetMonster())
        return;

    if (m_pGDR_TW_COMMONENEMY != NULL && m_pGDR_TW_COMMONENEMY->IsVisible()) {
        m_pGDR_TW_COMMONENEMY->OnClickPossibleDrops();
        return;
    }

    if (m_pGDR_TW_SPECIALMOBWND != NULL && m_pGDR_TW_SPECIALMOBWND->IsVisible()) {
        m_pGDR_TW_SPECIALMOBWND->OnClickPossibleDrops();
    }
}

void UpdateTargetPossibleDropsButtonOverlay()
{
    if (g_pCGInterface == NULL)
        return;

    CIFButton* button = GetPossibleDropsOverlayButton();
    if (button == NULL)
        return;

    CIFTargetWindow* targetWindow = g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(GDR_TARGETWINDOW, 1);
    const bool showButton = targetWindow != NULL && targetWindow->IsVisible() &&
                            IsSelectedTargetMonster() &&
                            ((targetWindow->m_pGDR_TW_COMMONENEMY != NULL && targetWindow->m_pGDR_TW_COMMONENEMY->IsVisible()) ||
                             (targetWindow->m_pGDR_TW_SPECIALMOBWND != NULL && targetWindow->m_pGDR_TW_SPECIALMOBWND->IsVisible()));

    if (!showButton) {
        button->ShowGWnd(false);
        g_possibleDropsMouseDown = false;
        return;
    }

    CGWndBase::wnd_pos targetPos = targetWindow->GetPos();
    button->MoveGWnd(targetPos.x + 188, targetPos.y + 51);
    button->ShowGWnd(true);
    button->BringToFront();

    const bool mouseDown = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (g_possibleDropsMouseDown && !mouseDown && GetMouseTargetId() == GDR_TARGET_POSSIBLE_DROPS_OVERLAY)
        targetWindow->OnClickPossibleDrops();

    g_possibleDropsMouseDown = mouseDown;
}
