#include "IFCOSSetup.h"
#include "BSLib/multibyte.h"
#include "Game.h"
#include "GInterface.h"
#include "GlobalHelpersThatHaveNoHomeYet.h"
#include "Macro/IFMacroMenu.h"
#include <algorithm>
#include <vector>

namespace {

const int PET_FILTER_BUTTON_ID = 5500;
const int PET_FILTER_LABEL_ID = 5501;
const int PET_FILTER_BUTTON_WIDTH = 76;
const int PET_FILTER_BUTTON_HEIGHT = 24;
const int PET_FILTER_BUTTON_GAP = 8;
const int PET_FILTER_BUTTON_Y_FALLBACK = 263;

bool CompareButtonsByX(CIFButton* left, CIFButton* right)
{
    if (left == NULL)
        return false;
    if (right == NULL)
        return true;

    return left->GetPos().x < right->GetPos().x;
}

bool IsBottomActionButton(CIFButton* button, int setupHeight)
{
    if (button == NULL || button->UniqueID() == PET_FILTER_BUTTON_ID)
        return false;

    CGWndBase::wnd_pos pos = button->GetPos();
    CGWndBase::wnd_size size = button->GetSize();
    return pos.y >= setupHeight - 90 && size.width >= 55 && size.height >= 18;
}

}

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFCOSSetup, 0x00eec0e8)
GFX_IMPLEMENT_DYNCREATE_FN(CIFCOSSetup, CIFMainFrame)
GFX_IMPLEMENT_DYNCREATE(CIFPetFilterButton, CIFButton)

GFX_BEGIN_MESSAGE_MAP(CIFCOSSetup, CIFWnd)
                    ONG_COMMAND(PET_FILTER_BUTTON_ID, &CIFCOSSetup::On_BtnClick)
GFX_END_MESSAGE_MAP()

CIFCOSSetup::CIFCOSSetup(void)
{
}

CIFCOSSetup::~CIFCOSSetup(void)
{
}

CIFPetFilterButton::CIFPetFilterButton()
{
}

CIFPetFilterButton::~CIFPetFilterButton()
{
}

int CIFPetFilterButton::OnMouseLeftUp(int a1, int x, int y)
{
    if (g_pCGInterface == NULL)
        return CIFButton::OnMouseLeftUp(a1, x, y);

    CIFMacroMenu* macroMenu = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
    if (macroMenu == NULL)
        return CIFButton::OnMouseLeftUp(a1, x, y);

    macroMenu->ShowGWnd(true);
    macroMenu->UpdateMenuSize();
    macroMenu->ActivateTabPage(3);
    macroMenu->BringToFront();
    return 0;
}

bool CIFCOSSetup::OnCreateIMPL(long ln)
{
    bool b = reinterpret_cast<bool (__thiscall *)(CIFCOSSetup *, long)>(0x007a86c0)(this, ln);
    return b;
}

void CIFCOSSetup::OnUpdateIMPL()
{
    reinterpret_cast<void (__thiscall *)(CIFCOSSetup *)>(0x006528a0)(this);
}

void CIFCOSSetup::On_BtnClick()
{
    if (g_pCGInterface == NULL)
        return;

    CIFMacroMenu* macroMenu = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
    if (macroMenu == NULL)
        return;

    macroMenu->ShowGWnd(true);
    macroMenu->UpdateMenuSize();
    macroMenu->ActivateTabPage(3);
    macroMenu->BringToFront();
}

