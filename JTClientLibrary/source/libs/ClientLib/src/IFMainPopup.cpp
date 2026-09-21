#include <BSLib/Debug.h>
#include <CustomData/CustomSettingManager.h>
#include <ctime>
#include "IFMainPopup.h"
#include "TextStringManager.h"
#include "GInterface.h"
#include "Game.h"

#include "support/MemberFunctionHook.h"

#define GDR_BTN_APPRENTICE 16
#define GDR_BTN_QUEST 15
#define GDR_BTN_PARTY 14
#define GDR_BTN_ACT 13
#define GDR_BTN_SKILL 12
#define GDR_BTN_INV 11
#define GDR_BTN_CHAR 10
#define GDR_MAINPOPUP_LEFT_DECO_STATIC 4
#define GDR_MAINPOPUP_BG_TILE 3

#define GDR_INVENTORY_DESIGN_BACKGROUND 6100
#define GDR_INVENTORY_DESIGN_HEADER 6101
#define GDR_INVENTORY_DESIGN_STORAGE_PANEL 6102
#define GDR_INVENTORY_DESIGN_EQUIPMENT_PANEL 6103
#define GDR_INVENTORY_DESIGN_STORAGE_TITLE 6104
#define GDR_INVENTORY_DESIGN_EQUIPMENT_TITLE 6105

namespace {
const int MAINPOPUP_LEGACY_WIDTH = 388;
const int MAINPOPUP_LEGACY_HEIGHT = 408;
const int INVENTORY_CHAMBER_WIDTH = 650;
const int INVENTORY_CHAMBER_HEIGHT = 455;
const D3DCOLOR INVENTORY_COLOR_GOLD = D3DCOLOR_ARGB(255, 239, 218, 164);
const D3DCOLOR INVENTORY_COLOR_CYAN = D3DCOLOR_ARGB(255, 132, 225, 221);

bool IsNewInventoryDesignEnabled()
{
    return m_Settings == NULL || m_Settings->NewInventoryDesign;
}

void PositionPopupChild(CIFMainPopup* popup, CIFWnd* child,
                        int x, int y, int width, int height)
{
    if (!popup || !child) {
        return;
    }

    const int originX = popup->GetPos().x;
    const int originY = popup->GetPos().y;
    child->MoveGWnd(originX + x, originY + y);
    child->SetGWndSize(width, height);
}

void SetControlVisible(CIFWnd* control, bool visible)
{
    if (control) {
        control->ShowGWnd(visible);
    }
}

void SetupInventoryDecoration(CIFStatic* control, const char* texture)
{
    if (!control) {
        return;
    }

    control->TB_Func_13(texture, 0, 0);
    control->SetClickable(false);
    control->ShowGWnd(false);
}

void SetupInventoryHeading(CIFStatic* control, const wchar_t* text, D3DCOLOR color)
{
    if (!control) {
        return;
    }

    control->SetText(text);
    control->SetFont(theApp.GetFont(0));
    control->m_FontTexture.SetColor(color);
    control->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    control->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    control->SetClickable(false);
    control->ShowGWnd(false);
}

void StyleInventoryControlTree(CGWndBase* root)
{
    if (!root) {
        return;
    }

    for (GWND_LIST::const_iterator it = root->N00000707.begin(); it != root->N00000707.end(); ++it) {
        CGWndBase* child = *it;
        if (!child) {
            continue;
        }

        if (child->IsKindOf(GFX_RUNTIME_CLASS(CIFButton))) {
            CIFButton* button = (CIFButton*)child;
            const std::n_wstring& text = button->GetNText();
            const wchar_t* pageText = KmtGetText(L"UIIT_KMT_PAGE");
            const size_t pageLength = pageText ? wcslen(pageText) : 0;
            if (pageLength > 0 &&
                text.length() >= pageLength &&
                wcsncmp(text.c_str(), pageText, pageLength) == 0) {
                button->TB_Func_13("clientlibrary\\mall\\mall_pre_button.ddj", 1, 1);
                button->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 245, 242, 232));
            }
        }

