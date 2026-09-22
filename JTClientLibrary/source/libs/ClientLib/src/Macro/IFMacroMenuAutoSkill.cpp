#include "IFMacroMenuAutoSkill.h"
#include <support/SafePath.h>
#include "Game.h"
#include "IFMacroMenu.h"
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <IFSliderCtrl.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomSettingManager.h>
#include <SRIFLib/NInterfaceResource.h>
#include <SRIFLib/NIFLattice.h>
#include <ICMonster.h>
#include <algorithm>
#include <EntityManagerClient.h>
#include <PartyData.h>
#include <CharacterDependentData.h>
#include <IFPlayerMiniInfo.h>
#include <iostream>
#include <NavMesh/LocationInfo.h>
#include <NavMesh/IRegionManager.h>
#include <cmath>
#include <ctime>
#include <sstream>
#include "IFMacro.h"
#include "../../../../DevKit_DLL/src/Util.h"
#define GDR_SAVE_BUTTON 29
#define GDR_CANCEL_BUTTON 30

#define GDR_SKILL_WEAPON 153
#define GDR_BUFF_WEAPON 154

#define StartOf

static DWORD g_lastMacroEquipTick = 0;
static DWORD g_lastMacroActionTick = 0;
static DWORD g_lastMacroTargetTick = 0;
static DWORD g_lastMacroZerkTick = 0;
static int g_macroAmmoRefObjId = 0;
static const DWORD MACRO_EQUIP_DELAY_MS = 1100;
static const DWORD MACRO_ACTION_DELAY_MS = 850;
static const DWORD MACRO_TARGET_DELAY_MS = 500;
static const DWORD MACRO_ZERK_DELAY_MS = 2500;

static const SItemData* GetMacroWeaponSlotData(CIFMacroSlotWep* slot)
{
    if (!slot || !slot->m_pMySlot || !slot->m_pMySlot->m_pSlot ||
        slot->m_pMySlot->m_pSlot->ItemInfo == 0x0)
        return 0;

    return slot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
}

static bool IsMacroSlotEquipped(const SItemData* equippedItem, const SItemData* configuredItem)
{
    return equippedItem != 0 && configuredItem != 0 && equippedItem->RefObjectId == configuredItem->RefObjectId;
}

static bool IsShieldCompatibleMacroWeapon(const SItemData* item)
{
    if (!item || item->m_typeId.getTypeID1() != 3 ||
        item->m_typeId.getTypeID2() != 1 || item->m_typeId.getTypeID3() != 6)
    {
        return false;
    }

    const tid_t weaponType = item->m_typeId.getTypeID4();
    return weaponType == 2 || weaponType == 3 || weaponType == 7 ||
           weaponType == 10 || weaponType == 15;
}

static bool IsCompatibleMacroAmmo(const SItemData* weapon, const SItemData* item)
{
    if (!weapon || !item)
        return false;

    if (weapon->IsChBow())
        return item->IsArrow();

    if (weapon->IsEuCrossbow())
        return item->IsBolt();

    return false;
}

static int FindMacroAmmoInventorySlot(const SItemData* weapon)
{
    if (!weapon || !g_pCGInterface || !g_pCGInterface->GetMainPopup())
        return -1;

    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (!inventory)
        return -1;

    const int slotCount = inventory->InventorySlotCount();
    int compatibleSlot = -1;
    for (int slot = 0; slot < slotCount; ++slot)
    {
        CSOItem* item = inventory->GetItemBySlot(slot);
        if (!item || item->m_blValid == 0 || item->GetQuantity() <= 0)
            continue;

        const SItemData* itemData = item->GetItemData();
        if (!IsCompatibleMacroAmmo(weapon, itemData))
            continue;

        if (itemData->RefObjectId == g_macroAmmoRefObjId)
            return slot;

        if (compatibleSlot == -1)
            compatibleSlot = slot;
    }

    return compatibleSlot;
}

static bool TryRestoreMacroAmmo(const SItemData* weapon, const SItemData* equippedOffhand)
{
    if (!weapon || (!weapon->IsChBow() && !weapon->IsEuCrossbow()))
        return true;

    if (IsCompatibleMacroAmmo(weapon, equippedOffhand))
    {
        g_macroAmmoRefObjId = equippedOffhand->RefObjectId;
        return true;
    }

    DWORD now = GetTickCount();
    if (now - g_lastMacroEquipTick < MACRO_EQUIP_DELAY_MS)
        return false;

    const int ammoSlot = FindMacroAmmoInventorySlot(weapon);
    if (ammoSlot < 0)
        return false;

    CMsgStreamBuffer buf(0x7034);
    buf << BYTE(0x00) << BYTE(ammoSlot + 0x0D) << BYTE(7) << UINT16(0x00);
    SendMsg(buf);
    g_lastMacroEquipTick = now;
    return false;
}

static bool TryWearMacroSlot(CIFMacroSlotWep* slot, const SItemData* equippedItem)
{
    const SItemData* configuredItem = GetMacroWeaponSlotData(slot);
    if (!configuredItem)
        return false;

    if (IsMacroSlotEquipped(equippedItem, configuredItem))
        return true;

    if (configuredItem->m_typeId.getTypeID1() == 3 &&
        configuredItem->m_typeId.getTypeID2() == 1 &&
        configuredItem->m_typeId.getTypeID3() == 4 &&
        equippedItem && (equippedItem->IsArrow() || equippedItem->IsBolt()))
    {
        g_macroAmmoRefObjId = equippedItem->RefObjectId;
    }

    DWORD now = GetTickCount();
    if (now - g_lastMacroEquipTick < MACRO_EQUIP_DELAY_MS)
        return false;

    slot->UseItem(slot->m_pMySlot->m_pSlot->GetSlot());
    g_lastMacroEquipTick = now;
    return false;
}

static bool IsMacroEquipPending()
{
    DWORD now = GetTickCount();
    return g_lastMacroEquipTick != 0 && now - g_lastMacroEquipTick < MACRO_EQUIP_DELAY_MS;
}

static bool IsMacroActionReady()
{
    if (IsMacroEquipPending())
        return false;

    if (g_pMyPlayerObj && g_pMyPlayerObj->CHARACTER_STATUS == SkillCast)
        return false;

    DWORD now = GetTickCount();
    return now - g_lastMacroActionTick >= MACRO_ACTION_DELAY_MS;
}

static void MarkMacroAction()
{
    g_lastMacroActionTick = GetTickCount();
}

static bool TryUseMacroSkillSlot(CIFMacroSlotSkill* slot)
{
    if (!slot || !slot->m_pMySlot || !slot->m_pMySlot->m_pSlot || !IsMacroActionReady())
        return false;

    slot->UseItem(slot->m_pMySlot->m_pSlot->GetSlot());
    MarkMacroAction();
    return true;
}

static bool TrySelectMacroTarget(unsigned int uniqueId)
{
    if (!g_pCGInterface || uniqueId == 0)
        return false;

    if (g_pCGInterface->Get_SelectedObjectId() == uniqueId)
        return true;

    DWORD now = GetTickCount();
    if (now - g_lastMacroTargetTick < MACRO_TARGET_DELAY_MS)
        return false;

    CMsgStreamBuffer buf(0x7045);
    buf << uniqueId;
    SendMsg(buf);
    g_lastMacroTargetTick = now;
    return false;
}

static bool TryUseMacroZerk()
{
    if (!IsMacroActionReady())
        return false;

    DWORD now = GetTickCount();
    if (now - g_lastMacroZerkTick < MACRO_ZERK_DELAY_MS)
        return false;

    CIFPlayerMiniInfo *miniinfo = (CIFPlayerMiniInfo*)g_pCGInterface->m_IRM.GetResObj(11, 1);
    if (miniinfo == NULL || !miniinfo->GetZerkButtonState())
        return false;

    CMsgStreamBuffer buf(0x70A7);
    buf << (byte)1;
    SendMsg(buf);
    g_lastMacroZerkTick = now;
    MarkMacroAction();
    return true;
}

GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoSkill, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoSkill, CIFWnd)
                    ONG_COMMAND(300, &CIFMacroMenuAutoSkill::OnUnknownStuff)
                    ONG_COMMAND(301, &CIFMacroMenuAutoSkill::OnUnknownStuff)
                    ONG_COMMAND(302, &CIFMacroMenuAutoSkill::OnUnknownStuff)
                    ONG_COMMAND(GDR_SAVE_BUTTON, &CIFMacroMenuAutoSkill::SaveButton)
                    ONG_COMMAND(GDR_CANCEL_BUTTON, &CIFMacroMenuAutoSkill::CancelButton)
                    ONG_COMMAND(903, &CIFMacroMenuAutoSkill::AddPtMemberBtn)
                    ONG_COMMAND(904, &CIFMacroMenuAutoSkill::RemovePtMemberBtn)
                    ONG_COMMAND(99999, &CIFMacroMenuAutoSkill::LoadPartyMembers)
GFX_END_MESSAGE_MAP()

