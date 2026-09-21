////////////////////////////////////////////////////////////////////////////////
//
// GInterfaceWndRelation.cpp: implementation of the CGInterface wnd realtion functions.
//
////////////////////////////////////////////////////////////////////////////////

#include <CustomData/CustomSettingManager.h>
#include <SecondBar/IFExtQuickSlotSecond.h>
#include <SecondBar/IFExtQuickSlotCustom.h>
#include <Macro/IFMacroMenu.h>
#include <Menu/IFMenu.h>
#include <GFXMainFrame/Controler.h>
#include "GInterface.h"
#include <WebViewer/WebViewerConfig.h>

#include "GlobalDataManager.h"

#include "Game.h"

#include "support/MemberFunctionHook.h"

HOOK_ORIGINAL_MEMBER(0x0079D5B0, &CGInterface::ToggleActionWnd);
void CGInterface::ToggleActionWnd() {
    // If MainPopup is visible and page 'action' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_ACTION)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
    } else {
        m_mainPopup->ShowGWnd(true);
        m_mainPopup->ShowSubPage(GDR_ACTION);
        FUN_0079a7e0(m_mainPopup);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079B0B0, &CGInterface::ToggleApprenticeshipWnd);
void CGInterface::ToggleApprenticeshipWnd() {
    // If MainPopup is visible and page 'apprenticeship' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_APPRENTICESHIP)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
        g_CGlobalDataManager->FUN_008cbac0(GDR_APPRENTICESHIP);
    } else {
        m_mainPopup->ShowGWnd(true);
        if (!m_mainPopup->GetApprenticeShip()->IsVisible()) {
            m_mainPopup->ShowSubPage(GDR_APPRENTICESHIP);
        }
        FUN_0079a7e0(m_mainPopup);
        g_CGlobalDataManager->FUN_008cbac0(GDR_APPRENTICESHIP);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079ACE0, &CGInterface::TogglePlayerInfoWnd);
void CGInterface::TogglePlayerInfoWnd() {
    // If MainPopup is visible and page 'playerinfo' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_PLAYERINFO)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
    } else {
        m_mainPopup->ShowGWnd(true);
        if (!m_mainPopup->GetPlayerInfo()->IsVisible()) {
            m_mainPopup->ShowSubPage(GDR_PLAYERINFO);
        }
        FUN_0079a7e0(m_mainPopup);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079f690, &CGInterface::ToggleGameGuide);
void CGInterface::ToggleGameGuide() {
    if (m_IRM.GetResObj(GDR_GAMEGUIDE, true) == NULL) {
        ToggleGGuide(true);
        FUN_0079a7e0(m_IRM.GetResObj(GDR_GAMEGUIDE, true));
        return;
    }

    ToggleGGuide(false);
    FUN_0079a7e0(m_IRM.GetResObj(GDR_SYSTEM, true));
}

void CGInterface::ToggleGGuide(bool bVisible) {
    CIFWnd *pGGuide = m_IRM.GetResObj(GDR_GAMEGUIDE, true);
    if (!bVisible) {
        if (pGGuide == NULL)
            return;

        SaveGWndPostion(GDR_GAMEGUIDE, pGGuide->GetPos());

        pGGuide->ShowGWnd(false);
        pGGuide->GetParentControl()->SetFocus_MAYBE();
        FUN_0079a7e0(pGGuide);

        m_IRM.DeleteCreatedSection("GameGuide");
        return;
    }

    if (pGGuide != NULL) {
        pGGuide->ShowGWnd(true);
        return;
    }

    m_IRM.CreateInterfaceSection("GameGuide", this);

    pGGuide = m_IRM.GetResObj(GDR_GAMEGUIDE, true);
    pGGuide->MoveGWnd(230, 80);// why do i saving the pos if im setting it by my self? lol
    pGGuide->ShowGWnd(true);
    FUN_0079a7e0(pGGuide);
}