        StyleInventoryControlTree(child);
    }
}
}

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFMainPopup, 0x00eea6dc)

GFX_IMPLEMENT_DYNCREATE_FN(CIFMainPopup, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFMainPopup, CIFMainFrame)
                    ONG_COMMAND(GDR_BTN_CHAR, &CIFMainPopup::OnClick_BtnChar)
                    ONG_COMMAND(GDR_BTN_INV, &CIFMainPopup::OnClick_BtnInv)
                    ONG_COMMAND(GDR_BTN_SKILL, &CIFMainPopup::OnClick_BtnSkill)
                    ONG_COMMAND(GDR_BTN_ACT, &CIFMainPopup::OnClick_BtnAct)
                    ONG_COMMAND(GDR_BTN_PARTY, &CIFMainPopup::OnClick_BtnParty)
                    ONG_COMMAND(GDR_BTN_QUEST, &CIFMainPopup::OnClick_BtnQuest)
                    ONG_COMMAND(GDR_BTN_APPRENTICE, &CIFMainPopup::OnClick_BtnApprentice)
GFX_END_MESSAGE_MAP()

CIFMainPopup::CIFMainPopup() :
        m_pInventoryWindow(NULL),
        m_pEquipmentWindow(NULL),
        m_pSkillWindow(NULL),
        m_pActionWindow(NULL),
        m_pPartyWindow(NULL),
        m_pApprenticeShipWindow(NULL),
        m_pPlayerInfoWindow(NULL),
        m_pQuestWindow(NULL),
        m_nWindowType(DUMMY) {
}