CIFMacroMenuAutoSkill::CIFMacroMenuAutoSkill(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
    m_pTabsSecond = 0;
    PartyBuffList = std::map<std::n_wstring, std::vector<int>>();
    SelectedPartyMemberName = std::n_wstring();
    Macro_AutoSkill = false;
    AutoSkillTimerRunning = false;
    MyLastResUseTime = 0;
    ActiveTabPage = -1;
    replace = -1;
    SelectObj = 0;
    SelectObjUniqueId = 0;
}
CIFMacroMenuAutoSkill::~CIFMacroMenuAutoSkill(void) {
    if (m_pTabsSecond) {
        delete[] m_pTabsSecond;
        m_pTabsSecond = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}
void CIFMacroMenuAutoSkill::AddPtMemberBtn(){

    std::n_wstring charname = m_popup->m_text->GetNText();
    if(charname.empty())
    {
        g_pCGInterface->ShowMessage_Notice(KmtGetText(L"UIIT_KMT_ENTER_THE_CHARACTER_NAME"));

        return;
    }
    if(PartyBuffList.size() < 7)
    {
        if(PartyBuffList.find(charname) == PartyBuffList.end())
        {
            std::vector<int> p;
            PartyBuffList.insert(std::make_pair(charname, p));
            for (int i = 0; i < 18; ++i)
            {
                for(int j = 0; j < 8; ++j)
                {
                    m_slotviewer[i]->slot1[j]->slot->N595 = 2;
                }
            }
            ClearPartySlotDDJ();
        }
        else
        {
            g_pCGInterface->ShowMessage_Notice(KmtGetText(L"UIIT_KMT_THIS_PARTY_MEMBER_IS_ALREADY_ADDED_TO_LIST"));

        }
    }
    else
    {
        g_pCGInterface->ShowMessage_Notice(KmtGetText(L"UIIT_KMT_MAXIMUM_CHARACTER_MEMBER_CAN_BE_ADD_TO_LIST"));

    }
    int i = 0;
    for(std::map<std::n_wstring, std::vector<int>>::iterator  it = PartyBuffList.begin(); it != PartyBuffList.end(); it++)
    {
        i++;
        if(i < 8)
        {
            m_PartySlot[i-1]->LoadItems(it->first);
        }
        else
        {
            break;
        }

    }
    m_popup->m_text->SetText(L"");
}
void CIFMacroMenuAutoSkill::RemovePtMemberBtn(){
    if(!SelectedPartyMemberName.empty())
    {
        if(PartyBuffList.find(SelectedPartyMemberName) != PartyBuffList.end())
        {
            PartyBuffList.erase(SelectedPartyMemberName);
            ClearPartySlotDDJ();
            for (int i = 0; i < 18; ++i)
            {
                for(int j = 0; j < 8; ++j)
                {
                    m_slotviewer[i]->slot1[j]->slot->N595 = 2;
                }
            }
            for(int i = 0; i < 7; i++)
            {
                m_PartySlot[i]->LoadItems(L"");
            }

            int i = 0;
            for(std::map<std::n_wstring , std::vector<int>>::iterator  it = PartyBuffList.begin(); it != PartyBuffList.end(); it++)
            {
                i++;
                if(i < 8)
                {
                    m_PartySlot[i-1]->LoadItems(it->first);
                }
                else
                {
                    break;
                }

            }
        }

    }
}

void CIFMacroMenuAutoSkill::SelectedPartyBuffs()
{
    if(!SelectedPartyMemberName.empty())
    {
        if(PartyBuffList.find(SelectedPartyMemberName) != PartyBuffList.end())
        {
            for (int i = 0; i < 18; ++i)
            {
                for(int j = 0; j < 8; ++j)
                {
                    m_slotviewer[i]->slot1[j]->slot->N595 = 2;
                }
            }
            std::vector<int>& skillList = PartyBuffList[SelectedPartyMemberName];

            for (int i = 0; i < 18; ++i)
            {
                for(int j = 0; j < 8; ++j)
                {
                    std::vector<int>::iterator skillIt = std::find(skillList.begin(), skillList.end(), m_slotviewer[i]->slot1[j]->slot->m_SkillID);
                    if (skillIt != skillList.end()) {
                        // m_SkillID varsa sil
                        m_slotviewer[i]->slot1[j]->slot->N595 = 0;
                    }
                }
            }
        }

    }

}

bool CIFMacroMenuAutoSkill::OnCreate(long ln) {

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    wnd_rect sz;
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacromenuautoskill.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pHandleBar->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pCloseBtn->ShowGWnd(false);
    // this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));
    this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->MoveGWnd(this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->GetPos().x, this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->GetPos().y - 8);

    this->m_IRM.GetResObj<CIFMainFrame>(4, 1)->m_pHandleBar->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFMainFrame>(4, 1)->m_pCloseBtn->ShowGWnd(false);
    // this->m_IRM.GetResObj<CIFMainFrame>(3, 1)->m_pTitleText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));
    this->m_IRM.GetResObj<CIFMainFrame>(4, 1)->m_pTitleText->MoveGWnd(this->m_IRM.GetResObj<CIFMainFrame>(4, 1)->m_pTitleText->GetPos().x, this->m_IRM.GetResObj<CIFMainFrame>(4, 1)->m_pTitleText->GetPos().y - 8);

    m_IRM.GetResObj(30, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    sz.pos.x= 45;
    sz.pos.y = 42;
    m_pTabsSecond = new CIFSelectableArea *[numberOfTabs];

    for (int i = 0; i < numberOfTabs; i++) {

        RECT selectable_area_size;
        selectable_area_size.top = 121;
        selectable_area_size.left = 24;
        // selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth + 1;
        selectable_area_size.bottom = tabHeight;

        m_pTabsSecond[i] = (CIFSelectableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                                     selectable_area_size, tabFirstId + i, 0);
        m_pTabsSecond[i]->SetFont(this->N00009C2F);


        m_pTabsSecond[i]->sub_64CE30("interface\\option\\opt_video_tab_long01_on.ddj",
                                     "interface\\option\\opt_video_tab_long01_off.ddj", "interface\\option\\opt_video_tab_long01_off.ddj");

        switch (i) {
            case 0:
            {
                m_pTabsSecond[0]->sub_64CC30(1);
                m_pTabsSecond[0]->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_ATTACK_SKILLS"));

            }
                break;
            case 1:
            {
                m_pTabsSecond[1]->MoveGWnd(m_pTabsSecond[0]->GetPos().x + tabWidth + 10, m_pTabsSecond[0]->GetPos().y);
                m_pTabsSecond[1]->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_BUFF_SKILLS"));

                m_pTabsSecond[1]->SetClickable(true);
            }
                break;
            case 2:
            {
                m_pTabsSecond[2]->MoveGWnd(m_pTabsSecond[1]->GetPos().x + tabWidth + 10, m_pTabsSecond[1]->GetPos().y);
                m_pTabsSecond[2]->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_PARTY_BUFF_SKILLS"));

                m_pTabsSecond[2]->SetClickable(true);

            }
        }
        m_pTabsSecond[i]->sub_64CC30(0);


    }

    sz.pos.x = 25;
    sz.pos.y = 186;
    sz.size.width = 303;
    sz.size.height = 187;
    CNIFLattice * p = (CNIFLattice*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CNIFLattice),
                                                          sz, 1000, 0);
    p->NTB_Func_13("interface\\ifcommon\\lattice_window\\com_lattice_", 0, 0);


    sz.pos.x = 353;
    sz.pos.y = 252;
    sz.size.width = 303;
    sz.size.height = 121;
    ps = (CNIFLattice*)CNIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CNIFLattice),
                                               sz, 1001, 0);

    ps->NTB_Func_13("interface\\ifcommon\\lattice_window\\com_lattice_", 0, 0);

    this->m_IRM.GetResObj(3, 1)->SetText(KmtGetText(L"UIIT_KMT_ACQUIRED_SKILLS"));
    this->m_IRM.GetResObj(4, 1)->SetText(KmtGetText(L"UIIT_KMT_SKILLS_TO_USE"));
    this->m_IRM.GetResObj(11, 1)->SetText(KmtGetText(L"UIIT_KMT_WEAPON"));
    this->m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_SHIELD"));
    this->m_IRM.GetResObj(29, 1)->SetText(KmtGetText(L"UIIT_KMT_SAVE"));


    m_scroll_skill_list = this->m_IRM.GetResObj<CIFScrollManager>(7, 1);
    m_scroll_skill_list->sub_008124F0(0);
    m_scroll_skill_list->sub_008124C0(36);
    m_scroll_skill_list->sub_008123F0(5);
    m_scroll_skill_list->sub_00812500(0);
    m_scroll_skill_list->sub_00812420(-8, 0);
    m_scroll_skill_list->BringToFront();

    sz.pos.x = 353;
    sz.pos.y = 348;
    sz.size.width = 180;
    sz.size.height = 24;
    m_popup = m_IRM.GetResObj<CIFPopupList2>(901, 1);


    m_popup->m_text->SetGWndSize(200, 24);
    m_popup->m_bg->SetGWndSize(200, 24);

    sz.pos.x = 0;
    sz.pos.y = 0;
    sz.size.width = 303;
    sz.size.height = 36;
    for(int i =0; i< 18;i++) {
        m_slotviewer[i] = (CIFMacroMenuAutoSkillSlotViewer*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroMenuAutoSkillSlotViewer), sz, 301 + i, 0);
        m_slotviewer[i]->ShowGWnd(false);
    }

    for (int i = 0; i < 7; ++i)
    {
        m_PartySlot[i] = m_IRM.GetResObj<CIFMacroMenuAutoSkillPartySlot>(5000+i, 1);
    }

    SkillWeaponSlot = this->m_IRM.GetResObj<CIFMacroSlotWep>(GDR_SKILL_WEAPON, 1);
    SkillWeaponSlot->m_pMySlot->m_pSlot->SetSlot(153);
    SkillWeaponSlot->m_pMySlot->m_pSlot->SetType(0xC);

    BuffWeaponSlot = this->m_IRM.GetResObj<CIFMacroSlotWep>(GDR_BUFF_WEAPON, 1);
    BuffWeaponSlot->m_pMySlot->m_pSlot->SetSlot(154);
    BuffWeaponSlot->m_pMySlot->m_pSlot->SetType(0xC);

    SkillShieldSlot = this->m_IRM.GetResObj<CIFMacroSlotWep>(155, 1);
    SkillShieldSlot->m_pMySlot->m_pSlot->SetSlot(155);
    SkillShieldSlot->m_pMySlot->m_pSlot->SetType(0xC);

    BuffShieldSlot = this->m_IRM.GetResObj<CIFMacroSlotWep>(156, 1);
    BuffShieldSlot->m_pMySlot->m_pSlot->SetSlot(156);
    BuffShieldSlot->m_pMySlot->m_pSlot->SetType(0xC);


    sz.pos.x = 354;
    sz.pos.y = 253;
    sz.size.width = 32;
    sz.size.height = 32;

    int startX = sz.pos.x;
    int startY = sz.pos.y;

    int offsetX = 36;
    int offsetY = 36;

    for (int i = 0; i < 24; i++) {
        int row = i / 8;
        int col = i % 8;

        sz.pos.x = 354 + col * offsetX;
        sz.pos.y = 253 + row * offsetY;

        skillslots[i] = (CIFMacroSlotSkill*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroSlotSkill), sz, 10005 + i, 0);
        skillslots[i]->m_pMySlot->m_pSlot->SetType(0xC);
        skillslots[i]->m_pMySlot->m_pSlot->SetSlot(157+i);
        if ((i + 1) % 8 == 0) {
            sz.pos.x = 354; // X koordinatÄ±nÄ± baÅŸa al
            sz.pos.y += offsetY; // Y koordinatÄ±nÄ± bir sonraki satÄ±ra taÅŸÄ±
        }
    }

    sz.pos.x = 354;
    sz.pos.y = 253;
    sz.size.width = 32;
    sz.size.height = 32;

    int startXx = sz.pos.x;
    int startYy = sz.pos.y;

    int offsetXx = 36;
    int offsetYy = 36;

    for (int i = 0; i < 24; i++) {
        int row = i / 8;
        int col = i % 8;

        sz.pos.x = 354 + col * offsetXx;
        sz.pos.y = 253 + row * offsetYy;


        buffslots[i] = (CIFMacroSlotSkill*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroSlotSkill), sz, 10405 + i, 0);
        buffslots[i]->m_pMySlot->m_pSlot->SetType(0xC);
        buffslots[i]->m_pMySlot->m_pSlot->SetSlot(181+i);
        if ((i + 1) % 8 == 0) {
            sz.pos.x = 354; // X koordinatÄ±nÄ± baÅŸa al
            sz.pos.y += offsetYy; // Y koordinatÄ±nÄ± bir sonraki satÄ±ra taÅŸÄ±
        }
    }
    m_popup->m_btn->SetUniqueID(99999);

    return true;
}
void CIFMacroMenuAutoSkill::LoadPartyMembers()
{

    int numtoDel =  m_popup->m_list->GetNumberOfItems() +1;
    for(int i =0;i<=numtoDel;i++) {

        m_popup->m_list->Removeline(0);
    }
    m_popup->m_list->m_CurrentLines = 0;

    const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();

    if (partyData.NumberOfMembers > 0)
    {
        if (partyData.NumberOfMembers > 0) {

            for (int i = 0; i < partyData.NumberOfMembers; ++i) {

                const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
                if(g_pMyPlayerObj->GetCharName() != memberData.m_charactername)
                {
                    std::n_wstring ptmember = memberData.m_charactername.c_str();
                    m_popup->m_list->sub_64F8A0(ptmember, 0, 0xffffff, 0xffffff, -1, 0, 0);
                }
            }
        }
    }

    m_popup->m_listbg->ShowGWnd(!m_popup->m_listbg->IsVisible());
    m_popup->m_list->ShowGWnd(!m_popup->m_list->IsVisible());

    m_popup->m_listbg->SetGWndSize(m_popup->m_listbg->GetSize().width, m_popup->m_list->m_CurrentLines * 14 + 10);
    m_popup->m_list->SetGWndSize(m_popup->m_listbg->GetSize().width, m_popup->m_list->m_CurrentLines * 14 + 10);

    m_popup->m_listbg->BringToFront();
    m_popup->m_list->BringToFront();
}
void CIFMacroMenuAutoSkill::OnUpdate() {

}