HOOK_ORIGINAL_MEMBER(0x0079B020, &CGInterface::ToggleInventoryWnd);
void CGInterface::ToggleInventoryWnd() {
    // If MainPopup is visible and page 'inventory' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_INVENTORY)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
    } else {
        m_mainPopup->ShowGWnd(true);
        if (!m_mainPopup->GetInventory()->IsVisible()) {
            m_mainPopup->ShowSubPage(GDR_INVENTORY);
        }
        FUN_0079a7e0(m_mainPopup);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079ad70, &CGInterface::ToggleEquipmentWnd);
void CGInterface::ToggleEquipmentWnd() {
    // If MainPopup is visible and page 'equipment' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_INVENTORY)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
    } else {
        m_mainPopup->ShowGWnd(true);
        if (!m_mainPopup->GetEquipment()->IsVisible()) {
            m_mainPopup->ShowSubPage(GDR_INVENTORY);
        }
        FUN_0079a7e0(m_mainPopup);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079AE90, &CGInterface::TogglePartyWnd);
void CGInterface::TogglePartyWnd() {
    // If MainPopup is visible and page 'party' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_PARTY)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
        g_CGlobalDataManager->FUN_008cbac0(GDR_PARTY);
    } else {
        m_mainPopup->ShowGWnd(true);
        if (!m_mainPopup->GetParty()->IsVisible()) {
            m_mainPopup->ShowSubPage(GDR_PARTY);
        }
        FUN_0079a7e0(m_mainPopup);
        g_CGlobalDataManager->FUN_008cbac0(GDR_PARTY);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079AE00, &CGInterface::ToggleSkillWnd);
void CGInterface::ToggleSkillWnd() {
    // If MainPopup is visible and page 'skill' is active
    if (m_mainPopup->IsVisible() && m_mainPopup->IsSubPageActive(GDR_SKILL)) {
        m_mainPopup->ShowGWnd(false);
        FUN_0079a7e0(m_mainPopup);
    } else {
        m_mainPopup->ShowGWnd(true);
        if (!m_mainPopup->GetSkill()->IsVisible()) {
            m_mainPopup->ShowSubPage(GDR_SKILL);
        }
        FUN_0079a7e0(m_mainPopup);
    }
}

HOOK_ORIGINAL_MEMBER(0x0079af20, &CGInterface::ShowInventoryWnd);
void CGInterface::ShowInventoryWnd() {
    m_mainPopup->ShowGWnd(true);
    if (!m_mainPopup->GetInventory()->IsVisible()) {
        m_mainPopup->ShowSubPage(GDR_INVENTORY);
    }
    FUN_0079a7e0(m_mainPopup);
}

HOOK_ORIGINAL_MEMBER(0x0079af70, &CGInterface::ShowApprenticeshipWnd);
void CGInterface::ShowApprenticeshipWnd() {
    m_mainPopup->ShowGWnd(true);
    if (!m_mainPopup->GetApprenticeShip()->IsVisible()) {
        m_mainPopup->ShowSubPage(GDR_APPRENTICESHIP);
    }
    FUN_0079a7e0(m_mainPopup);
}

void CGInterface::ToggleMainPopup() {
    m_mainPopup->ShowGWnd(!m_mainPopup->IsVisible());
    FUN_0079a7e0(m_mainPopup);
}

HOOK_ORIGINAL_MEMBER(0x0079abe0, &CGInterface::ToggleWorldMap);
void CGInterface::ToggleWorldMap() {
    // Don't we have the window?
    if (GetWorldMap() == NULL) {
        g_sNewInterfaceMgr.InstantiateDimensional("res_ui\\worldmapguide.2dt", this, false);
        SetOnMiddleOfScreen(GDR_WORLDMAP);
        return;
        // Window is created, so let's un/visible it
    } else {
        g_sNewInterfaceMgr.GetInterfaceObj(GDR_WORLDMAP)->OnGWndShow();
        FUN_0079ab80(g_sNewInterfaceMgr.GetInterfaceObj(GDR_WORLDMAP));
        SetOnMiddleOfScreen(GDR_WORLDMAP);
        CIFMenu *menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        if(menu->CanSendPing)
        {
            menu->CanSendPing = false;
            g_Controler->SetCustomCursor(149);
        }
    }
}