bool CIFMainPopup::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    const bool useNewDesign = IsNewInventoryDesignEnabled();

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 0);
    SetGWndSize(MAINPOPUP_LEGACY_WIDTH, MAINPOPUP_LEGACY_HEIGHT);

    if (useNewDesign) {
        RECT designBackgroundRect = {0, 31, INVENTORY_CHAMBER_WIDTH, INVENTORY_CHAMBER_HEIGHT - 31};
        CIFStatic* designBackground = (CIFStatic*)CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFStatic), designBackgroundRect, GDR_INVENTORY_DESIGN_BACKGROUND, 0);
        SetupInventoryDecoration(designBackground, "clientlibrary\\mall\\win_bg.ddj");

        RECT designHeaderRect = {0, 31, INVENTORY_CHAMBER_WIDTH, 45};
        CIFStatic* designHeader = (CIFStatic*)CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFStatic), designHeaderRect, GDR_INVENTORY_DESIGN_HEADER, 0);
        SetupInventoryDecoration(designHeader, "clientlibrary\\mall\\header.ddj");

        RECT storagePanelRect = {15, 78, 188, 356};
        CIFStatic* storagePanel = (CIFStatic*)CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFStatic), storagePanelRect, GDR_INVENTORY_DESIGN_STORAGE_PANEL, 0);
        SetupInventoryDecoration(storagePanel, "clientlibrary\\mall\\leftbg.ddj");

        RECT equipmentPanelRect = {213, 78, 422, 356};
        CIFStatic* equipmentPanel = (CIFStatic*)CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFStatic), equipmentPanelRect, GDR_INVENTORY_DESIGN_EQUIPMENT_PANEL, 0);
        SetupInventoryDecoration(equipmentPanel, "clientlibrary\\mall\\mall_pre_start.ddj");

        RECT storageTitleRect = {22, 48, 174, 24};
        CIFStatic* storageTitle = (CIFStatic*)CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFStatic), storageTitleRect, GDR_INVENTORY_DESIGN_STORAGE_TITLE, 0);
        SetupInventoryHeading(storageTitle, KmtGetText(L"UIIT_KMT_INVENTORY_VAULT"), INVENTORY_COLOR_GOLD);

        RECT equipmentTitleRect = {230, 48, 388, 24};
        CIFStatic* equipmentTitle = (CIFStatic*)CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFStatic), equipmentTitleRect, GDR_INVENTORY_DESIGN_EQUIPMENT_TITLE, 0);
        SetupInventoryHeading(equipmentTitle, KmtGetText(L"UIIT_KMT_LIVE_EQUIPMENT_CHAMBER"), INVENTORY_COLOR_CYAN);
    }


        m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmainpopup.txt");
        m_IRM.CreateInterfaceSection("Create", this);


    m_pInventoryWindow = m_IRM.GetResObj<CIFInventory>(GDR_INVENTORY, 1);
    m_pEquipmentWindow = m_IRM.GetResObj<CIFEquipment>(GDR_EQUIPMENT, 1);
    m_pSkillWindow = m_IRM.GetResObj<CIFSkill>(GDR_SKILL, 1);
    m_pActionWindow = m_IRM.GetResObj<CIFAction>(GDR_ACTION, 1);
    m_pPartyWindow = m_IRM.GetResObj<CIFParty>(GDR_PARTY, 1);
    m_pPlayerInfoWindow = m_IRM.GetResObj<CIFPlayerInfo>(GDR_PLAYERINFO, 1);
    m_pQuestWindow = m_IRM.GetResObj<CIFQuest>(GDR_QUEST, 1);
    m_pApprenticeShipWindow = m_IRM.GetResObj<CIFApprenticeShip>(GDR_APPRENTICESHIP, 1);

    m_pBtnInventory = m_IRM.GetResObj<CIFButton>(GDR_BTN_INV, 1);
    m_pBtnSkill = m_IRM.GetResObj<CIFButton>(GDR_BTN_SKILL, 1);
    m_pBtnAction = m_IRM.GetResObj<CIFButton>(GDR_BTN_ACT, 1);
    m_pBtnParty = m_IRM.GetResObj<CIFButton>(GDR_BTN_PARTY, 1);
    m_pBtnCharacter = m_IRM.GetResObj<CIFButton>(GDR_BTN_CHAR, 1);
    m_pBtnQuest = m_IRM.GetResObj<CIFButton>(GDR_BTN_QUEST, 1);
    m_pBtnApprentice = m_IRM.GetResObj<CIFButton>(GDR_BTN_APPRENTICE, 1);

    m_pBackground = m_IRM.GetResObj<CIFStatic>(GDR_MAINPOPUP_BG_TILE, 1);

    PositionPopupChild(this, m_pInventoryWindow,
            useNewDesign ? 21 : 13, useNewDesign ? 91 : 63, 176, 333);
    PositionPopupChild(this, m_pEquipmentWindow,
            useNewDesign ? 220 : 198, useNewDesign ? 82 : 41,
            useNewDesign ? 408 : 178, useNewDesign ? 350 : 355);

    if (m_pInventoryWindow) {
        CIFWnd* inventoryFrame = m_pInventoryWindow->GetGuiFromList<CIFWnd>(0);
        if (inventoryFrame) {
            inventoryFrame->TB_Func_13(useNewDesign
                    ? "interface\\frame\\mall_sub_wnd04_"
                    : "interface\\inventory\\int_window_", 0, 0);
        }

        CIFStatic* goldLabel = m_pInventoryWindow->GetGuiFromList<CIFStatic>(12);
        CIFStatic* goldValue = m_pInventoryWindow->GetGuiFromList<CIFStatic>(10);
        const D3DCOLOR inventoryTextColor = useNewDesign
                ? INVENTORY_COLOR_GOLD : D3DCOLOR_ARGB(255, 255, 255, 255);
        if (goldLabel) {
            goldLabel->m_FontTexture.SetColor(inventoryTextColor);
        }
        if (goldValue) {
            goldValue->m_FontTexture.SetColor(inventoryTextColor);
        }
    }

    m_pBtnCharacter->FUN_00655fa0(8);
    m_pBtnInventory->FUN_00655fa0(8);
    m_pBtnSkill->FUN_00655fa0(8);
    m_pBtnAction->FUN_00655fa0(8);
    m_pBtnParty->FUN_00655fa0(8);
    m_pBtnQuest->FUN_00655fa0(8);
    m_pSkillWindow->FUN_006a6e60(1);

    m_pBtnInventory->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_STT_TOGGLE_INVENTORY")));
    m_pBtnInventory->SetStyleThingy(TOOLTIP);

    m_pBtnCharacter->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_STT_TOGGLE_CHARACTER")));
    m_pBtnCharacter->SetStyleThingy(TOOLTIP);

    m_pBtnSkill->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_STT_TOGGLE_SKILL")));
    m_pBtnSkill->SetStyleThingy(TOOLTIP);

    m_pBtnAction->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_STT_TOGGLE_ACTION")));
    m_pBtnAction->SetStyleThingy(TOOLTIP);

    m_pBtnParty->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_STT_TOGGLE_PARTY")));
    m_pBtnParty->SetStyleThingy(TOOLTIP);

    m_pBtnQuest->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_STT_TOGGLE_QUEST")));
    m_pBtnQuest->SetStyleThingy(TOOLTIP);

    m_pBtnApprentice->SetTooltip(TSM_GETTEXT(KmtGetText(L"UIIT_CTL_TC_SHORTKEY_L")));
    m_pBtnApprentice->SetStyleThingy(TOOLTIP);

    // The new chamber owns its full surface. The original design keeps the
    // project's existing OldMainPopup switch for its native side rail.
    const bool showLegacySidebar = !useNewDesign &&
            m_Settings != NULL && m_Settings->EnableOldMainPopup;
    SetControlVisible(m_pBtnInventory, showLegacySidebar);
    SetControlVisible(m_pBtnSkill, showLegacySidebar);
    SetControlVisible(m_pBtnAction, showLegacySidebar);
    SetControlVisible(m_pBtnParty, showLegacySidebar);
    SetControlVisible(m_pBtnCharacter, showLegacySidebar);
    SetControlVisible(m_pBtnQuest, showLegacySidebar);
    SetControlVisible(m_pBtnApprentice, showLegacySidebar);
    CIFWnd* legacyRail = m_IRM.GetResObj<CIFWnd>(GDR_MAINPOPUP_LEFT_DECO_STATIC, 1);
    SetControlVisible(legacyRail, showLegacySidebar);
    if (useNewDesign && m_pInventoryWindow) {
        m_pInventoryWindow->BringToFront();
    }

    if (useNewDesign && m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(INVENTORY_COLOR_GOLD);
        m_pTitleText->BringToFront();
    }
    if (useNewDesign && m_pCloseBtn) {
        m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
        m_pCloseBtn->SetGWndSize(28, 32);
        m_pCloseBtn->BringToFront();
    }
    ShowInventoryDecorations(false);

    return true;
}

