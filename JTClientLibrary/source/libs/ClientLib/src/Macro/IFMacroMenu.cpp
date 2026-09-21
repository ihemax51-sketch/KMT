#include "IFMacroMenu.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <IFSelectableArea.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>


#define GDR_STATIC2 5
#define GDR_STATIC3 6
GFX_IMPLEMENT_DYNCREATE(CIFMacroMenu, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenu, CIFMainFrame)
                    ONG_COMMAND(100, &CIFMacroMenu::OnUnknownStuff)

GFX_END_MESSAGE_MAP()

CIFMacroMenu::CIFMacroMenu(void) {
    //   AutoPotionSlot = 0;
    descbox = 0;
    m_pTabs = 0;
    m_savedSlotsApplied = false;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFMacroMenu::~CIFMacroMenu(void) {
    if (m_pTabs) {
        delete[] m_pTabs;
        m_pTabs = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}
int CIFMacroMenu::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 7)
            return -1;
    }

    return 100;
}
bool CIFMacroMenu::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacromenu.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    this->SetText(KmtGetText(L"UIIT_KMT_SRO_MACRO"));
    wnd_rect sz;

    sz.pos.x= 45;
    sz.pos.y = 42;

    m_pTabs = new CIFSelectableArea *[numberOfTabs];

    for (int i = 0; i < numberOfTabs; i++) {

        RECT selectable_area_size;
        selectable_area_size.top = 42;
        selectable_area_size.left = 32;
        // selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth + 1;
        selectable_area_size.bottom = tabHeight;

        m_pTabs[i] = (CIFSelectableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                               selectable_area_size, tabFirstId + i, 0);

        m_pTabs[i]->SetFont(this->N00009C2F);


        m_pTabs[i]->sub_64CE30("interface\\option\\opt_long_tab_on.ddj",
                               "interface\\option\\opt_long_tab_off.ddj", "interface\\option\\opt_long_tab_off.ddj");

        switch (i) {
            case 0:
            {
                m_pTabs[0]->sub_64CC30(1);
                m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_AUTO_POTION"));
            }
                break;
            case 1:
            {
                m_pTabs[1]->MoveGWnd(m_pTabs[0]->GetPos().x + tabWidth + 10, m_pTabs[0]->GetPos().y);
                m_pTabs[1]->SetText(KmtGetText(L"UIIT_KMT_AUTO_SKILL_62A8C749"));
                // m_pTabs[1]->SetClickable(false);
            }
                break;
            case 2:
            {
                m_pTabs[2]->MoveGWnd(m_pTabs[1]->GetPos().x + tabWidth + 10, m_pTabs[1]->GetPos().y);
                m_pTabs[2]->SetText(KmtGetText(L"UIIT_KMT_AUTO_HUNTING"));
                //m_pTabs[2]->SetClickable(false);
            }
                break;
            case 3:
            {
                m_pTabs[3]->MoveGWnd(m_pTabs[2]->GetPos().x + tabWidth + 10, m_pTabs[2]->GetPos().y);

                m_pTabs[3]->SetText((KmtGetText(L"UIIT_KMT_AUTO_PICK_UP")));
            }
                break;
            case 4:
            {
                m_pTabs[4]->MoveGWnd(m_pTabs[3]->GetPos().x + tabWidth + 10, m_pTabs[3]->GetPos().y);

                m_pTabs[4]->SetText(KmtGetText(L"UIIT_KMT_AUTO_SCROLL"));
                // m_pTabs[4]->SetClickable(false);
            }
                break;
        }
        m_pTabs[i]->sub_64CC30(0);

    }


    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pHandleBar->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pCloseBtn->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));

    descbox = this->m_IRM.GetResObj<CIFTextBox>(4, 1);
   // descbox->JustifyHorizontal(JUSTIFY_LEFT);
    //descbox->JustifyVertical(JUSTIFY_TOP);

    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->MoveGWnd(this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->GetPos().x, this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->GetPos().y - 8);


    sz.pos.x= 9;
    sz.pos.y = 64;
    sz.size.width = 683;
    sz.size.height = 412;

   AutoPotionSlot = m_IRM.GetResObj<CIFMacroMenuAutoPotion>(300, 1);
   AutoPotionSlot->BringToFront();



    AutoSkillSlot = m_IRM.GetResObj<CIFMacroMenuAutoSkill>(301, 1);

    AutoHuntSlot = m_IRM.GetResObj<CIFMacroMenuAutoHunt>(302, 1);

    PickupFilterSlot = m_IRM.GetResObj<CIFMacroMenuPickFilter>(303, 1);
    AutoScrollSlot = m_IRM.GetResObj<CIFMacroMenuAutoScrollSlot>(304, 1);
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == 0)
            continue;

        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }


    m_pTabs[0]->sub_64CC30(1);


    //AutoSkillSlot->ShowGWnd(false);
    descbox->SetText(KmtGetText(L"UIIT_KMT_SET_THE_OF_THE_HP_MP_GAUGE_TO_THE_DESIRED_LEVEL_TO_USE_POTIONS_AUTOMATICALLY_W_3649EBE7"));
    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_POTION"));

    //AutoPotionSlot->ShowGWnd(true);
    //AutoHuntSlot->ShowGWnd(false);
    //PickupFilterSlot->ShowGWnd(false);
    //AutoScrollSlot->ShowGWnd(false);
    UpdateMenuSize();
    this->ShowGWnd(false);



    return true;
}