HOOK_ORIGINAL_MEMBER(0x0079c750, &CGInterface::ToggleAutoPotion);
void CGInterface::ToggleAutoPotion(bool bVisible) {

    if(m_Settings->EnableMacro)
    {
        if (this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->IsVisible()) {
            this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(false);
            CGEffSoundBody::get()->PlaySound(L"snd_window_close");
        }
        else {
            this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(0);

            this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->ActivateTabPage(0);
            this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->UpdateMenuSize();
            //this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(0);
            this->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(true);
            CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        }
        return;
    }

    CIFWnd *pAutoPotion = m_IRM.GetResObj(GDR_AUTO_POTION, true);

    // If close it up
    if (!bVisible) {
        // If the window is already not there
        if (pAutoPotion == NULL)
            return;

        SaveGWndPostion(GDR_AUTO_POTION, pAutoPotion->GetPos());

        pAutoPotion->ShowGWnd(false);
        m_IRM.DeleteCreatedSection("AutoPotion");
        return;
        // Open it up
    } else {
        // Do we have it already opened before?
        if (pAutoPotion != NULL)
            return;

        m_IRM.CreateInterfaceSection("AutoPotion", this);

        pAutoPotion = m_IRM.GetResObj(GDR_AUTO_POTION, true);
        SaveGWndPosOnMiddleOfScreen(GDR_AUTO_POTION, NULL);
        pAutoPotion->ShowGWnd(true);

        FUN_0079a7e0(pAutoPotion);
    }
}

void CGInterface::CreateFlorian0Event() {
    if(m_Settings->ShowGuideMenu)
        GetAlarmManager()->CreateGuideIcon(GDR_MENU_GUIDE);

    if(m_Settings->ShowGuideLuckySpin && m_Settings->EnableLuckySpin)
        GetAlarmManager()->CreateGuideIcon(GDR_LUCKY_SPIN_GUIDE);

    if(m_Settings->ShowGuideSpecialOffers && m_Settings->EnableSpecialOffers)
        GetAlarmManager()->CreateGuideIcon(GDR_SPECIAL_OFFERS_GUIDE);

    if(m_Settings->ShowGuideKillerAnimation)
        GetAlarmManager()->CreateGuideIcon(GDR_KILLER_ANIMATION_GUIDE);

    // The Event Register guide deliberately follows the first four guide
    // controls, so it occupies the first slot of the second row.
    if(m_Settings->EventRegisterWnd)
        GetAlarmManager()->CreateGuideIcon(GDR_EVENT_REGISTER_GUIDE);

    if(m_Settings->ShowGuideDropLogs)
        GetAlarmManager()->CreateGuideIcon(GDR_DROPLOG_OPEN_ICON);

    if(m_Settings->ShowGuidePvpChallenge && m_Settings->EnablePvpChallenge)
        GetAlarmManager()->CreateGuideIcon(GDR_PVP_CHALLENGE_GUIDE);

    if(m_Settings->ShowGuideItemChest)
        GetAlarmManager()->CreateGuideIcon(GDR_CHEST_GUIDE);

    if(m_Settings->ShowGuideMacro && m_Settings->EnableMacro)
    {
        GetAlarmManager()->CreateGuideIcon(GDR_MACRO_GUIDE);
    }
   // GetAlarmManager()->CreateGuideIcon(GDR_ALCHEM_MACRO_GUIDE);
   /* this->m_IRM.CreateInterfaceSection("Battery", this);
    this->m_IRM.CreateInterfaceSection("BatteryNum", this);

    this->m_IRM.GetResObj(201, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj(202, 1)->ShowGWnd(true);
*/

    if(m_Settings->ShowGuideDailyLogin)
        GetAlarmManager()->CreateGuideIcon(GDR_DAILY_LOGIN);

    if(m_Settings->ShowGuideDiscord)
        GetAlarmManager()->CreateGuideIcon(GDR_DISCORD_GUIDE);

    if(m_Settings->ShowGuideWebsite)
        GetAlarmManager()->CreateGuideIcon(GDR_WEBS_GUIDE);

    if(m_Settings->ShowGuideFacebook)
        GetAlarmManager()->CreateGuideIcon(GDR_FACEBOOK_GUIDE);

    if(m_Settings->ShowGuideAutoEquip)
        GetAlarmManager()->CreateGuideIcon(GDR_AUTOEQUIP_GUIDE);

    if(m_Settings->ShowGuideWebViewer && CWebViewerConfig::Load()) {
        int webViewerCount = CWebViewerConfig::GetCount();
        int i;
        for(i = 0; i < webViewerCount; ++i) {
            GetAlarmManager()->CreateGuideIcon(WEBVIEWER_GUIDE_BASE_ID + i);
        }
    }

    wnd_rect sz;
    sz.pos.x = 0;
    sz.pos.y = 0;
    sz.size.width = 32;
    sz.size.height = 32;
    CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, JobIconID, 0);
    // GetAlarmManager()->CreateGuideIcon(GDR_WEB_GUIDE);
    CMsgStreamBuffer buf(0xA400);
    SendMsg(buf);
}