void CIFMainPopup::ShowInventoryDecorations(bool visible) {
    const int decorationIds[] = {
            GDR_INVENTORY_DESIGN_BACKGROUND,
            GDR_INVENTORY_DESIGN_HEADER,
            GDR_INVENTORY_DESIGN_STORAGE_TITLE,
            GDR_INVENTORY_DESIGN_EQUIPMENT_TITLE
    };

    for (int i = 0; i < sizeof(decorationIds) / sizeof(decorationIds[0]); ++i) {
        CIFWnd* decoration = GetGuiFromList<CIFWnd>(decorationIds[i]);
        if (decoration) {
            decoration->ShowGWnd(visible);
        }
    }
}

void CIFMainPopup::ApplyInventoryPresentation(bool inventoryPage) {
    const bool useNewDesign = inventoryPage && IsNewInventoryDesignEnabled();
    const int width = useNewDesign ? INVENTORY_CHAMBER_WIDTH : MAINPOPUP_LEGACY_WIDTH;
    const int height = useNewDesign ? INVENTORY_CHAMBER_HEIGHT : MAINPOPUP_LEGACY_HEIGHT;

    TB_Func_13(useNewDesign
            ? "interface\\frame\\mall_sub_wnd04_"
            : "interface\\frame\\mframe_wnd_", 0, 0);
    SetGWndSize(width, height);
    ShowInventoryDecorations(useNewDesign);

    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(useNewDesign
                ? INVENTORY_COLOR_GOLD
                : D3DCOLOR_ARGB(255, 255, 255, 255));
        m_pTitleText->BringToFront();
    }
    if (m_pCloseBtn) {
        if (useNewDesign) {
            m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
            m_pCloseBtn->SetGWndSize(28, 32);
            m_pCloseBtn->MoveGWnd(GetPos().x + width - 36, GetPos().y + 2);
        } else {
            m_pCloseBtn->TB_Func_13("interface\\ifcommon\\com_windowclose.ddj", 0, 0);
            m_pCloseBtn->SetGWndSize(16, 16);
            m_pCloseBtn->MoveGWnd(GetPos().x + width - 26, GetPos().y + 10);
        }
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }
}