void CIFMacroMenu::OnUpdate()
{
    if (!m_savedSlotsApplied && m_Player && !m_Player->m_MacroAutoSkillData.empty())
        ApplySavedSlots();
}

void CIFMacroMenu::ApplySavedSlots()
{
    if (!m_Player || !AutoPotionSlot || !AutoSkillSlot || !AutoScrollSlot)
        return;

    for (std::map<int, CustomCICPlayer::Macro_AutoSkillData>::const_iterator it =
             m_Player->m_MacroAutoSkillData.begin();
         it != m_Player->m_MacroAutoSkillData.end(); ++it)
    {
        const CustomCICPlayer::Macro_AutoSkillData& saved = it->second;
        const int sequence = saved.SlotSeq;

        if (sequence >= 141 && sequence <= 152)
        {
            CIFMacroSlot* slot = AutoPotionSlot->MySlots[sequence - 141];
            if (slot && slot->m_pMySlot && slot->m_pMySlot->m_pSlot)
            {
                if (saved.SlotType == 0)
                    slot->m_pMySlot->m_pSlot->ClearSlot();
                else
                    slot->LoadSlot(sequence, saved.SlotType, saved.Data);
            }
        }
        else if (sequence >= 153 && sequence <= 156)
        {
            CIFMacroSlotWep* slot = 0;
            switch (sequence)
            {
                case 153: slot = AutoSkillSlot->SkillWeaponSlot; break;
                case 154: slot = AutoSkillSlot->BuffWeaponSlot; break;
                case 155: slot = AutoSkillSlot->SkillShieldSlot; break;
                case 156: slot = AutoSkillSlot->BuffShieldSlot; break;
            }
            if (slot && slot->m_pMySlot && slot->m_pMySlot->m_pSlot)
            {
                if (saved.SlotType == 0)
                    slot->m_pMySlot->m_pSlot->ClearSlot();
                else
                    slot->LoadSlot(sequence, saved.SlotType, saved.Data);
            }
        }
        else if (sequence >= 157 && sequence <= 180)
        {
            CIFMacroSlotSkill* slot = AutoSkillSlot->skillslots[sequence - 157];
            if (slot && slot->m_pMySlot && slot->m_pMySlot->m_pSlot)
            {
                if (saved.SlotType == 0)
                    slot->m_pMySlot->m_pSlot->ClearSlot();
                else
                    slot->LoadSlot(sequence, saved.SlotType, saved.Data);
            }
        }
        else if (sequence >= 181 && sequence <= 204)
        {
            CIFMacroSlotSkill* slot = AutoSkillSlot->buffslots[sequence - 181];
            if (slot && slot->m_pMySlot && slot->m_pMySlot->m_pSlot)
            {
                if (saved.SlotType == 0)
                    slot->m_pMySlot->m_pSlot->ClearSlot();
                else
                    slot->LoadSlot(sequence, saved.SlotType, saved.Data);
            }
        }
        else if (sequence >= 205 && sequence <= 212)
        {
            CIFMacroSCSlot* slot = AutoScrollSlot->Slot1[sequence - 205];
            if (slot && slot->m_pMySlot && slot->m_pMySlot->m_pSlot)
            {
                if (saved.SlotType == 0)
                    slot->m_pMySlot->m_pSlot->ClearSlot();
                else
                    slot->LoadSlot(sequence, saved.SlotType, saved.Data);
            }
        }
    }

    m_savedSlotsApplied = true;
    if (AutoPotionSlot->Macro_AutoPotion)
        AutoPotionSlot->StartAutomation();
}
void CIFMacroMenu::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFMacroMenu::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }


    m_pTabs[page]->sub_64CC30(1);
    switch (page)
    {
        case 0:
        {
            descbox->SetText(KmtGetText(L"UIIT_KMT_SET_THE_OF_THE_HP_MP_GAUGE_TO_THE_DESIRED_LEVEL_TO_USE_POTIONS_AUTOMATICALLY_W_3649EBE7"));
            this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_POTION"));


            AutoHuntSlot->ShowGWnd(false);
            AutoSkillSlot->ShowGWnd(false);
            PickupFilterSlot->ShowGWnd(false);
            AutoScrollSlot->ShowGWnd(false);
            AutoPotionSlot->ShowGWnd(true);
            AutoPotionSlot->BringToFront();
        }
            break;
        case 1:
        {
            descbox->SetText(KmtGetText(L"UIIT_KMT_ASSIGN_THE_SKILLS_AND_WEAPONS_SHIELDS_TO_USE_THE_SKILLS_AUTOMATICALLY"));
            this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_SKILLS"));


            AutoScrollSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoPotionSlot->ShowGWnd(false);
            PickupFilterSlot->ShowGWnd(false);
            AutoSkillSlot->ShowGWnd(true);
            AutoSkillSlot->BringToFront();
        }
            break;

        case 2:
        {
            descbox->SetText(KmtGetText(L"UIIT_KMT_MOVE_TO_THE_LOCATION_TO_HUNT_IN_AND_RUN_AUTO_HUNT_TO_BEGIN_HUNTING_AUTOMATICALLY"));
            this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_HUNT"));


            AutoScrollSlot->ShowGWnd(false);
            PickupFilterSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoPotionSlot->ShowGWnd(false);
            AutoSkillSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(true);
            AutoHuntSlot->BringToFront();
        }
            break;
        case 3:
        {
            descbox->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_THE_PICK_UP_FILTER_FOR_AUTO_PICK_UP"));
            this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_PICK_UP_FILTER"));


            AutoScrollSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoPotionSlot->ShowGWnd(false);
            AutoSkillSlot->ShowGWnd(false);
            PickupFilterSlot->ShowGWnd(true);
            PickupFilterSlot->BringToFront();
        }
            break;
        case 4:
        {
            descbox->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_THE_AUTO_SCROLL_PUT_ITEMS_TO_SLOTS_FOR_AUTO_USE"));
            this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_SCROLL"));


            PickupFilterSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoPotionSlot->ShowGWnd(false);
            AutoSkillSlot->ShowGWnd(false);
            AutoScrollSlot->ShowGWnd(true);
            AutoScrollSlot->BringToFront();
        }
        break;

        case 5:
        {
            descbox->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_THE_AUTO_ALCHEMY"));
            this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_ALCHEMY"));

            PickupFilterSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoHuntSlot->ShowGWnd(false);
            AutoPotionSlot->ShowGWnd(false);
            AutoSkillSlot->ShowGWnd(false);
            AutoScrollSlot->ShowGWnd(false);

        }
            break;
    }
}

void CIFMacroMenu::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2 - 50) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