CAlramGuideMgrWnd *CGInterface::GetAlarmManager() {
    return m_IRM.GetResObj<CAlramGuideMgrWnd>(GDR_ALRAM_GUIDE_MGR_WND, 1);
}

CIF_NPCWindow *CGInterface::Get_GDR_NPCWINDOW() {
    return (CIF_NPCWindow *) this->m_IRM.GetResObj(GDR_NPCWINDOW, 1);
}

HOOK_ORIGINAL_MEMBER(0x00798D00, &CGInterface::GetMainPopup);
CIFMainPopup *CGInterface::GetMainPopup() {
    return (CIFMainPopup *) this->m_IRM.GetResObj(GDR_MAINPOPUP, 1);
}

CIFStorageRoom *CGInterface::Get_GDR_STORAGEROOM() {
    return (CIFStorageRoom *) this->m_IRM.GetResObj(GDR_STORAGEROOM, 1);
}

CIFStorageRoom *CGInterface::Get_GDR_GUILDSTORAGEROOM() {
    return (CIFStorageRoom *) this->m_IRM.GetResObj(GDR_GUILDSTORAGEROOM, 1);
}

void CGInterface::ToggleQuestNew() {
    reinterpret_cast<void(__thiscall *)(CGInterface *)>(0x007990e0)(this);
}

CNIFCommunityWnd *CGInterface::GetCommunityWnd() {
    return g_sNewInterfaceMgr.GetInterfaceObj<CNIFGuildWnd>(GDR_GUILD)->m_pCommunity;
}

CIFSystemMessage *CGInterface::GetSystemMessageView() {
    return m_IRM.GetResObj<CIFSystemMessage>(GDR_SYSTEM_MESSAGE_VIEW, 1);
}

void CGInterface::FUN_0079a7e0(CGWndBase *pGWnd) const {
    reinterpret_cast<void(__thiscall *)(const CGInterface *, CGWndBase *)>(0x0079a7e0)(this, pGWnd);
}

void CGInterface::FUN_0079ab80(CNIFWnd *pWnd) {
    reinterpret_cast<void(__thiscall *)(const CGInterface *, CNIFWnd *)>(0x0079ab80)(this, pWnd);
}

void CGInterface::FUN_0079b8a0(undefined1 a1, undefined4 a2) {
    reinterpret_cast<void(__thiscall *)(CGInterface *, undefined1, undefined4)>(0x0079b8a0)(this, a1, a2);
}

CNIFUnderMenuBar *CGInterface::GetUnderMenuBar() {
    return reinterpret_cast<CNIFUnderMenuBar *(__thiscall *) (CGInterface *)>(0x007994e0)(this);
}