void CIFMainPopup::OnUpdate() {
    CIFWnd::OnUpdate();
}

void CIFMainPopup::HideAll() {
    m_pInventoryWindow->ShowGWnd(false);
    m_pEquipmentWindow->ShowGWnd(false);
    m_pSkillWindow->ShowGWnd(false);
    m_pActionWindow->ShowGWnd(false);
    m_pPartyWindow->ShowGWnd(false);
    m_pPlayerInfoWindow->ShowGWnd(false);
    m_pQuestWindow->ShowGWnd(false);
    m_pApprenticeShipWindow->ShowGWnd(false);
}

void CIFMainPopup::ShowSubPage(int nPageId) {
    HideAll();

    ApplyInventoryPresentation(nPageId == GDR_INVENTORY);

    wnd_rect rect = GetBounds();

    const ClientResolutonData &res = CGame::GetClientDimensionStuff();

    switch (nPageId) {
        case GDR_INVENTORY: {
            m_nWindowType = MAINPOP_INVENTORY;
            SetText(TSM_GETTEXTPTR(L"UIIT_STT_INVENTORY"));

            m_pInventoryWindow->ShowGWnd(true);
            m_pEquipmentWindow->ShowGWnd(true);

            if (IsNewInventoryDesignEnabled()) {
                StyleInventoryControlTree(m_pInventoryWindow);
                m_pBackground->ShowGWnd(false);

                int targetY = (res.height - GetSize().height) / 2;
                if (targetY < 35) {
                    targetY = 35;
                }
                if (targetY + GetSize().height > res.height - 20) {
                    targetY = res.height - GetSize().height - 20;
                }

                int targetX = res.width - GetSize().width - 40;
                if (targetX < 20) {
                    targetX = 20;
                }
                MoveGWnd(targetX, targetY);

                m_pInventoryWindow->BringToFront();
                m_pEquipmentWindow->BringToFront();
            } else {
                m_pBackground->ShowGWnd(true);
                m_pBackground->MoveGWnd(rect.pos.x + 189, rect.pos.y + 68);
                m_pBackground->SetGWndSize(9, 292);
                MoveGWnd(((res.width - GetSize().width) - 80), GetPos().y);
            }

            BringToFront();

            /* mySkillSlot->BringToFront();
             mySkillSlot->ShowGWnd(true);*/
            break;
        }

        case GDR_PARTY: {
            m_nWindowType = MAINPOP_PARTY;
            SetText(TSM_GETTEXTPTR(L"UIIT_STT_PARTY"));

            m_pPartyWindow->ShowGWnd(true);
            m_pBackground->ShowGWnd(false);

            MoveGWnd(((res.width - GetSize().width) - 80), GetPos().y);
            BringToFront();
            break;
        }

        case GDR_SKILL: {
            m_nWindowType = MAINPOP_SKILL;
            SetText(TSM_GETTEXTPTR(L"UIIT_STT_SKILL"));

            m_pSkillWindow->ShowGWnd(true);
            m_pBackground->ShowGWnd(false);

            MoveGWnd(((res.width - GetSize().width) - 80), GetPos().y);
            BringToFront();
            break;
        }

        case GDR_ACTION: {
            m_nWindowType = MAINPOP_ACTION;
            SetText(TSM_GETTEXTPTR(L"UIIT_STT_ACTION"));

            m_pActionWindow->ShowGWnd(true);

            m_pBackground->ShowGWnd(true);
            m_pBackground->MoveGWnd(rect.pos.x + 40, rect.pos.y + 154);
            m_pBackground->SetGWndSize(308, 124);

            MoveGWnd(((res.width - GetSize().width) - 80), GetPos().y);
            BringToFront();
            break;
        }

        case GDR_PLAYERINFO: {
            m_nWindowType = MAINPOP_PLAYER_INFO;
            SetText(TSM_GETTEXTPTR(L"UIIT_STT_CHARACTER"));

            m_pPlayerInfoWindow->ShowGWnd(true);
            m_pBackground->ShowGWnd(false);

            MoveGWnd(((res.width - GetSize().width) - 80), GetPos().y);
            BringToFront();
            break;
        }

        case GDR_QUEST: {
            m_nWindowType = MAINPOP_QUEST;
            SetText(TSM_GETTEXTPTR(L"UIIT_STT_QUEST"));

            m_pQuestWindow->ShowGWnd(true);
            m_pBackground->ShowGWnd(false);

            MoveGWnd(80, GetPos().y);
            BringToFront();
            break;
        }

        case GDR_APPRENTICESHIP: {
            m_nWindowType = MAINPOP_ACADEMY;
            SetText(TSM_GETTEXTPTR(L"UIIT_CTL_TC_TRAININGCAMP"));

            m_pApprenticeShipWindow->ShowGWnd(true);
            m_pBackground->ShowGWnd(true);
            m_pBackground->ShowGWnd(false);

            MoveGWnd(((res.width - GetSize().width) - 80), GetPos().y);
            BringToFront();

            break;
        }
    }
}