void CIFMacroMenuAutoSkill::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;
    //printf("%id %d\n", id);
    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabsSecond[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFMacroMenuAutoSkill::LoadPartyBuffInfos()
{
    int i = 0;
    for(std::map<std::n_wstring, std::vector<int>>::iterator  it = PartyBuffList.begin(); it != PartyBuffList.end(); it++)
    {
        i++;
        if(i < 8)
        {
            m_PartySlot[i-1]->LoadItems(it->first);
        }
        else
        {
            break;
        }

    }
}
void CIFMacroMenuAutoSkill::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabsSecond[i]->sub_64CC30(0);
        m_pTabsSecond[i]->m_FontTexture.sub_8B4750(2);
    }
    ActiveTabPage = page;

    m_pTabsSecond[page]->sub_64CC30(1);
    switch (page)
    {
        case 0:
        {
            Clear();
            LoadSkills();
            for (int i = 0; i< 7; i++)
            {
                m_PartySlot[i]->ShowGWnd(false);
            }
            m_IRM.GetResObj(6, 1)->ShowGWnd(true);
            m_IRM.GetResObj(8, 1)->ShowGWnd(true);
            m_IRM.GetResObj(10, 1)->ShowGWnd(true);
            m_IRM.GetResObj(11, 1)->ShowGWnd(true);
            m_IRM.GetResObj(12, 1)->ShowGWnd(true);
            m_IRM.GetResObj(13, 1)->ShowGWnd(true);
            ps->SetGWndSize(303, 121);
            ps->ShowGWnd(true);
            m_IRM.GetResObj(901, 1)->ShowGWnd(false);

            m_IRM.GetResObj(903, 1)->ShowGWnd(false);
            m_IRM.GetResObj(904, 1)->ShowGWnd(false);
            m_IRM.GetResObj(900, 1)->ShowGWnd(false);

            SkillWeaponSlot->ShowGWnd(true);
            SkillWeaponSlot->BringToFront();
            SkillShieldSlot->ShowGWnd(true);
            SkillShieldSlot->BringToFront();

            BuffWeaponSlot->ShowGWnd(false);
            BuffShieldSlot->ShowGWnd(false);
            for (int i = 0; i < 24; i++)
            {
                buffslots[i]->ShowGWnd(false);
                skillslots[i]->ShowGWnd(true);
                skillslots[i]->BringToFront();
            }
        }
            break;
        case 1:
        {
            for (int i = 0; i < 7; i++)
            {
                m_PartySlot[i]->ShowGWnd(false);
            }
            Clear();
            LoadBuffs();
            m_IRM.GetResObj(6, 1)->ShowGWnd(true);
            m_IRM.GetResObj(8, 1)->ShowGWnd(true);
            m_IRM.GetResObj(10, 1)->ShowGWnd(true);
            m_IRM.GetResObj(11, 1)->ShowGWnd(true);
            m_IRM.GetResObj(12, 1)->ShowGWnd(true);
            m_IRM.GetResObj(13, 1)->ShowGWnd(true);
            ps->SetGWndSize(303, 121);
            ps->ShowGWnd(true);

            m_IRM.GetResObj(901, 1)->ShowGWnd(false);

            m_IRM.GetResObj(903, 1)->ShowGWnd(false);
            m_IRM.GetResObj(904, 1)->ShowGWnd(false);
            m_IRM.GetResObj(900, 1)->ShowGWnd(false);
            SkillWeaponSlot->ShowGWnd(false);
            SkillShieldSlot->ShowGWnd(false);

            BuffWeaponSlot->ShowGWnd(true);
            BuffShieldSlot->ShowGWnd(true);
            BuffShieldSlot->BringToFront();
            BuffWeaponSlot->BringToFront();

            for (int i = 0; i < 24; i++)
            {
                skillslots[i]->ShowGWnd(false);
                buffslots[i]->ShowGWnd(true);
                buffslots[i]->BringToFront();
            }


        }
            break;
        case 2:
        {
            Clear();
            LoadPartyBuffs();
            ClearPartySlotDDJ();
            LoadPartyBuffInfos();
            for (int i = 0; i< 7; i++)
            {
                m_PartySlot[i]->ShowGWnd(true);
            }
            m_IRM.GetResObj(6, 1)->ShowGWnd(false);
            m_IRM.GetResObj(8, 1)->ShowGWnd(false);
            m_IRM.GetResObj(10, 1)->ShowGWnd(false);
            m_IRM.GetResObj(11, 1)->ShowGWnd(false);
            m_IRM.GetResObj(12, 1)->ShowGWnd(false);
            m_IRM.GetResObj(13, 1)->ShowGWnd(false);
            m_IRM.GetResObj(901, 1)->ShowGWnd(true);

            m_IRM.GetResObj(903, 1)->ShowGWnd(true);
            m_IRM.GetResObj(904, 1)->ShowGWnd(true);
            m_IRM.GetResObj(900, 1)->ShowGWnd(true);
            ps->SetGWndSize(303, 121);
            ps->ShowGWnd(false);

            SkillWeaponSlot->ShowGWnd(false);
            BuffWeaponSlot->ShowGWnd(false);
            SkillShieldSlot->ShowGWnd(false);
            BuffShieldSlot->ShowGWnd(false);

            for (int i = 0; i < 24; i++)
            {
                skillslots[i]->ShowGWnd(false);
                buffslots[i]->ShowGWnd(false);
            }

        }
            break;
    }
}

int CIFMacroMenuAutoSkill::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 17)
            return -1;
    }

    return 100;
}
void CIFMacroMenuAutoSkill::ClearPartySlotDDJ()
{
    for (int i = 0; i < 7; ++i)
    {
        m_PartySlot[i]->ClearDDJ();
    }
    SelectedPartyMemberName = L"";

}

void CIFMacroMenuAutoSkill::Clear()
{
    for (int i = 0; i < 18; ++i)
    {
        m_scroll_skill_list->DeleteItem(m_slotviewer[i]);
        for(int j = 0; j < 8; ++j)
        {
            m_scroll_skill_list->DeleteItem(m_slotviewer[i]->slot1[j]);

        }
    }
}
void CIFMacroMenuAutoSkill::AutoSave(int SlotSeq, byte SlotType, int SlotData)
{
    CMsgStreamBuffer buf(0x188C);
    buf << SlotSeq << SlotType << SlotData;
    SendMsg(buf);
}
void CIFMacroMenuAutoSkill::LoadSkills()
{
    int slotViewerIndex = 0; // slotViewer indexini tutmak iÃ§in kullanÄ±lacak bir deÄŸiÅŸken
    int slotNumber = 0; // slot numarasÄ±nÄ± tutmak iÃ§in kullanÄ±lacak bir deÄŸiÅŸken
    int is = 0;
    int itemCount = 0;
    for (std::map<int, sSkillData*>::iterator it = g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.begin();
         it != g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.end(); ++it)
    {
        if (it->second->m_pSkillData->Target_Required == 1 && it->second->m_pSkillData->TargetType_Animal == 1
            && it->second->m_pSkillData->TargetGroup_Self == 0 && it->second->m_pSkillData->TargetEtc_SelectDeadBody == 0
            && it->second->m_pSkillData->TargetGroup_Party == 0)
        {
            if (slotViewerIndex >= 18)
                break;
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->PutSkills(it->first);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->ShowGWnd(true);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->slot->SetDragable(true);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->slot->N595 = 0;
            is++;

            if ((slotNumber + 1) % 8 == 0) {
                slotViewerIndex++;
                slotNumber = 0;
            } else {
                slotNumber++;
            }

            // Her 8 slot tamamlandÄ±ÄŸÄ±nda itemCountâ€™u arttÄ±r
            if (is % 8 == 0) {
                itemCount++;
            }
        }
    }

// Kalan Ã¶ÄŸeler varsa son bir AddItem Ã§aÄŸrÄ±sÄ± daha yap
    if (is % 8 != 0) {
        itemCount++;
    }

// itemCount deÄŸeri kadar AddItem Ã§aÄŸrÄ±sÄ± yap
    for (int i = 0; i < itemCount; ++i) {
        m_scroll_skill_list->AddItem(m_slotviewer[i], 1, 0);
        //  m_slotviewer[i]->ShowGWnd(true);
        m_slotviewer[i]->BringToFront();
    }

}
void CIFMacroMenuAutoSkill::LoadBuffs()
{

    int slotViewerIndex = 0; // slotViewer indexini tutmak iÃ§in kullanÄ±lacak bir deÄŸiÅŸken
    int slotNumber = 0; // slot numarasÄ±nÄ± tutmak iÃ§in kullanÄ±lacak bir deÄŸiÅŸken
    int is = 0;
    int itemCount = 0;
    for (std::map<int, sSkillData*>::iterator it = g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.begin();
         it != g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.end(); ++it)
    {
        if (it->second->m_pSkillData->TargetGroup_Enemy_M != 1 && it->second->m_pSkillData->TargetGroup_Enemy_P != 1
            && it->second->m_pSkillData->Basic_Activity != 0)
        {
            if (slotViewerIndex >= 18)
                break;
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->PutSkills(it->first);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->ShowGWnd(true);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->slot->SetDragable(true);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->slot->N595 = 0;
            is++;

            if ((slotNumber + 1) % 8 == 0) {
                slotViewerIndex++;
                slotNumber = 0;
            } else {
                slotNumber++;
            }

            // Her 8 slot tamamlandÄ±ÄŸÄ±nda itemCountâ€™u arttÄ±r
            if (is % 8 == 0) {
                itemCount++;
            }
        }
    }

// Kalan Ã¶ÄŸeler varsa son bir AddItem Ã§aÄŸrÄ±sÄ± daha yap
    if (is % 8 != 0) {
        itemCount++;
    }

// itemCount deÄŸeri kadar AddItem Ã§aÄŸrÄ±sÄ± yap
    for (int i = 0; i < itemCount; ++i) {
        m_scroll_skill_list->AddItem(m_slotviewer[i], 1, 0);
        // m_slotviewer[i]->ShowGWnd(true);
        m_slotviewer[i]->BringToFront();  }

}
void CIFMacroMenuAutoSkill::LoadPartyBuffs()
{
    int slotViewerIndex = 0; // slotViewer indexini tutmak iÃ§in kullanÄ±lacak bir deÄŸiÅŸken
    int slotNumber = 0; // slot numarasÄ±nÄ± tutmak iÃ§in kullanÄ±lacak bir deÄŸiÅŸken
    int is = 0;
    int itemCount = 0;
    for (std::map<int, sSkillData*>::iterator it = g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.begin();
         it != g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.end(); ++it)
    {
        if (it->second->m_pSkillData->TargetGroup_Enemy_M != 1 && it->second->m_pSkillData->TargetGroup_Enemy_P != 1
            && it->second->m_pSkillData->Basic_Activity != 0 && it->second->m_pSkillData->TargetGroup_Party == 1)
        {
            if (slotViewerIndex >= 18)
                break;
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->PutSkills(it->first);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->ShowGWnd(true);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->slot->SetDragable(false);
            m_slotviewer[slotViewerIndex]->slot1[slotNumber]->slot->N595 = 2;

            is++;

            if ((slotNumber + 1) % 8 == 0) {
                slotViewerIndex++;
                slotNumber = 0;
            } else {
                slotNumber++;
            }

            // Her 8 slot tamamlandÄ±ÄŸÄ±nda itemCountâ€™u arttÄ±r
            if (is % 8 == 0) {
                itemCount++;
            }
        }
    }

// Kalan Ã¶ÄŸeler varsa son bir AddItem Ã§aÄŸrÄ±sÄ± daha yap
    if (is % 8 != 0) {
        itemCount++;
    }
    // itemCount deÄŸeri kadar AddItem Ã§aÄŸrÄ±sÄ± yap
    for (int i = 0; i < itemCount; ++i) {
        m_scroll_skill_list->AddItem(m_slotviewer[i], 1, 0);
        // m_slotviewer[i]->ShowGWnd(true);
        m_slotviewer[i]->BringToFront();
    }
}
void CIFMacroMenuAutoSkill::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2 - 50) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
void CIFMacroMenuAutoSkill::SaveButton(){
    char settingDirectory[MAX_PATH];
    if (!KmtFormatPath(settingDirectory, sizeof(settingDirectory), "%s\\Setting", theApp.GetWorkingDir()))
        return;
    CreateDirectoryA(settingDirectory, NULL);

    char buffer3[0x200];
    if (!KmtFormatPath(buffer3, sizeof(buffer3), "%s\\Setting\\%ls_MacroAutoBuffSettings.txt", theApp.GetWorkingDir(), g_pMyPlayerObj->GetCharName().c_str()))
        return;

// DosyayÄ± yazma modunda aÃ§
    FILE *file = fopen(buffer3, "w");
    if (file != NULL) {
        std::set<int> seenIDs; // Tekrar eden ID'leri izlemek iÃ§in kÃ¼me oluÅŸtur

        std::map<std::n_wstring, std::vector<int>>::iterator it;
        for (it = PartyBuffList.begin(); it != PartyBuffList.end(); ++it) {
            // Anahtar ve deÄŸerleri dosyaya yaz
            std::fwprintf(file, L"%ls: ", it->first.c_str());
            std::vector<int>& values = it->second;

            std::set<int> uniqueValues; // Tekrar etmeyen deÄŸerler iÃ§in kÃ¼me oluÅŸtur
            for (size_t i = 0; i < values.size(); ++i) {
                if (uniqueValues.find(values[i]) == uniqueValues.end()) {
                    uniqueValues.insert(values[i]);
                    std::fwprintf(file, L"%d ", values[i]);
                }
            }
            std::fwprintf(file, L"\n");

            seenIDs.insert(uniqueValues.begin(), uniqueValues.end());
        }
        std::fclose(file); // DosyayÄ± kapat
    }
}
void CIFMacroMenuAutoSkill::CancelButton(){
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->ShowGWnd(false);
}