void CIFCOSSetup::EnsurePetFilterButton()
{
    CIFButton* petFilterButton = (CIFButton*)GetChildControl(PET_FILTER_BUTTON_ID);
    CIFStatic* petFilterLabel = (CIFStatic*)GetChildControl(PET_FILTER_LABEL_ID);
    if (petFilterButton == NULL)
    {
        RECT rect = {128, PET_FILTER_BUTTON_Y_FALLBACK, PET_FILTER_BUTTON_WIDTH, PET_FILTER_BUTTON_HEIGHT};
        petFilterButton = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFPetFilterButton), rect, PET_FILTER_BUTTON_ID, 0);
    }

    if (petFilterLabel == NULL)
    {
        RECT rect = {128, PET_FILTER_BUTTON_Y_FALLBACK, PET_FILTER_BUTTON_WIDTH, PET_FILTER_BUTTON_HEIGHT};
        petFilterLabel = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), rect, PET_FILTER_LABEL_ID, 0);
    }

    if (petFilterButton == NULL || petFilterLabel == NULL)
        return;

    petFilterButton->TB_Func_13("interface\\ifcommon\\com_button.ddj", 0, 0);
    petFilterButton->SetText(KmtGetText(L"UIIT_KMT_PET_FILTER"));
    petFilterButton->SetFont(theApp.GetFont(0));
    petFilterButton->SetGWndSize(PET_FILTER_BUTTON_WIDTH, PET_FILTER_BUTTON_HEIGHT);
    petFilterButton->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    petFilterButton->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    petFilterButton->SetEnabledState(true);
    petFilterButton->SetClickable(true);
    petFilterButton->ShowGWnd(true);

    petFilterLabel->SetText(KmtGetText(L"UIIT_KMT_PET_FILTER"));
    petFilterLabel->SetFont(theApp.GetFont(0));
    petFilterLabel->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 239, 218, 164));
    petFilterLabel->SetGWndSize(PET_FILTER_BUTTON_WIDTH, PET_FILTER_BUTTON_HEIGHT);
    petFilterLabel->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    petFilterLabel->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    petFilterLabel->SetClickable(false);
    petFilterLabel->ShowGWnd(true);
}

void CIFCOSSetup::UpdatePetFilterButtonLayout(bool visible)
{
    EnsurePetFilterButton();

    CIFButton* petFilterButton = (CIFButton*)GetChildControl(PET_FILTER_BUTTON_ID);
    CIFStatic* petFilterLabel = (CIFStatic*)GetChildControl(PET_FILTER_LABEL_ID);
    if (petFilterButton == NULL || petFilterLabel == NULL)
        return;

    petFilterButton->ShowGWnd(visible);
    petFilterLabel->ShowGWnd(visible);
    if (!visible)
        return;

    CGWndBase::wnd_size setupSize = GetSize();
    std::vector<CIFButton*> bottomButtons;

    for (GWND_LIST::const_iterator it = N00000707.begin(); it != N00000707.end(); ++it)
    {
        if ((*it) == NULL || !(*it)->IsSame(GFX_RUNTIME_CLASS(CIFButton)))
            continue;

        CIFButton* button = (CIFButton*)(*it);
        if (IsBottomActionButton(button, setupSize.height))
            bottomButtons.push_back(button);
    }

    std::sort(bottomButtons.begin(), bottomButtons.end(), CompareButtonsByX);

    if (bottomButtons.size() < 2)
    {
        petFilterButton->ShowGWnd(false);
        petFilterLabel->ShowGWnd(false);
        return;
    }

    CIFButton* leftButton = bottomButtons[0];
    CIFButton* rightButton = bottomButtons[1];
    const int leftCenter = leftButton->GetPos().x + leftButton->GetSize().width / 2;
    const int rightCenter = rightButton->GetPos().x + rightButton->GetSize().width / 2;
    const int groupCenter = (leftCenter + rightCenter) / 2;
    const int totalWidth = (PET_FILTER_BUTTON_WIDTH * 3) + (PET_FILTER_BUTTON_GAP * 2);
    const int startX = groupCenter - totalWidth / 2;
    const int buttonY = leftButton->GetPos().y;

    leftButton->MoveGWnd(startX, buttonY);
    petFilterButton->MoveGWnd(startX + PET_FILTER_BUTTON_WIDTH + PET_FILTER_BUTTON_GAP, buttonY);
    petFilterLabel->MoveGWnd(startX + PET_FILTER_BUTTON_WIDTH + PET_FILTER_BUTTON_GAP, buttonY);
    rightButton->MoveGWnd(startX + ((PET_FILTER_BUTTON_WIDTH + PET_FILTER_BUTTON_GAP) * 2), buttonY);

    petFilterButton->BringToFront();
    petFilterLabel->BringToFront();
}