void CIFMainPopup::OnClick_BtnChar() {
    ShowSubPage(GDR_PLAYERINFO);
}

void CIFMainPopup::OnClick_BtnInv() {
    ShowSubPage(GDR_INVENTORY);
}

void CIFMainPopup::OnClick_BtnSkill() {
    ShowSubPage(GDR_SKILL);
}

void CIFMainPopup::OnClick_BtnAct() {
    ShowSubPage(GDR_ACTION);
}

void CIFMainPopup::OnClick_BtnParty() {
    ShowSubPage(GDR_PARTY);
}

void CIFMainPopup::OnClick_BtnQuest() {
    g_pCGInterface->ToggleQuestNew();
#ifdef CONFIG_OLD_MAINPOPUP
    g_pCGInterface->ToggleQuestNew();
#else
//    ShowSubPage(GDR_QUEST);
#endif
}

void CIFMainPopup::OnClick_BtnApprentice() {
    ShowSubPage(GDR_APPRENTICESHIP);
}

void CIFMainPopup::RenderMyself() {
    CIFFrame::RenderMyself();
}

void CIFMainPopup::ShowGWnd(bool bVisible) {
    if (bVisible) {
        switch (this->m_nWindowType) {
            case MAINPOP_PLAYER_INFO:
                ShowSubPage(GDR_PLAYERINFO);
                break;
            case MAINPOP_INVENTORY:
                ShowSubPage(GDR_INVENTORY);
                break;
            case MAINPOP_SKILL:
                ShowSubPage(GDR_SKILL);
                break;
            case MAINPOP_ACTION:
                ShowSubPage(GDR_ACTION);
                break;
            case MAINPOP_PARTY:
                ShowSubPage(GDR_PARTY);
                break;
            case MAINPOP_QUEST:
                ShowSubPage(GDR_QUEST);
                break;
            case MAINPOP_ACADEMY:
                ShowSubPage(GDR_APPRENTICESHIP);
                break;
        }
    } else {
        m_pInventoryWindow->ShowGWnd(false);
        m_pEquipmentWindow->ShowGWnd(false);
        m_pSkillWindow->ShowGWnd(false);
        m_pActionWindow->ShowGWnd(false);
        m_pPartyWindow->ShowGWnd(false);
        m_pPlayerInfoWindow->ShowGWnd(false);
        m_pQuestWindow->ShowGWnd(false);
        m_pApprenticeShipWindow->ShowGWnd(false);
        if(m_Settings->EnableAutoSkill)
        {
            this->GetSkill()->GetGuiFromList<CIFSkillBoard>(10)->OnCloseWndIMPL();
        }
    }

    CIFMainFrame::ShowGWnd(bVisible);
}