bool CIFMacroMenuAutoSkill::IsAutoSkillRuntimeReady()
{
    if (!Macro_AutoSkill || !g_pCGInterface || !g_pMyPlayerObj)
        return false;

    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead ||
        g_pMyPlayerObj->CHARACTER_STATUS == Stall ||
        g_pMyPlayerObj->CHARACTER_STATUS == SkillCast ||
        g_pMyPlayerObj->CHARACTER_STATUS == 0 ||
        g_pMyPlayerObj->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet == 17)
        return false;

    CIFMainPopup* popup = g_pCGInterface->GetMainPopup();
    if (!popup || !popup->GetInventory() || !popup->GetEquipment() || !popup->GetSkill())
        return false;

    return true;
}

const SItemData* CIFMacroMenuAutoSkill::GetEquippedItemData(byte slot)
{
    if (!g_pCGInterface)
        return 0;

    CIFMainPopup* popup = g_pCGInterface->GetMainPopup();
    if (!popup || !popup->GetEquipment())
        return 0;

    CSOItem* item = popup->GetEquipment()->GetEquipmentObjectBySlot(slot);
    if (!item)
        return 0;

    return item->GetItemData();
}

void CIFMacroMenuAutoSkill::ResetSelectedTarget()
{
    SelectObj = 0;
    SelectObjUniqueId = 0;
}

CICharactor* CIFMacroMenuAutoSkill::ResolveSelectedTarget()
{
    if (SelectObjUniqueId == 0)
    {
        SelectObj = 0;
        return 0;
    }

    CICharactor* target = GetCharacterObjectByID_MAYBE(SelectObjUniqueId);
    if (!target || target->GetUniqueId() == 0 || target->CHARACTER_STATUS == 0x2 || target->CHARACTER_STATUS == 0x18)
    {
        ResetSelectedTarget();
        return 0;
    }

    SelectObj = target;
    return target;
}

void CIFMacroMenuAutoSkill::SetSelectedTarget(CICharactor* target)
{
    if (!target)
    {
        ResetSelectedTarget();
        return;
    }

    SelectObj = target;
    SelectObjUniqueId = target->GetUniqueId();
}

bool CIFMacroMenuAutoSkill::CheckBuffWeaponSlot(int Type1, int Type2)
{
    const SItemData* configuredWeapon = GetMacroWeaponSlotData(BuffWeaponSlot);
    if(configuredWeapon == NULL)
        return false;

    if (configuredWeapon->m_typeId.getTypeID1() == 3 &&
        configuredWeapon->m_typeId.getTypeID2() == 1 &&
        configuredWeapon->m_typeId.getTypeID3() == 6 &&
        (configuredWeapon->m_typeId.getTypeID4() == Type1 ||
         configuredWeapon->m_typeId.getTypeID4() == Type2))
    {
        return TryWearMacroSlot(BuffWeaponSlot, GetEquippedItemData(6));
    }

    return false;
}
bool CIFMacroMenuAutoSkill::CheckSkillWeaponAndWear(int Type1, int Type2)
{
    const SItemData* configuredWeapon = GetMacroWeaponSlotData(SkillWeaponSlot);
    if(configuredWeapon == NULL)
        return false;

    if (configuredWeapon->m_typeId.getTypeID1() == 3 &&
        configuredWeapon->m_typeId.getTypeID2() == 1 &&
        configuredWeapon->m_typeId.getTypeID3() == 6 &&
        (configuredWeapon->m_typeId.getTypeID4() == Type1 ||
         configuredWeapon->m_typeId.getTypeID4() == Type2))
    {
        return TryWearMacroSlot(SkillWeaponSlot, GetEquippedItemData(6));
    }
    return false;
}


bool CIFMacroMenuAutoSkill::CheckSkillShieldCondition()
{
    if(SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo != NULL && SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL && SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 3
               || SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 2
               || SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 7
               ||  SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 10
               ||  SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 15)
            {
                if(SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                {
                    if (SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3 &&
                        SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1 &&
                        SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 4)
                    {
                        return TryWearMacroSlot(SkillShieldSlot, GetEquippedItemData(7));
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
        }
    }
    return false;
}
bool CIFMacroMenuAutoSkill::CheckBuffShieldSlot()
{
    const SItemData* configuredShield = GetMacroWeaponSlotData(BuffShieldSlot);
    if(configuredShield != NULL &&
       configuredShield->m_typeId.getTypeID1() == 3 &&
       configuredShield->m_typeId.getTypeID2() == 1 &&
       configuredShield->m_typeId.getTypeID3() == 4)
    {
        return TryWearMacroSlot(BuffShieldSlot, GetEquippedItemData(7));
    }

    return false;
}
bool CIFMacroMenuAutoSkill::CheckBuffShieldCondition()
{
    if(BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 3
               || BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 2
               || BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 7
               ||  BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 10
               ||  BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 15)
            {
                if(BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                {
                    if (BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3 &&
                        BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1 &&
                        BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 4)
                    {
                        return TryWearMacroSlot(BuffShieldSlot, GetEquippedItemData(7));
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
        }
    }


    return false;
}
bool CIFMacroMenuAutoSkill::CheckSkillWeaponSlots()
{
    if(!IsAutoSkillRuntimeReady())
        return false;

    if(!SkillWeaponSlot || !SkillWeaponSlot->m_pMySlot || !SkillWeaponSlot->m_pMySlot->m_pSlot ||
       SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo == 0x0)
        return false;

    const SItemData* configuredWeapon = SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
    if(configuredWeapon == NULL)
        return false;

    const SItemData* equippedWeapon = GetEquippedItemData(6);
    const SItemData* equippedShield = GetEquippedItemData(7);

    const bool needsShield = IsShieldCompatibleMacroWeapon(configuredWeapon);

    if(!TryWearMacroSlot(SkillWeaponSlot, equippedWeapon))
        return false;

    if (!TryRestoreMacroAmmo(configuredWeapon, GetEquippedItemData(7)))
        return false;

    if(needsShield && SkillShieldSlot && SkillShieldSlot->m_pMySlot && SkillShieldSlot->m_pMySlot->m_pSlot &&
       SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo != 0x0)
    {
        const SItemData* configuredShield = SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        if(configuredShield != NULL &&
           configuredShield->m_typeId.getTypeID1() == 3 &&
           configuredShield->m_typeId.getTypeID2() == 1 &&
           configuredShield->m_typeId.getTypeID3() == 4 &&
           !TryWearMacroSlot(SkillShieldSlot, equippedShield))
        {
            return false;
        }
    }

    return true;
}
D3DXVECTOR3 CIFMacroMenuAutoSkill::ConvertToGlobalCoordinates(const uregion& region, const D3DXVECTOR3& localCoords) {
    float globalX, globalY;

    if (region.single.x == 0) {  // Assuming dungeon
        globalX = localCoords.x / 10.0f;
        globalY = localCoords.y / 10.0f;
    } else {  // World map
        globalX = (region.single.x - 135) * 192 + localCoords.x / 10.0f;
        globalY = (region.single.y - 92) * 192 + localCoords.y / 10.0f;
    }

    return D3DXVECTOR3(globalX, globalY, localCoords.z);  // Assuming Z coordinate remains unchanged
}
D3DVECTOR CIFMacroMenuAutoSkill::generateRandomPositionWithinDistance(const D3DVECTOR& center, float minDistance, float maxDistance) {
    const float PI = 3.14159265358979323846f;  // Pi sayÄ±sÄ±

    float scaledMinDistance = minDistance;
    float scaledMaxDistance = maxDistance;

    D3DVECTOR randomPosition;
    float distance;

    do {
        // Rastgele bir aÃ§Ä± ve mesafe Ã¼ret
        float theta = ((float)std::rand() / RAND_MAX) * 2 * PI;  // 0 ile 2Ï€ arasÄ±nda bir aÃ§Ä±
        float phi = ((float)std::rand() / RAND_MAX) * PI;       // 0 ile Ï€ arasÄ±nda bir aÃ§Ä±
        float radius = scaledMinDistance + ((float)std::rand() / RAND_MAX) * (scaledMaxDistance - scaledMinDistance);

        // KÃ¼resel koordinatlarÄ± kartesyen koordinatlara Ã§evir
        float randomX = radius * std::sin(phi) * std::cos(theta);
        float randomY = radius * std::sin(phi) * std::sin(theta);
        float randomZ = radius * std::cos(phi);

        randomPosition.x = center.x + randomX;
        randomPosition.y = center.y + randomY;
        randomPosition.z = center.z + randomZ;

        distance = CalculateDistance(center, randomPosition);
    } while (distance < scaledMinDistance || distance > scaledMaxDistance);

    return randomPosition;
}
float CIFMacroMenuAutoSkill::CalculateDistance(const D3DXVECTOR3& loc1, const D3DXVECTOR3& loc2) {
    D3DXVECTOR3 delta = loc1 - loc2;
    return D3DXVec3Length(&delta);
}

int CIFMacroMenuAutoSkill::FindNeededBuff()
{
    int aaa = -1;
    for (int i = 0; i < 24; i++)
    {
        if (buffslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 73 && buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex() != 0)
        {
            CSkillData* configuredBuff =
                g_CGlobalDataManager->GetSkillData(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex());
            if (!configuredBuff)
                continue;

            // An empty buff list is a valid state and means every configured
            // buff is currently missing.
            if (g_pMyPlayerObj)
            {
                if (!g_pMyPlayerObj->TargetIsBuffInUse(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex(), (DWORD32)g_pMyPlayerObj)
                    && !g_pMyPlayerObj->CheckMagOverlap(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex(), (DWORD32)g_pMyPlayerObj)
                    && !g_pMyPlayerObj->CheckPhyOverlap(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex(), (DWORD32)g_pMyPlayerObj)
                    && !g_pMyPlayerObj->CheckSpeedOverlap(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex(), (DWORD32)g_pMyPlayerObj))
                {
                    if (configuredBuff->Param8 == 1919250793)
                    {
                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex());
                        if (cooldown != 0)
                            continue;

                        // Shield-only buffs still need a one-handed weapon first;
                        // attempting to wear the shield over a bow is rejected.
                        const SItemData* configuredBuffWeapon = GetMacroWeaponSlotData(BuffWeaponSlot);
                        const SItemData* equippedBuffWeapon = GetEquippedItemData(6);
                        if (configuredBuffWeapon && IsShieldCompatibleMacroWeapon(configuredBuffWeapon) &&
                            TryWearMacroSlot(BuffWeaponSlot, equippedBuffWeapon) &&
                            CheckBuffShieldSlot())
                        {
                            if (configuredBuff->TargetGroup_Self == 1)
                            {
                                if(g_pCGInterface->Get_SelectedObjectId() != g_pMyPlayerObj->GetUniqueId())
                                {
                                    g_pCGInterface->Set_SelectedObjectId(g_pMyPlayerObj->GetUniqueId());
                                }
                            }
                            aaa = i;
                            return aaa;
                        }
                    }

                    else if(configuredBuff->ReqCast_Weapon1 == 255)
                    {
                        if (configuredBuff->TargetGroup_Self == 1)
                        {
                            //g_pCGInterface->Set_SelectedObjectId(this->UniqueID());
                            if(g_pCGInterface->Get_SelectedObjectId() != g_pMyPlayerObj->GetUniqueId())
                            {
                                g_pCGInterface->Set_SelectedObjectId(g_pMyPlayerObj->GetUniqueId());
                            }
                        }
                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex());
                        if (cooldown == 0)
                        {
                            aaa = i;
                            return aaa;
                        }
                    }
                    else if (configuredBuff->ReqCast_Weapon1 != 255)
                    {
                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex());
                        if (cooldown != 0)
                            continue;

                        int WeaponCon1 = configuredBuff->ReqCast_Weapon1;
                        int WeaponCon2 = configuredBuff->ReqCast_Weapon2;
                        if(CheckBuffWeaponSlot(WeaponCon1, WeaponCon2))
                        {
                            CheckBuffShieldCondition();
                            if (configuredBuff->TargetGroup_Self == 1)
                            {
                                //g_pCGInterface->Set_SelectedObjectId(this->UniqueID());
                                if(g_pCGInterface->Get_SelectedObjectId() != g_pMyPlayerObj->GetUniqueId())
                                {
                                    g_pCGInterface->Set_SelectedObjectId(g_pMyPlayerObj->GetUniqueId());
                                }
                            }
                            aaa = i;
                            return aaa;
                        }
                        else if(CheckSkillWeaponAndWear(WeaponCon1, WeaponCon2))
                        {
                            CheckSkillShieldCondition();
                            if (configuredBuff->TargetGroup_Self == 1)
                            {
                                //g_pCGInterface->Set_SelectedObjectId(this->UniqueID());
                                if(g_pCGInterface->Get_SelectedObjectId() != g_pMyPlayerObj->GetUniqueId())
                                {
                                    g_pCGInterface->Set_SelectedObjectId(g_pMyPlayerObj->GetUniqueId());
                                }
                            }
                            aaa = i;
                            return aaa;
                        }
                    }
                }
            }
        }
    }
    return aaa;
}
bool CIFMacroMenuAutoSkill::SkillVeItemUyumu(tid_t CastWeapon1, tid_t CastWeapon2)
{

    if(SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            TypeId tid = SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId;
            if(tid.getTypeID1() == 3 && tid.getTypeID2() == 1 && tid.getTypeID3() == 6 && (tid.getTypeID4() == CastWeapon1 || tid.getTypeID4() == CastWeapon2))
            {
                return true;
            }
        }
    }
    return false;
}
bool CIFMacroMenuAutoSkill::BuffVeItemUyumu(tid_t CastWeapon1, tid_t CastWeapon2)
{
    if(BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            TypeId tid = BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_typeId;
            if(tid.getTypeID1() == 3 && tid.getTypeID2() == 1 && tid.getTypeID3() == 6 && (tid.getTypeID4() == CastWeapon1 || tid.getTypeID4() == CastWeapon2))
            {
                return true;
            }
        }
    }
    return false;
}
int CIFMacroMenuAutoSkill::FindAttackSkillSlot()
{
    for (int i = 0; i < 24; i++)
    {
        if (!skillslots[i] || !skillslots[i]->m_pMySlot ||
            !skillslots[i]->m_pMySlot->m_pSlot ||
            skillslots[i]->m_pMySlot->m_pSlot->GetSlotType() != 73)
            continue;

        const int skillId = skillslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex();
        if (skillId == 0)
            continue;

        CSkillData* skillData = g_CGlobalDataManager->GetSkillData(skillId);
        if (!skillData)
            continue;

        if (g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(skillId) != 0)
            continue;

        if (skillData->ReqCast_Weapon1 == 255)
            return i;

        const SItemData* equippedWeapon = GetEquippedItemData(6);
        const SItemData* configuredWeapon =
            SkillWeaponSlot && SkillWeaponSlot->m_pMySlot &&
            SkillWeaponSlot->m_pMySlot->m_pSlot &&
            SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo
                ? SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()
                : 0;

        if (equippedWeapon && configuredWeapon &&
            equippedWeapon->RefObjectId == configuredWeapon->RefObjectId &&
            SkillVeItemUyumu(skillData->ReqCast_Weapon1, skillData->ReqCast_Weapon2))
            return i;
    }
    return -1;
}

