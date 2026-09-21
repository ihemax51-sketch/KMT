#include "IFMacro.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <ICPlayer.h>
#include <Macro/IFMacroMenu.h>


#define GDR_AUTO_POT_OPTION_BTN  6
#define GDR_AUTO_POT_ON_OFF_BTN 7

#define GDR_MACRO_AUTO_HUNT_ON_OFF_BTN 100
#define GDR_MACRO_AUTO_SKILL_ON_OFF_BTN 101
#define GDR_MACRO_PET_FILTER_ON_OFF_BTN 102
#define GDR_MACRO_AUTO_SCROLL_ON_OFF_BTN 103

#define GDR_MACRO_AUTO_POTION_CHECKBOX 255
#define GDR_MACRO_AUTO_SKILL_CHECKBOX 256
#define GDR_MACRO_AUTO_HUNT_CHECKBOX 257
#define GDR_MACRO_PET_FILTER_CHECKBOX 258
#define GDR_MACRO_AUTO_SCROLL_CHECKBOX 259

#define GDR_MACRO_AUTO_SKILL_OPT 10

#define GDR_MACRO_AUTO_HUNT_OPT 14

#define GDR_MACRO_PET_FILTER_OPT 18



#define GDR_MACRO_AUTO_SCROLL_OPT 21


GFX_IMPLEMENT_DYNCREATE(CIFMacro, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFMacro, CIFMainFrame)
                    ONG_COMMAND(GDR_AUTO_POT_ON_OFF_BTN, &AutoPotionOnOffBtn)
                    ONG_COMMAND(GDR_AUTO_POT_OPTION_BTN, &AutoPotionOptionBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_SKILL_OPT, &AutoSkillOptionBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_SKILL_ON_OFF_BTN, &AutoSkillOnOffBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_SKILL_OPT, &AutoSkillOptionBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_SKILL_ON_OFF_BTN, &AutoSkillOnOffBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_HUNT_ON_OFF_BTN, &AutoHuntOnOffBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_HUNT_OPT, &AutoHuntOptionBtn)

                    ONG_COMMAND(GDR_MACRO_PET_FILTER_OPT, &PetFilterOptionBtn)
                    ONG_COMMAND(GDR_MACRO_PET_FILTER_ON_OFF_BTN, &PetFilterOnOffBtn)

                    ONG_COMMAND(GDR_MACRO_AUTO_SCROLL_OPT, &AutoScrollOptionBtn)
                    ONG_COMMAND(GDR_MACRO_AUTO_SCROLL_ON_OFF_BTN, &AutoScrollOnOffBtn)


GFX_END_MESSAGE_MAP()

CIFMacro::CIFMacro(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
    autopotiononoffbtn = 0;
    autoskillonoffbtn = 0;
    autohuntonoffbtn = 0;
    petfilteronoffbtn = 0;
    autoscrollonoffbtn = 0;


}
CIFMacro::~CIFMacro(void) {

    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFMacro::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacro.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_SRO_MACRO"));
    wnd_rect sz;
    /*  sz.pos.x= 139;
      sz.pos.y = 61;
      sz.size.width = 33;
      sz.size.height = 32;
      autopotiononoffbtn = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea), sz, GDR_AUTO_POT_ON_OFF_BTN, 0);
      autopotiononoffbtn->TB_Func_13("interface\\sguardmedia\\macro\\macro_icon_potion_off.ddj", 0, 0);
  */
    this->m_IRM.GetResObj(21, 1)->SetText(KmtGetText(L"UIIT_KMT_OPTIONS"));

    this->m_IRM.GetResObj(18, 1)->SetText(KmtGetText(L"UIIT_KMT_OPTIONS"));
    this->m_IRM.GetResObj(14, 1)->SetText(KmtGetText(L"UIIT_KMT_OPTIONS"));

    this->m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_OPTIONS"));
    this->m_IRM.GetResObj(6, 1)->SetText(KmtGetText(L"UIIT_KMT_OPTIONS"));


    this->m_IRM.GetResObj(4, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_POTION"));
    this->m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_SKILL_62A8C749"));
    this->m_IRM.GetResObj(16, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_PICK_UP"));
    this->m_IRM.GetResObj(12, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_HUNTING"));
    this->m_IRM.GetResObj(19, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_SCROLL"));
    sz.pos.x= 139;
    sz.pos.y = 61;
    sz.size.width = 33;
    sz.size.height = 32;
    autopotiononoffbtn = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea), sz, GDR_AUTO_POT_ON_OFF_BTN, 0);
    autopotiononoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_potion_off.ddj", 0, 0);

    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->ShowGWnd(false);


    sz.pos.x= 139;
    sz.pos.y = 121;
    sz.size.width = 33;
    sz.size.height = 32;
    autoskillonoffbtn = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea), sz, GDR_MACRO_AUTO_SKILL_ON_OFF_BTN, 0);
    autoskillonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_skill_off.ddj", 0, 0);


    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->ShowGWnd(false);

    sz.pos.x= 139;
    sz.pos.y = 181;
    sz.size.width = 33;
    sz.size.height = 32;
    autohuntonoffbtn = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea), sz, GDR_MACRO_AUTO_HUNT_ON_OFF_BTN, 0);
    autohuntonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_hunting_off.ddj", 0, 0);

    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->ShowGWnd(false);



    sz.pos.x= 139;
    sz.pos.y = 241;
    sz.size.width = 33;
    sz.size.height = 32;
    petfilteronoffbtn = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea), sz, GDR_MACRO_PET_FILTER_ON_OFF_BTN, 0);
    petfilteronoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_item_off.ddj", 0, 0);

    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->ShowGWnd(false);


    sz.pos.x= 139;
    sz.pos.y = 301;
    sz.size.width = 33;
    sz.size.height = 32;
    autoscrollonoffbtn = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea), sz, GDR_MACRO_AUTO_SCROLL_ON_OFF_BTN, 0);
    autoscrollonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_scroll_off.ddj", 0, 0);

    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->ShowGWnd(false);



    this->ShowGWnd(false);

    return true;
}