HOOK_ORIGINAL_MEMBER(0x006a1d20, &CIFMainPopup::GetInventory);
CIFInventory *CIFMainPopup::GetInventory() const {
    BS_ASSERT_MSG(m_pInventoryWindow, "Mainpop에 세팅된 Inventory가 이상하다");
    return m_pInventoryWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1d80, &CIFMainPopup::GetSkill);
CIFSkill *CIFMainPopup::GetSkill() const {
    BS_ASSERT_MSG(m_pSkillWindow, "Mainpop에 세팅된 Skill이 이상하다");
    return m_pSkillWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1db0, &CIFMainPopup::GetAction);
CIFAction *CIFMainPopup::GetAction() const {
    BS_ASSERT_MSG(m_pActionWindow, "Mainpop에 세팅된 Action이 이상하다");
    return m_pActionWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1de0, &CIFMainPopup::GetParty);
CIFParty *CIFMainPopup::GetParty() const {
    BS_ASSERT_MSG(m_pPartyWindow, "Mainpop에 세팅된 Party가 이상하다");
    return m_pPartyWindow;
}

HOOK_ORIGINAL_MEMBER(0x06a1e10, &CIFMainPopup::GetPlayerInfo);
CIFPlayerInfo *CIFMainPopup::GetPlayerInfo() const {
    BS_ASSERT_MSG(m_pPlayerInfoWindow, "Mainpop에 세팅된 PlayerInfo가 이상하다");
    return m_pPlayerInfoWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1e40, &CIFMainPopup::GetQuest);
CIFQuest *CIFMainPopup::GetQuest() const {
    BS_ASSERT_MSG(m_pQuestWindow, "Mainpop에 세팅된 Quest가 이상하다");
    return m_pQuestWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1e70, &CIFMainPopup::GetApprenticeShip);
CIFApprenticeShip *CIFMainPopup::GetApprenticeShip() const {
    BS_ASSERT_MSG(m_pApprenticeShipWindow, "Mainpop에 세팅된 ApprenticeShip이 이상하다");
    return m_pApprenticeShipWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1d50, &CIFMainPopup::GetEquipment);
CIFEquipment *CIFMainPopup::GetEquipment() const {
    BS_ASSERT_MSG(m_pEquipmentWindow, "Mainpop에 세팅된 Equipment가 이상하다");
    return m_pEquipmentWindow;
}

HOOK_ORIGINAL_MEMBER(0x006a1cf0, &CIFMainPopup::IsSubPageActive);
bool CIFMainPopup::IsSubPageActive(int nPageId) {
    return m_IRM.GetResObj<CIFWnd>(nPageId, 1)->IsVisible();
}