void CIFMacroMenuAutoSkill::StartAutoSkill()
{
    if (!AutoSkillTimerRunning)
    {
        AutoSkillTimerRunning = true;
        g_pCGInterface->StartTimer(START_AUTO_SKILL, 250);
    }

    if (!IsAutoSkillRuntimeReady())
    {
        ResetSelectedTarget();
        return;
    }

    if (!IsMacroActionReady())
        return;

    if(AutoSkillTimerRunning)
    {
        int NeededBufSlot = FindNeededBuff();
        if (IsMacroEquipPending())
            return;

        if(NeededBufSlot != -1)
        {
            TryUseMacroSkillSlot(buffslots[NeededBufSlot]);
            return;
        }
        else
        {
            CIFMacroMenuAutoHunt * AutoHunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
            if(AutoHunt->Macro_AutoHunt)
            {
                if(!AutoHunt->DONT_ATTACK_MONSTERCB->GetCheckedState_MAYBE())
                {
                    FindMonsterViaAttackAndPartyBuff();
                }
                else
                {
                    if(AutoHunt->AutoResPtMembersCB->GetCheckedState_MAYBE())
                    {
                        if(AutoResurrrectPartyMember() == 0)
                        {
                            ///TODO GO PT BUFF
                            JustPartyBuff();
                        }
                    }
                    else
                    {
                        JustPartyBuff();
                    }
                }
            }
            else
            {
                // JustAutoAttack();
            }
        }
    }
}
float Distance(D3DVECTOR startAirportPos, D3DVECTOR endAirportPos)
{
    float dis = sqrt(pow(startAirportPos.x - endAirportPos.x, 2) + pow(startAirportPos.y - endAirportPos.y, 2) + pow(startAirportPos.z - endAirportPos.z, 2));
    return fabs(dis);
}

static bool IsMacroUniqueMonster(CIObject* object)
{
    if (!object || !object->IsSame(GFX_RUNTIME_CLASS(CICMonster)))
        return false;

    CICMonster* monster = (CICMonster*)object;
    const SCommonData* commonData = monster->GetCommonData();
    if (!commonData)
        return false;

    return commonData->Rarity == 3 || commonData->Rarity == 8;
}

static CICharactor* FindUniqueMacroTarget(CIFMacroMenuAutoHunt* hunt)
{
    if (!hunt || !hunt->IsUniqueTargetEnabled() || !g_pMyPlayerObj || !g_pGfxEttManager)
        return 0;

    const float radius = hunt->AutoHuntSetting[RADUIS_SETTING];
    CICharactor* selectedUnique = 0;
    float selectedDistance = 0.0f;

    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin(); it != entities.end(); ++it)
    {
        if (!IsMacroUniqueMonster(it->second))
            continue;

        CICharactor* uniqueMonster = (CICharactor*)it->second;
        if (uniqueMonster->GetUniqueId() == 0 ||
            uniqueMonster->CHARACTER_STATUS == Dead ||
            uniqueMonster->CHARACTER_STATUS == 0x18 ||
            ObjIntersect(g_pMyPlayerObj, uniqueMonster))
        {
            continue;
        }

        D3DVECTOR targetLocation = uniqueMonster->GetLocation();
        GetSilkPos(uniqueMonster->GetRegion(), targetLocation);
        if (Distance(targetLocation, GetSilkPosD3D(hunt->StartRegion, hunt->StartPosition)) > radius + 60)
            continue;

        D3DVECTOR playerLocation = g_pMyPlayerObj->GetLocation();
        GetSilkPos(g_pMyPlayerObj->GetRegion(), playerLocation);
        const float distanceToPlayer = Distance(playerLocation, targetLocation);
        if (!selectedUnique || distanceToPlayer < selectedDistance)
        {
            selectedUnique = uniqueMonster;
            selectedDistance = distanceToPlayer;
        }
    }

    return selectedUnique;
}

void CIFMacroMenuAutoSkill::JustAutoAttack(){
    if (!g_pMyPlayerObj)
        return;
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == SkillCast)
        return;
    if (!IsMacroActionReady())
        return;

    if(g_pCGInterface->Get_SelectedObjectId() != 0)
    {
        SelectObj = ResolveSelectedTarget();
        if(!SelectObj)
            return;

        // Weapon-independent skills remain usable while the configured attack
        // set is restored. Weapon-dependent skills and basic attacks wait for
        // the weapon/offhand sequence to finish.
        const bool skillEquipmentReady = CheckSkillWeaponSlots();
        if (IsMacroEquipPending())
            return;

        int SlotID = FindAttackSkillSlot();
        if(SlotID != -1)
        {
            const int skillId = skillslots[SlotID]->m_pMySlot->m_pSlot->GetSkillSlotInDex();
            CSkillData* skillData = g_CGlobalDataManager->GetSkillData(skillId);
            if (skillData && skillData->ReqCast_Weapon1 != 255 && !skillEquipmentReady)
                return;

            TryUseMacroSkillSlot(skillslots[SlotID]);
        }
        else
        {
            if (GetMacroWeaponSlotData(SkillWeaponSlot) && !skillEquipmentReady)
                return;

            if (!IsMacroActionReady())
                return;

            CMsgStreamBuffer buf(0x7074);
            buf << (byte)1;
            buf << (byte)1;
            buf << (byte)1;
            buf << (unsigned int)SelectObj->GetUniqueId();
            SendMsg(buf);
            MarkMacroAction();
        }
    }


}
time_t lastClickTime55;
int FPatrolDestIndexs;
int FPatrolDestIndex;