void CIFMacro::OnUpdate()
{

}


void CIFMacro::UpdateMenuSize()
{
    if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->Macro_AutoPotion)
    {
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->SetCheckBoxState(true);
        autopotiononoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_potion_on.ddj", 0, 0);
    }

    if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->Macro_AutoSkill)
    {
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->SetCheckBoxState(true);
        autoskillonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_skill_on.ddj", 0, 0);
    }

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2 - 50) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFMacro::AutoPotionOnOffBtn(){

    if(this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->GetCheckedState_MAYBE())
    {

        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->SetCheckBoxState(false);
        autopotiononoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_potion_off.ddj", 0, 0);
        CIFMacroMenuAutoPotion* autoPotion =
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot;
        autoPotion->Macro_AutoPotion = false;
        autoPotion->StopAutomation();
        SendPacket();
    }
    else
    {
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->SetCheckBoxState(true);
        autopotiononoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_potion_on.ddj", 0, 0);
        CIFMacroMenuAutoPotion* autoPotion =
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot;
        autoPotion->Macro_AutoPotion = true;
        autoPotion->StartAutomation();
        SendPacket();
    }

}
void CIFMacro::ActiveAutoPotion()
{
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->SetCheckBoxState(true);
    autopotiononoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_potion_on.ddj", 0, 0);
    CIFMacroMenuAutoPotion* autoPotion =
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot;
    autoPotion->Macro_AutoPotion = true;
    autoPotion->StartAutomation();
}
void CIFMacro::AutoPotionOptionBtn(){


    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(0);

    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->ActivateTabPage(0);
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    wnd_pos r;
    r =  g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->GetPos();
    this->MoveGWnd(r.x - 313, r.y);
}


void CIFMacro::AutoSkillOnOffBtn(){
    if(this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->GetCheckedState_MAYBE())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->Macro_AutoSkill = false;
        if( g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->AutoSkillTimerRunning)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->AutoSkillTimerRunning = false;
            g_pCGInterface->KillTimer(START_AUTO_SKILL);
        }
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->SetCheckBoxState(false);
        autoskillonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_skill_off.ddj", 0, 0);
        SendPacket();
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->Macro_AutoSkill = true;
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->StartAutoSkill();
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->SetCheckBoxState(true);
        autoskillonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_skill_on.ddj", 0, 0);
        SendPacket();

    }
}
void CIFMacro::GDR_MACRO_AUTO_HUNT_CHECKBOX_STATE(bool b)
{
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->SetCheckBoxState(b);
}
void CIFMacro::ActiveAutoSkill()
{
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->Macro_AutoSkill = true;
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->StartAutoSkill();
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->SetCheckBoxState(true);
    autoskillonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_skill_on.ddj", 0, 0);
}
void CIFMacro::AutoSkillOptionBtn()
{
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(1);

    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->ActivateTabPage(0);
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    wnd_pos r;
    r =  g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->GetPos();
    this->MoveGWnd(r.x - 313, r.y);
}