CIFExtQuickSlot *CGInterface::GetExtQuickSlot() {
    return m_IRM.GetResObj<CIFExtQuickSlot>(GDR_EXT_QUICK_SLOT, 1);
}

void CGInterface::FUN_00777a70(undefined4 param_1, undefined4 param_2) {
    //printf("%d %d \n", param_1, param_2);
    reinterpret_cast<void(__thiscall *)(CGInterface *, undefined4, undefined4)>(0x00777a70)(this, param_1, param_2);
}

CNIFWorldMap *CGInterface::GetWorldMap() {
    return g_sNewInterfaceMgr.GetInterfaceObj<CNIFWorldMap>(GDR_WORLDMAP);
}

void CGInterface::SetOnMiddleOfScreen(DWORD dwNInterfaceID) {
    CNIFWnd *pWnd = g_sNewInterfaceMgr.GetInterfaceObj(dwNInterfaceID);
    if (pWnd == NULL)
        return;

    wnd_pos sNewPos = {(theApp.GetClientDimensionStuff().width / 2) - (pWnd->GetBounds().size.width / 2),
                       (theApp.GetClientDimensionStuff().height / 2) - (pWnd->GetBounds().size.height / 2)};
    pWnd->MoveGWnd2(sNewPos);
}

void CGInterface::SaveGWndPostion(DWORD dwID, CGWnd::wnd_pos sWndPos) {
    reinterpret_cast<void(__thiscall *)(CGInterface *, DWORD, CGWnd::wnd_pos)>(0x0079a840)(this, dwID, sWndPos);
}
void CGInterface::Fun_00777f00() /// this is loading slt
{

    if(m_Settings->SecondarySlot)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlot>(GDR_EXT_QUICK_SLOT, 1)->ShowGWnd(false);
        for (int i = 0; i < 50; i++) {
            if(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i] != NULL)
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
        }
    }
    if(m_Settings->SecondarySlot)
    {
        for (int i = 0; i < 40; i++) {
            if(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(1363, 1)->myslots[i] != NULL)
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(1363, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
        }
    }
    if(m_Settings->EnableMacro)
    {
        for (int i = 0; i < 12; i++) {
            if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i] != NULL)
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ClearSlot();
        }

        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot != NULL)
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ClearSlot();

        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot != NULL)
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ClearSlot();


        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot != NULL)
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ClearSlot();

        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot != NULL)
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ClearSlot();


        for(int i = 0; i < 24; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i] != NULL)
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot->ClearSlot();

            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i] != NULL)
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot->ClearSlot();


        }
        for(int i = 0; i < 8; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i] != NULL)
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ClearSlot();

            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i] != NULL)
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ClearSlot();


        }

    }
    //CMsgStreamBuffer buf(0xA400);
    //buf << (byte)1;
    //SendMsg(buf);
    reinterpret_cast<void(__thiscall*)(CGInterface*)>(0x00777f00)(this);


    /* for()
     g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MyMacroAutoPotionData.insert(std::make_pair(SlotSeq, Data));
     g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->UpdateSlot(SlotSeq, SlotType, Data, Active, Value);
 */
}
void CGInterface::SaveGWndPosOnMiddleOfScreen(DWORD dwID, CGWnd *pParent) {
    CGWnd *pParentWnd = pParent;
    if (pParentWnd == NULL)
        pParentWnd = g_pCGInterface;

    CIFWnd *pWnd = (CIFWnd *) pParentWnd->GetChildControl(dwID);
    if (pWnd == NULL)
        return;

    wnd_pos sNewPos = {(theApp.GetClientDimensionStuff().width / 2) - (pWnd->GetBounds().size.width / 2),
                       (theApp.GetClientDimensionStuff().height / 2) - (pWnd->GetBounds().size.height / 2)};
    pWnd->MoveGWnd2(sNewPos);

    SaveGWndPostion(dwID, sNewPos);
}