void CIFMacroMenuAutoSkill::FindMonsterViaAttackAndPartyBuff(){
    if (!g_pMyPlayerObj)
        return;
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == Stall)
        return;
    CIFMacroMenuAutoHunt* Hunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
    float radius = Hunt->AutoHuntSetting[RADUIS_SETTING];

    /*  }
      if(CheckSkillWeaponSlots())
      {*/

    if(!Hunt->DONT_ATTACK_MONSTERCB->GetCheckedState_MAYBE())
    {
        if(Hunt->AutoResPtMembersCB->GetCheckedState_MAYBE())
        {
            if(AutoResurrrectPartyMember() == 0 && JustPartyBuff() == 0)
            {
                SelectObj = ResolveSelectedTarget();
                if (SelectObj) {
                    D3DVECTOR SelectLocation = SelectObj->GetLocation();
                    GetSilkPos(SelectObj->GetRegion(), SelectLocation);
                    D3DVECTOR PlayerLocation = g_pMyPlayerObj->GetLocation();
                    GetSilkPos(g_pMyPlayerObj->GetRegion(), PlayerLocation);
                    double SelectDis = Distance(PlayerLocation, SelectLocation);
                    short Status = SelectObj->CHARACTER_STATUS;
                    if (SelectObj->GetUniqueId() == 0 || Status == 2 || Status == 0x18 || SelectDis > 500 || ObjIntersect(g_pMyPlayerObj, SelectObj)) {
                        ResetSelectedTarget();
                    }
                    else if (Distance(SelectLocation, GetSilkPosD3D(Hunt->StartRegion, Hunt->StartPosition)) > radius + 60)
                    {
                        ResetSelectedTarget();
                    }
                }
                if (!SelectObj || !IsMacroUniqueMonster(SelectObj)) {
                    CICharactor* uniqueTarget = FindUniqueMacroTarget(Hunt);
                    if (uniqueTarget) {
                        SetSelectedTarget(uniqueTarget);
                        InvalidObjects.clear();
                    }
                }
                if (!SelectObj) {
                    SetSelectedTarget((CICharactor*)GetNearestObj());
                    InvalidObjects.clear();

                }
                if (SelectObj && SelectObj->GetUniqueId() != 0 && SelectObj->CHARACTER_STATUS != 0x2 && SelectObj->CHARACTER_STATUS != 0x18 && g_pCGInterface)
                {
                    if (!TrySelectMacroTarget(SelectObj->GetUniqueId()))
                        return;

                    short Level = *(short*)((DWORD32)SelectObj + 0x7AC);
                    if(Hunt->AutoHuntSetting[ZERK_SETTING] == eZerkSetting::All && Level == 0)
                    {
                        TryUseMacroZerk();
                    }
                    else if(Hunt->AutoHuntSetting[ZERK_SETTING] == eZerkSetting::Giant && Level == 4)
                    {
                        TryUseMacroZerk();
                    }
                    JustAutoAttack();
                }
                else
                {
                    short state = *(short*)((DWORD32)g_pMyPlayerObj + 0x065C);
                    if (g_pMyPlayerObj->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet != 2) {
                        if (radius != 0) {

                            D3DVECTOR CenterLocation = Hunt->StartPosition;
                            uregion PatrolRegion = Hunt->StartRegion;
                            FPatrolDestIndexs++;
                            if (FPatrolDestIndexs > 3) {
                                FPatrolDestIndexs = 0;
                            }
                            int angle = FPatrolDestIndexs * 90;
                            CenterLocation.x = ceil(cos((angle) * (3.1415926 / 180)) * (radius) + CenterLocation.x);
                            CenterLocation.z = ceil(sin((angle) * (3.1415926 / 180)) * (radius) + CenterLocation.z);
                            g_pCGInterface->m_Nav.MoveTo(Hunt->GetRegion(), CenterLocation);


                        }
                        else {
                            if (Distance(g_pMyPlayerObj->GetLocation(), GetSilkPosD3D(Hunt->StartRegion, Hunt->StartPosition)) > 50) {

//                                  SendMessageA(Rebot::GameHWND, WM_MOVECHAR, (DWORD32)&PatrolRegions, (DWORD32)&CenterLocations);
                                g_pCGInterface->m_Nav.MoveTo(Hunt->GetRegion(), Hunt->GetLocation());
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if(JustPartyBuff() == 0)
            {
                SelectObj = ResolveSelectedTarget();
                if (SelectObj != NULL) {
                    D3DVECTOR SelectLocation = SelectObj->GetLocation();
                    GetSilkPos(SelectObj->GetRegion(), SelectLocation);
                    D3DVECTOR PlayerLocation = g_pMyPlayerObj->GetLocation();
                    GetSilkPos(g_pMyPlayerObj->GetRegion(), PlayerLocation);
                    double SelectDis = Distance(PlayerLocation, SelectLocation);
                    short Status = SelectObj->CHARACTER_STATUS;
                    if (SelectObj->GetUniqueId() == 0 || Status == 2 || Status == 0x18 || SelectDis > 500 || ObjIntersect(g_pMyPlayerObj, SelectObj)) {
                        ResetSelectedTarget();
                    }
                    else if (Distance(SelectLocation, GetSilkPosD3D(Hunt->StartRegion, Hunt->StartPosition)) > radius + 60)
                    {
                        ResetSelectedTarget();
                    }
                }
                if (!SelectObj || !IsMacroUniqueMonster(SelectObj)) {
                    CICharactor* uniqueTarget = FindUniqueMacroTarget(Hunt);
                    if (uniqueTarget) {
                        SetSelectedTarget(uniqueTarget);
                        InvalidObjects.clear();
                    }
                }
                if (!SelectObj) {
                    SetSelectedTarget((CICharactor*)GetNearestObj());
                    InvalidObjects.clear();
                }
                if (SelectObj && SelectObj->GetUniqueId() != 0 && SelectObj->CHARACTER_STATUS != 0x2 && SelectObj->CHARACTER_STATUS != 0x18 && g_pCGInterface)
                {
                    if (!TrySelectMacroTarget(SelectObj->GetUniqueId()))
                        return;

                    short Level = *(short*)((DWORD32)SelectObj + 0x7AC);

                    if(Hunt->AutoHuntSetting[ZERK_SETTING] == eZerkSetting::All && Level == 0)
                    {
                        TryUseMacroZerk();
                    }
                    else if(Hunt->AutoHuntSetting[ZERK_SETTING] == eZerkSetting::Giant && Level == 4)
                    {
                        TryUseMacroZerk();
                    }
                    JustAutoAttack();
                }
                else
                {
                    short state = *(short*)((DWORD32)g_pMyPlayerObj + 0x065C);
                    if (g_pMyPlayerObj->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet != 2) {
                        if (radius != 0) {
                            D3DVECTOR CenterLocation = Hunt->StartPosition;
                            uregion PatrolRegion = Hunt->StartRegion;
                        FPatrolDestIndexs++;
                        if (FPatrolDestIndexs > 3) {
                            FPatrolDestIndexs = 0;
                            }
                            int angle = FPatrolDestIndexs * 90;
                            CenterLocation.x = ceil(cos((angle) * (3.1415926 / 180)) * (radius) + CenterLocation.x);
                            CenterLocation.z = ceil(sin((angle) * (3.1415926 / 180)) * (radius) + CenterLocation.z);
                            g_pCGInterface->m_Nav.MoveTo(Hunt->GetRegion(), CenterLocation);

                        }
                        else {
                            if (Distance(g_pMyPlayerObj->GetLocation(), GetSilkPosD3D(Hunt->StartRegion, Hunt->StartPosition)) > 50) {

//                                        SendMessageA(Rebot::GameHWND, WM_MOVECHAR, (DWORD32)&PatrolRegions, (DWORD32)&CenterLocations);
                                g_pCGInterface->m_Nav.MoveTo(Hunt->GetRegion(), Hunt->GetLocation());
                            }
                        }
                    }
                }

            }
        }

    }

}
int CIFMacroMenuAutoSkill::JustPartyBuff()
{
    if(!PartyBuffList.empty())
    {
        CICharactor * SelectedPartyMember = GetNearPartyMemberID();
        if(SelectedPartyMember != 0)
        {
            if(!TrySelectMacroTarget(SelectedPartyMember->GetUniqueId()))
                return 0;

            if(PartyBuffList.find(SelectedPartyMember->GetName().c_str()) != PartyBuffList.end())
            {
                int UnusedSkillID = 0;
                std::vector<int>& skillList = PartyBuffList[SelectedPartyMember->GetName()];
                for (std::vector<int>::iterator skillIt2 = skillList.begin(); skillIt2 != skillList.end(); ++skillIt2) {
                    if(!TargetIsBuffInUse(*skillIt2, (DWORD32)SelectedPartyMember)
                       && !CheckMagOverlap(*skillIt2, (DWORD32)SelectedPartyMember)
                       && !CheckPhyOverlap(*skillIt2, (DWORD32)SelectedPartyMember)
                       && !CheckSameBuffOverlap(*skillIt2, (DWORD32)SelectedPartyMember))
                    {
                        UnusedSkillID = *skillIt2;
                        break;
                    }
                }

                if(UnusedSkillID != 0)
                {
                    int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(UnusedSkillID);
                    if (cooldown == 0 && SelectedPartyMember->CHARACTER_STATUS != Dead)
                    {
                        int WeaponCon1 = g_CGlobalDataManager->GetSkillData(UnusedSkillID)->ReqCast_Weapon1;
                        int WeaponCon2 = g_CGlobalDataManager->GetSkillData(UnusedSkillID)->ReqCast_Weapon2;
                        if(BuffVeItemUyumu(WeaponCon1, WeaponCon2) && CheckBuffWeaponSlot(WeaponCon1, WeaponCon2))
                        {
                            if(!IsMacroActionReady())
                                return 0;

                            CheckBuffShieldCondition();
                            CMsgStreamBuffer buf(0x7074);
                            buf << (byte)1;
                            buf << (byte)4;
                            buf << (unsigned int)UnusedSkillID;
                            buf << (byte)1;
                            buf << (unsigned int)SelectedPartyMember->GetUniqueId();
                            SendMsg(buf);
                            MarkMacroAction();
                            return 1;
                        }

                    }

                }

            }
        }
    }
    return 0;
}
DWORD32 CIFMacroMenuAutoSkill::GetBuffListBase(DWORD32 Addr) {
    if(g_pMyPlayerObj)
        return *(DWORD32*)(Addr + 0x288);
    else
        return 0;
}
boolean CIFMacroMenuAutoSkill::TargetIsBuffInUse(DWORD32 SkillID, DWORD32 Target) {
    DWORD32 dwListBase = GetBuffListBase(Target);
    if(dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0)
                return false;
            if (Base == dwListBase)
                return false;
            if (SkillID == *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C)) {
                return true;
            }
        }
    }
    return false;
}
CICharactor* CIFMacroMenuAutoSkill::GetNearPartyMemberID() {
    {
        if (!g_pMyPlayerObj || !g_pGfxEttManager || !g_pCGInterface)
            return 0;

        CIFMacroMenu* macroMenu =
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
        if (!macroMenu || !macroMenu->AutoHuntSlot)
            return 0;

        const float partyRadius =
            (float)macroMenu->AutoHuntSlot->AutoHuntSetting[RADUIS_SETTING];
        const SPartyData& currentParty = g_CCharacterDependentData.GetPartyData();

        for (std::map<int, CIObject*>::iterator candidateIt =
                 g_pGfxEttManager->entities.begin();
             candidateIt != g_pGfxEttManager->entities.end(); ++candidateIt)
        {
            CIObject* object = candidateIt->second;
            if (!object || strcmp(object->GetRuntimeClass()->m_lpszClassName, "CICUser"))
                continue;

            CICharactor* candidate = (CICharactor*)object;
            if (!candidate || candidate->GetUniqueId() == 0 ||
                candidate->CHARACTER_STATUS == Dead)
                continue;

            bool isPartyMember = false;
            for (int partyIndex = 0; partyIndex < currentParty.NumberOfMembers; ++partyIndex)
            {
                if (g_CCharacterDependentData.GetPartyMemberData(partyIndex).m_charactername ==
                    candidate->GetName())
                {
                    isPartyMember = true;
                    break;
                }
            }
            if (!isPartyMember)
                continue;

            std::map<std::n_wstring, std::vector<int> >::iterator configured =
                PartyBuffList.find(candidate->GetName());
            if (configured == PartyBuffList.end())
                continue;

            bool needsBuff = false;
            for (std::vector<int>::const_iterator skillIt = configured->second.begin();
                 skillIt != configured->second.end(); ++skillIt)
            {
                if (!TargetIsBuffInUse(*skillIt, (DWORD32)candidate) &&
                    !CheckMagOverlap(*skillIt, (DWORD32)candidate) &&
                    !CheckPhyOverlap(*skillIt, (DWORD32)candidate) &&
                    !CheckSameBuffOverlap(*skillIt, (DWORD32)candidate))
                {
                    needsBuff = true;
                    break;
                }
            }
            if (!needsBuff)
                continue;

            D3DXVECTOR3 candidateGlobal =
                ConvertToGlobalCoordinates(candidate->GetRegion(), candidate->GetLocation());
            D3DXVECTOR3 playerGlobal =
                ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), g_pMyPlayerObj->GetLocation());
            if (CalculateDistance(candidateGlobal, playerGlobal) / 10.0f > partyRadius + 10.0f)
                continue;

            LocationInfo playerInfo = {0};
            playerInfo.region.raw = g_pMyPlayerObj->GetRegion().r;
            playerInfo.pos = g_pMyPlayerObj->GetLocation();
            playerInfo.field_0 = g_pMyPlayerObj->GetNavCell();
            playerInfo.field_1 = g_pMyPlayerObj->m_object_under_foot;

            LocationInfo candidateInfo = {0};
            candidateInfo.region.raw = candidate->GetRegion().r;
            candidateInfo.pos = candidate->GetLocation();
            candidateInfo.field_0 = candidate->GetNavCell();
            candidateInfo.field_1 = candidate->m_object_under_foot;

            if (!g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(
                    1, 1, &playerInfo, &candidateInfo, 0, candidate))
                return candidate;
        }
        return 0;
    }

    CIFMacroMenuAutoHunt * AutoHuntSlot = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
    D3DXVECTOR3 location2 = g_pMyPlayerObj->GetLocation();
    float radius = AutoHuntSlot->AutoHuntSetting[RADUIS_SETTING];

    D3DXVECTOR3 locationtarget = AutoHuntSlot->GetLocation();
    D3DXVECTOR3 locationmine = g_pMyPlayerObj->GetLocation();