void CIFMacro::AutoHuntOnOffBtn()
{
    if(this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->GetCheckedState_MAYBE())
    {
        CIFMacroMenuAutoHunt * Macrohunt = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
        Macrohunt->Macro_AutoHunt = false;

        if(Macrohunt->MacroAutoInviteRunning)
        {
            Macrohunt->MacroAutoInviteRunning = false;
            g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        }
        if(Macrohunt->MacroAutoTownTimerRunning)
        {
            Macrohunt->MacroAutoTownTimerRunning = false;
            g_pCGInterface->KillTimer(START_BACK_TOWN);
        }
        if(Macrohunt->AutoHuntTimerRunning)
        {
            Macrohunt->AutoHuntTimerRunning = false;
            g_pCGInterface->KillTimer(START_AUTO_HUNT);
        }
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->SetCheckBoxState(false);
        autohuntonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_hunting_off.ddj", 0, 0);
        SendPacket();
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->Macro_AutoHunt = true;
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->StartRegion = g_pMyPlayerObj->GetRegion();
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->StartPosition = g_pMyPlayerObj->GetLocation();

        D3DVECTOR CenterPos = g_pMyPlayerObj->GetLocation();
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->StartPosition = CenterPos;
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->ResetSelectedTarget();
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->StartAutoHunt();

        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->SetCheckBoxState(true);
        autohuntonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_hunting_on.ddj", 0, 0);

        SendPacket();
    }
}
void CIFMacro::AutoHuntOptionBtn()
{
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(2);

    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->ActivateTabPage(0);
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    wnd_pos r;
    r =  g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->GetPos();
    this->MoveGWnd(r.x - 313, r.y);
}

void CIFMacro::ActiveAutoHunt()
{
    CIFMacroMenu* menu = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
    if (!menu || !menu->AutoHuntSlot || !menu->AutoSkillSlot || !g_pMyPlayerObj)
        return;

    menu->AutoHuntSlot->Macro_AutoHunt = true;
    menu->AutoHuntSlot->StartRegion.r = 0;
    menu->AutoHuntSlot->StartPosition.x = 0.0f;
    menu->AutoHuntSlot->StartPosition.y = 0.0f;
    menu->AutoHuntSlot->StartPosition.z = 0.0f;
    menu->AutoSkillSlot->ResetSelectedTarget();
    menu->AutoHuntSlot->StartAutoHunt();

    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->SetCheckBoxState(true);
    autohuntonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_hunting_on.ddj", 0, 0);
}
void CIFMacro::PetFilterOptionBtn()
{
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(true);

    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(3);

    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    wnd_pos r;
    r =   g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->GetPos();
    this->MoveGWnd(r.x - 313, r.y);
}

void CIFMacro::PetFilterOnOffBtn(){
    if(this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->GetCheckedState_MAYBE())
    {

        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->SetCheckBoxState(false);
        petfilteronoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_item_off.ddj", 0, 0);
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->Macro_PetFilter = false;
        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->PetPickTimerIsRunning)
        {
            g_pCGInterface->KillTimer(START_PICK_PET_TIMER);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->PetPickTimerIsRunning = false;
        }
        SendPacket();
    }
    else
    {
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->SetCheckBoxState(true);
        petfilteronoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_item_on.ddj", 0, 0);
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->Macro_PetFilter = true;
        if(!g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->PetPickTimerIsRunning)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->PickWithPet();
        }
        SendPacket();
    }
}
void CIFMacro::ActivePetFilter()
{
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->SetCheckBoxState(true);
    petfilteronoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_item_on.ddj", 0, 0);
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->Macro_PetFilter = true;
    if(!g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->PetPickTimerIsRunning)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->PickWithPet();
    }
}


void CIFMacro::AutoScrollOptionBtn()
{
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(true);

    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(4);

    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    wnd_pos r;
    r =   g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->GetPos();
    this->MoveGWnd(r.x - 313, r.y);
}

void CIFMacro::AutoScrollOnOffBtn(){
    if(this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->GetCheckedState_MAYBE())
    {

        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->SetCheckBoxState(false);
        autoscrollonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_scroll_off.ddj", 0, 0);
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Macro_AutoScroll = false;
        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->AutoScrollTimerRunning)
        {
            g_pCGInterface->KillTimer(START_AUTO_SCROLL_TIMER);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->AutoScrollTimerRunning = false;
        }
        SendPacket();
    }
    else
    {
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->SetCheckBoxState(true);
        autoscrollonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_scroll_on.ddj", 0, 0);
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Macro_AutoScroll = true;
        if(!g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->AutoScrollTimerRunning)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->AutoScrolling();
        }
        SendPacket();
    }
}

void CIFMacro::ActiveAutoScroll()
{
    this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->SetCheckBoxState(true);
    autoscrollonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_scroll_on.ddj", 0, 0);
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Macro_AutoScroll = true;
    if(!g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->AutoScrollTimerRunning)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->AutoScrolling();
    }
}
void CIFMacro::SendPacket()
{
    CMsgStreamBuffer buf(0x187E);
    buf << (BYTE)this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_POTION_CHECKBOX, 1)->GetCheckedState_MAYBE();
    buf << (BYTE)this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SKILL_CHECKBOX, 1)->GetCheckedState_MAYBE();
    buf << (BYTE)this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_HUNT_CHECKBOX, 1)->GetCheckedState_MAYBE();
    buf << (BYTE)this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_PET_FILTER_CHECKBOX, 1)->GetCheckedState_MAYBE();
    buf << (BYTE)this->m_IRM.GetResObj<CIFCheckBox>(GDR_MACRO_AUTO_SCROLL_CHECKBOX, 1)->GetCheckedState_MAYBE();
    SendMsg(buf);
}