// Convert local coordinates to global coordinates
    D3DXVECTOR3 globalTarget = ConvertToGlobalCoordinates(AutoHuntSlot->GetRegion(), locationtarget);
    D3DXVECTOR3 globalMine = ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), locationmine);


    float distance = CalculateDistance(globalTarget, globalMine);
    if(distance / 10 < radius + 10)
    {
        if(g_pMyPlayerObj->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet != 2)
        {
            LocationInfo l1;
            l1.region.raw = g_pMyPlayerObj->GetRegion().r;
            l1.pos = g_pMyPlayerObj->GetLocation();
            l1.field_0 = g_pMyPlayerObj->GetNavCell();
            l1.field_1 = g_pMyPlayerObj->m_object_under_foot;

            LocationInfo l2;
            l2.region.raw = AutoHuntSlot->GetRegion().r;
            l2.pos = AutoHuntSlot->GetLocation();

            if(!g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(1, 1, &l1, &l2, 0, g_pMyPlayerObj))
            {
                g_pCGInterface->m_Nav.MoveTo(AutoHuntSlot->GetRegion(), AutoHuntSlot->GetLocation());
            }
        }
    }

    unsigned int FoundedUserUniqueID = 0;
    std::n_wstring FoundedCharName;
    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin(); it != entities.end(); ++it) {
        if (!strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICUser")) {
            CICUser* nearmember = (CICUser*)it->second;
            if (nearmember)
            {
                if (nearmember->CHARACTER_STATUS != Dead)
                {
                    FoundedUserUniqueID = nearmember->GetUniqueId();
                    FoundedCharName = nearmember->GetName();
                    break;
                }
            }
        }
    }
    bool isinparty = false;

    if (FoundedUserUniqueID != 0) {

        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        if (partyData.NumberOfMembers > 0)
        {
            for (int i = 0; i < partyData.NumberOfMembers; ++i) {
                const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
                if(memberData.m_charactername == FoundedCharName)
                {
                    isinparty = true;
                    break;
                }
            }
        }
        if(isinparty)
        {
            CICharactor* nearpartyuser = GetCharacterObjectByID_MAYBE(FoundedUserUniqueID);
            if(nearpartyuser->CHARACTER_STATUS != Dead)
            {
                if(nearpartyuser->IsChinese() || nearpartyuser->IsEurope())
                {
                    if(PartyBuffList.find(nearpartyuser->GetName().c_str()) != PartyBuffList.end())
                    {
                        int UnusedSkillID = 0;
                        std::vector<int>& skillList = PartyBuffList[nearpartyuser->GetName()];
                        for (std::vector<int>::iterator skillIt2 = skillList.begin(); skillIt2 != skillList.end(); ++skillIt2) {
                            if(!TargetIsBuffInUse(*skillIt2, (DWORD32)nearpartyuser)
                               && !CheckMagOverlap(*skillIt2, (DWORD32)nearpartyuser)
                               && !CheckPhyOverlap(*skillIt2, (DWORD32)nearpartyuser)
                               && !CheckSameBuffOverlap(*skillIt2, (DWORD32)nearpartyuser))
                            {
                                UnusedSkillID = *skillIt2;
                                break;
                            }
                        }
                        if(UnusedSkillID != 0)
                        {
                            CICharactor* pUser = GetCharacterObjectByID_MAYBE(nearpartyuser->GetUniqueId());
                            // Get local coordinates from your objects
                            D3DXVECTOR3 locationtarget = pUser->GetLocation();
                            D3DXVECTOR3 locationmine = g_pMyPlayerObj->GetLocation();

// Convert local coordinates to global coordinates
                            D3DXVECTOR3 globalTarget = ConvertToGlobalCoordinates(pUser->GetRegion(), locationtarget);
                            D3DXVECTOR3 globalMine = ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), locationmine);


                            float distance = CalculateDistance(globalTarget, globalMine);
                            if (distance / 10 <= radius + 10)
                            {
                                if (pUser && pUser->CHARACTER_STATUS != Dead) {
                                    LocationInfo l1;
                                    l1.region.raw = g_pMyPlayerObj->GetRegion().r;
                                    l1.pos = g_pMyPlayerObj->GetLocation();
                                    l1.field_0 = g_pMyPlayerObj->GetNavCell();
                                    l1.field_1 = g_pMyPlayerObj->m_object_under_foot;

                                    LocationInfo l2;
                                    l2.region.raw = nearpartyuser->GetRegion().r;
                                    l2.pos = nearpartyuser->GetLocation();
                                    l2.field_0 = nearpartyuser->GetNavCell();
                                    l2.field_1 = nearpartyuser->m_object_under_foot;
                                    if(!g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(1, 1, &l1, &l2, 0, nearpartyuser))
                                        return pUser;
                                }
                            }
                        }

                    }
                }
            }
        }
    }
    return 0;
}

CIObject* CIFMacroMenuAutoSkill::GetNearestObj() {
    CIFMacroMenuAutoHunt* Hunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
    float radius = Hunt->AutoHuntSetting[RADUIS_SETTING];
    radius = radius;
    CICharactor* uniqueTarget = FindUniqueMacroTarget(Hunt);
    if (uniqueTarget)
        return uniqueTarget;

    CIObject* TempObj = 0;
    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin();
         it != entities.end(); ++it) {
        if (!strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICMonster") ||
            !strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICUser")) {
            short Status = ((CICharactor*)it->second)->CHARACTER_STATUS;//0x2 or 0x18

            short Level = *(short*)((DWORD32)it->second + 0x7AC);//0 Normal 1 = champion 3 rare 4 Giant
            if (std::count(InvalidObjects.begin(), InvalidObjects.end(), it->second)) {
                continue;
            }

            if (((CICharactor*)it->second)->GetUniqueId() == 0 || Status == 0x18 || Status == 2 || ObjIntersect(g_pMyPlayerObj, it->second)) {
                continue;
            }

            D3DVECTOR TagLocation = it->second->GetLocation();
            GetSilkPos(it->second->GetRegion(), TagLocation);
            //if the distance between starting pos , monster greater than range , just ignore it
            if (Distance(TagLocation, GetSilkPosD3D(Hunt->StartRegion, Hunt->StartPosition)) > radius + 60) {
                continue;
            }

            bool AttackOtherPlayer = false;
            if (TempObj == 0) {
                if (!strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICUser")) {
                    if ((((CICharactor*)it->second)->fonttexture_playername.m_color_fg == 0xFFFF58FE || ((CICharactor*)it->second)->fonttexture_playername.m_color_fg == 0xFFFF6262) && AttackOtherPlayer) {
                        TempObj = it->second;
                    }
                    else {
                        continue;
                    }

                }
                TempObj = it->second;
            }

            D3DVECTOR MyLocation = g_pMyPlayerObj->GetLocation();
            GetSilkPos(g_pMyPlayerObj->GetRegion(), MyLocation);
            D3DVECTOR SelectLocation = TempObj->GetLocation();
            GetSilkPos(TempObj->GetRegion(), SelectLocation);

            double TagDis = Distance(MyLocation, TagLocation);
            double SelectDis = Distance(MyLocation, SelectLocation);
            //give attacking priority type ?
            if ((((CICharactor*)it->second)->fonttexture_playername.m_color_fg == 0xFFFF58EE || ((CICharactor*)it->second)->fonttexture_playername.m_color_fg == 0xFFFF6262) && TagDis < 500 && AttackOtherPlayer) {
                //normal player
                TempObj = it->second;
            }
            else if (Level != 0 && Level != 1 && TagDis < 500 && !strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICMonster")) {
                TempObj = it->second;

            }
            else if (TagDis < SelectDis && !strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICMonster")) {
                TempObj = it->second;
            }
        }
    }
    return TempObj;
}
CICMonster* CIFMacroMenuAutoSkill::GetNearMonsterID() {

    CICMonster* pSelectedMob = NULL;

    CIFMacroMenuAutoHunt* Hunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
    float radius = Hunt->AutoHuntSetting[RADUIS_SETTING];

    D3DXVECTOR3 locationmine = g_pMyPlayerObj->GetLocation();

// Convert local coordinates to global coordinates
    D3DXVECTOR3 StartLocation = ConvertToGlobalCoordinates(Hunt->GetRegion(), Hunt->GetLocation());
    D3DXVECTOR3 CurrentLocationForBack   = ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), locationmine);

    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin(); it != entities.end(); ++it)
    {
        if (it->second->IsSame(GFX_RUNTIME_CLASS(CICMonster)))
        {
            CICMonster* nearmonster = (CICMonster*)it->second;

            if (!nearmonster)
            {
                continue;
            }
            if (nearmonster->CHARACTER_STATUS == Dead && nearmonster->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet == 0)
            {
                continue;
            }


            D3DXVECTOR3 TargetMobLocation = ConvertToGlobalCoordinates(it->second->GetRegion(), it->second->GetLocation());
            D3DXVECTOR3 CurrentLocation = ConvertToGlobalCoordinates(Hunt->GetRegion(), Hunt->GetLocation());

            float distanceForMob = CalculateDistance(TargetMobLocation, CurrentLocation);
            // CanavarÄ±n bÃ¶lge ve konum bilgilerini al
            if ((distanceForMob / 10) > (radius + 40))
            {
                continue;
            }

            short Level = *(short*)((DWORD32)nearmonster + 0x7AC);//0 Normal 1 = champion 3 rare 4 Giant
            if(Hunt->AutoHuntSetting[ZERK_SETTING] == eZerkSetting::All && Level == 0)
            {
                TryUseMacroZerk();
            }
            else if(Hunt->AutoHuntSetting[ZERK_SETTING] == eZerkSetting::Giant && Level == 4)
            {
                TryUseMacroZerk();
            }

            pSelectedMob = nearmonster;
            return pSelectedMob;
        }
    }

/*
    int ID = 0;
    CIFMacroMenuAutoHunt* Hunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
    float radius = Hunt->AutoHuntSetting[RADUIS_SETTING];

    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin();
         it != entities.end(); ++it) {
        if (!strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICMonster") ||
            !strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICUser")) {
            //            int HP = ((CICharactor*)it->second)->HP;
            //            int MAXHP = ((CICharactor*)it->second)->MAXHP;
            short Status = ((CICharactor*)it->second)->CHARACTER_STATUS;//0x2 or 0x18

//            wchar_t* HPT = new wchar_t[100];
//            wsprintfW(HPT, L"<%d/%d>", HP, MAXHP);
//            ((CICharactor*)it->second)->ShowMessageAboveEntity(HPT,0xFFFFFF);

            short Level = *(short*)((DWORD32)it->second + 0x7AC);//0 Normal 1 = champion 3 rare 4 Giant


            if (((CICharactor*)it->second)->GetUniqueId() == 0 || Status == 0x18 || Status == 2 || ObjIntersect(g_pMyPlayerObj, it->second)) {
                continue;
            }

            D3DVECTOR TagLocation = it->second->GetLocation();
            GetSilkPos(it->second->GetRegion(), TagLocation);
            //if the distance between starting pos , monster greater than range , just ignore it

            const double distanceFromHuntCenter = Distance(TagLocation, GetSilkPosD3D(Hunt->GetRegion(), Hunt->GetLocation()));
            if (distanceFromHuntCenter / 10 > radius) {
                continue;
            }


            D3DVECTOR MyLocation = g_pMyPlayerObj->GetLocation();
            D3DVECTOR SelectLocation = it->second->GetLocation();
            GetSilkPos(it->second->GetRegion(), SelectLocation);

            double TagDis = Distance(MyLocation, TagLocation);
            double SelectDis = Distance(MyLocation, SelectLocation);
            //give attacking priority type ?

            if (Level != 0 && Level != 1 && TagDis < 500 && !strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICMonster")) {
                ID = ((CICharactor*)it->second)->GetUniqueId();

            }
            else if (TagDis < SelectDis && !strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICMonster")) {
                ID = ((CICharactor*)it->second)->GetUniqueId();
            }
        }
    }*/
    return NULL;
}
int CIFMacroMenuAutoSkill::IHaveResurrectSkill(byte Level)
{
    for (std::map<int, sSkillData*>::iterator it = g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.begin();
         it != g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_SkillData.end(); ++it)
    {
        if(g_CGlobalDataManager->GetSkillData(it->second->m_skillId) != NULL)
        {
            if(g_CGlobalDataManager->GetSkillData(it->second->m_skillId)->TargetEtc_SelectDeadBody == 1 &&
               g_CGlobalDataManager->GetSkillData(it->second->m_skillId)->Param2 != 1835229552
               && (g_CGlobalDataManager->GetSkillData(it->second->m_skillId)->Param10 >= Level ||
                   g_CGlobalDataManager->GetSkillData(it->second->m_skillId)->Param8 >= Level))
            {
                return it->first;
            }
        }
    }

    return 0;
}

int CIFMacroMenuAutoSkill::AutoResurrrectPartyMember()
{
    time_t now = time(NULL);  // GÃ¼ncel zamanÄ± al
    double elapsed = difftime(now, MyLastResUseTime);

    if(elapsed <  30)
        return 0;// 30 saniye geÃ§miÅŸ mi kontrol et
    CICharactor * SelectedPartyMember = GetNearPartyDeadMemberID();
    if(SelectedPartyMember != 0)
    {
        if(!TrySelectMacroTarget(SelectedPartyMember->GetUniqueId()))
            return 0;

        byte TargetLevel = 0;
        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        if (partyData.NumberOfMembers > 0)
        {
            for (int i = 0; i < partyData.NumberOfMembers; ++i) {
                const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
                if(memberData.m_charactername == SelectedPartyMember->GetName())
                {
                    TargetLevel = memberData.currentLevel;
                    break;
                }
            }
        }
        if(TargetLevel != 0)
        {
            int ResSkillID = IHaveResurrectSkill(TargetLevel);
            if(ResSkillID!= 0)
            {
                int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(ResSkillID);
                if(cooldown == 0)
                {
                    if (SelectedPartyMember->CHARACTER_STATUS == Dead)
                    {
                        if (g_CGlobalDataManager->GetSkillData(ResSkillID)->Param8 == 1919250793)
                        {
                            if(CheckBuffShieldCondition())
                            {
                                if(!IsMacroActionReady())
                                    return 0;

                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte)1;
                                buf << (byte)4;
                                buf << (unsigned int)ResSkillID;
                                buf << (byte)1;
                                buf << (unsigned int)SelectedPartyMember->GetUniqueId();
                                SendMsg(buf);
                                MarkMacroAction();
                                MyLastResUseTime = time(NULL);
                                return 1;
                            }
                        }
                        else if(g_CGlobalDataManager->GetSkillData(ResSkillID)->ReqCast_Weapon1 == 255)
                        {
                            if(!IsMacroActionReady())
                                return 0;

                            CMsgStreamBuffer buf(0x7074);
                            buf << (byte)1;
                            buf << (byte)4;
                            buf << (unsigned int)ResSkillID;
                            buf << (byte)1;
                            buf << (unsigned int)SelectedPartyMember->GetUniqueId();
                            SendMsg(buf);
                            MarkMacroAction();
                            MyLastResUseTime = time(NULL);
                            return 1;
                        }
                        else if (g_CGlobalDataManager->GetSkillData(ResSkillID)->ReqCast_Weapon1 != 255)
                        {

                            int WeaponCon1 = g_CGlobalDataManager->GetSkillData(ResSkillID)->ReqCast_Weapon1;
                            int WeaponCon2 = g_CGlobalDataManager->GetSkillData(ResSkillID)->ReqCast_Weapon2;
                            if(CheckBuffWeaponSlot(WeaponCon1, WeaponCon2))
                            {
                                CheckBuffShieldCondition();
                                if(!IsMacroActionReady())
                                    return 0;

                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte)1;
                                buf << (byte)4;
                                buf << (unsigned int)ResSkillID;
                                buf << (byte)1;
                                buf << (unsigned int)SelectedPartyMember->GetUniqueId();
                                SendMsg(buf);
                                MarkMacroAction();
                                MyLastResUseTime = time(NULL);
                                return 1;
                            }
                            else if(CheckSkillWeaponAndWear(WeaponCon1, WeaponCon2))
                            {
                                CheckSkillShieldCondition();
                                if(!IsMacroActionReady())
                                    return 0;

                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte)1;
                                buf << (byte)4;
                                buf << (unsigned int)ResSkillID;
                                buf << (byte)1;
                                buf << (unsigned int)SelectedPartyMember->GetUniqueId();
                                SendMsg(buf);
                                MarkMacroAction();
                                MyLastResUseTime = time(NULL);
                                return 1;
                            }

                        }
                    }
                }
            }
        }
    }
    return 0;
}
CICharactor* CIFMacroMenuAutoSkill::GetNearPartyDeadMemberID() {
    {
        if (!g_pMyPlayerObj || !g_pGfxEttManager || !g_pCGInterface)
            return 0;

        CIFMacroMenu* macroMenu =
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
        if (!macroMenu || !macroMenu->AutoHuntSlot)
            return 0;

        const float resurrectRadius =
            (float)macroMenu->AutoHuntSlot->AutoHuntSetting[RADUIS_SETTING];
        const SPartyData& currentParty = g_CCharacterDependentData.GetPartyData();

        for (std::map<int, CIObject*>::iterator candidateIt =
                 g_pGfxEttManager->entities.begin();
             candidateIt != g_pGfxEttManager->entities.end(); ++candidateIt)
        {
            CIObject* object = candidateIt->second;
            if (!object || strcmp(object->GetRuntimeClass()->m_lpszClassName, "CICUser"))
                continue;

            CICharactor* candidate = (CICharactor*)object;
            if (!candidate || candidate->GetUniqueId() == 0 ||
                candidate->CHARACTER_STATUS != Dead)
                continue;

            bool isPartyMember = false;
            for (int partyIndex = 0; partyIndex < currentParty.NumberOfMembers; ++partyIndex)
            {
                if (g_CCharacterDependentData.GetPartyMemberData(partyIndex).m_charactername ==
                    candidate->GetName())
                {
                    isPartyMember = true;
                    break;
                }
            }
            if (!isPartyMember)
                continue;

            D3DXVECTOR3 candidateGlobal =
                ConvertToGlobalCoordinates(candidate->GetRegion(), candidate->GetLocation());
            D3DXVECTOR3 playerGlobal =
                ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), g_pMyPlayerObj->GetLocation());
            if (CalculateDistance(candidateGlobal, playerGlobal) / 10.0f > resurrectRadius)
                continue;

            LocationInfo playerInfo = {0};
            playerInfo.region.raw = g_pMyPlayerObj->GetRegion().r;
            playerInfo.pos = g_pMyPlayerObj->GetLocation();
            playerInfo.field_0 = g_pMyPlayerObj->GetNavCell();
            playerInfo.field_1 = g_pMyPlayerObj->m_object_under_foot;

            LocationInfo candidateInfo = {0};
            candidateInfo.region.raw = candidate->GetRegion().r;
            candidateInfo.pos = candidate->GetLocation();
            candidateInfo.field_0 = candidate->GetNavCell();
            candidateInfo.field_1 = candidate->m_object_under_foot;

            if (!g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(
                    1, 1, &playerInfo, &candidateInfo, 0, candidate))
                return candidate;
        }
        return 0;
    }

    CIFMacroMenuAutoHunt* Hunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
    float radius = Hunt->AutoHuntSetting[RADUIS_SETTING];

    unsigned int FoundedUserUniqueID = 0;
    std::n_wstring FoundedCharName;
    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin(); it != entities.end(); ++it) {
        if (!strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICUser")) {
            CICUser* nearmember = (CICUser*)it->second;
            if (nearmember)
            {
                if (nearmember->CHARACTER_STATUS == Dead)
                {
                    FoundedUserUniqueID = nearmember->GetUniqueId();
                    FoundedCharName = nearmember->GetName();
                    break;
                }
            }
        }
    }
    bool isinparty = false;

    if (FoundedUserUniqueID != 0) {

        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        if (partyData.NumberOfMembers > 0)
        {
            for (int i = 0; i < partyData.NumberOfMembers; ++i) {
                const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
                if(memberData.m_charactername == FoundedCharName)
                {
                    isinparty = true;
                    break;
                }
            }
        }

        if(isinparty)
        {
            CICharactor* nearpartyuser = GetCharacterObjectByID_MAYBE(FoundedUserUniqueID);
            if(nearpartyuser->CHARACTER_STATUS == Dead)
            {
                if(nearpartyuser->IsChinese() || nearpartyuser->IsEurope())
                {
                    CICharactor* pUser = GetCharacterObjectByID_MAYBE(nearpartyuser->GetUniqueId());

                    D3DXVECTOR3 globalTarget = ConvertToGlobalCoordinates(pUser->GetRegion(), pUser->GetLocation());
                    D3DXVECTOR3 globalMine = ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), g_pMyPlayerObj->GetLocation());

                    float distance = CalculateDistance(globalTarget, globalMine);
                    if (distance /10 <= radius)
                    {
                        if (pUser && pUser->CHARACTER_STATUS == Dead) {
                            LocationInfo l1;
                            l1.region.raw = g_pMyPlayerObj->GetRegion().r;
                            l1.pos = g_pMyPlayerObj->GetLocation();
                            l1.field_0 = g_pMyPlayerObj->GetNavCell();
                            l1.field_1 = g_pMyPlayerObj->m_object_under_foot;

                            LocationInfo l2;
                            l2.region.raw = nearpartyuser->GetRegion().r;
                            l2.pos = nearpartyuser->GetLocation();
                            l2.field_0 = nearpartyuser->GetNavCell();
                            l2.field_1 = nearpartyuser->m_object_under_foot;
                            if(!g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(1, 1, &l1, &l2, 0, nearpartyuser))
                                return pUser;
                        }
                    }
                }

            }
        }
    }
    return 0;
}

boolean CIFMacroMenuAutoSkill::CheckSameBuffOverlap(DWORD32 SkillID, DWORD32 Target)
{
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0 || Base == dwListBase)
            {
                break;
            }
            DWORD32 currentSkillID = *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C);
            if (g_CGlobalDataManager->GetSkillData(currentSkillID) != NULL && g_CGlobalDataManager->GetSkillData(SkillID) != NULL)
            {
                int currentSkillOverlap = g_CGlobalDataManager->GetSkillData(currentSkillID)->Action_Overlap;
                int targetSkillOverlap = g_CGlobalDataManager->GetSkillData(SkillID)->Action_Overlap;
                // Mag overlap kontrolÃ¼
                if (targetSkillOverlap == currentSkillOverlap)
                {
                    return true;
                }
            }
        }
    }
    return false;
}
boolean CIFMacroMenuAutoSkill::CheckMagOverlap(DWORD32 SkillID, DWORD32 Target)
{
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0 || Base == dwListBase)
            {
                break;
            }
            DWORD32 currentSkillID = *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C);
            if (g_CGlobalDataManager->GetSkillData(currentSkillID) != NULL && g_CGlobalDataManager->GetSkillData(SkillID) != NULL)
            {
                int currentSkillOverlap = g_CGlobalDataManager->GetSkillData(currentSkillID)->Action_Overlap;
                int targetSkillOverlap = g_CGlobalDataManager->GetSkillData(SkillID)->Action_Overlap;

                int ChFireMagDef = 3;
                int EuDeityMagDef = 1744830467;

                // Mag overlap kontrolÃ¼
                if ((targetSkillOverlap == ChFireMagDef && currentSkillOverlap == EuDeityMagDef) ||
                    (targetSkillOverlap == EuDeityMagDef && currentSkillOverlap == ChFireMagDef))
                {

                    return true;
                }
            }
        }
    }
    return false;
}

boolean CIFMacroMenuAutoSkill::CheckPhyOverlap(DWORD32 SkillID, DWORD32 Target)
{
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0 || Base == dwListBase)
            {
                break;
            }
            DWORD32 currentSkillID = *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C);
            if (g_CGlobalDataManager->GetSkillData(currentSkillID) != NULL && g_CGlobalDataManager->GetSkillData(SkillID) != NULL)
            {
                int currentSkillOverlap = g_CGlobalDataManager->GetSkillData(currentSkillID)->Action_Overlap;
                int targetSkillOverlap = g_CGlobalDataManager->GetSkillData(SkillID)->Action_Overlap;

                int ChColdPhyDef = 2;
                int EuDeityPhyDef = 1728053250;


                // Phy overlap kontrolÃ¼
                if ((targetSkillOverlap == ChColdPhyDef && currentSkillOverlap == EuDeityPhyDef) ||
                    (targetSkillOverlap == EuDeityPhyDef && currentSkillOverlap == ChColdPhyDef))
                {

                    return true;
                }
            }
        }
    }
    return false;
}
