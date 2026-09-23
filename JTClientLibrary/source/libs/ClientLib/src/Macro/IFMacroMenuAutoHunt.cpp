<<<<<<< ours
#include "IFMacroMenuAutoHunt.h"
#include <support/SafePath.h>
#include "MacroSafety.h"
=======
#include "IFMacroMenuAutoHunt.h"
#include <support/SafePath.h>
#include "MacroSafety.h"
>>>>>>> theirs
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
#include <CustomInterface/IFPopupList.h>
#include <EntityManagerClient.h>
#include <ICMonster.h>
#include <cmath>
#include <IFPlayerMiniInfo.h>
#include <CharacterDependentData.h>
#include <sstream>
#include <BSLib/multibyte.h>
#include <fstream>
#include <iostream>
#include <CustomData/CustomDataManager.h>
#include "../../../../DevKit_DLL/src/Util.h"



#include "IFMacro.h"

static float MacroHuntDistance(const D3DVECTOR& left, const D3DVECTOR& right)
{
    const float dx = left.x - right.x;
    const float dy = left.y - right.y;
    const float dz = left.z - right.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoHunt, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoHunt, CIFWnd)
                    ONG_COMMAND(300, &CIFMacroMenuAutoHunt::OnUnknownStuff)
                    ONG_COMMAND(301, &CIFMacroMenuAutoHunt::OnUnknownStuff)
                    ONG_COMMAND(29, &CIFMacroMenuAutoHunt::SaveButton)
                    ONG_COMMAND(30, &CIFMacroMenuAutoHunt::CancelBtn)

                    ONG_COMMAND(71, &CIFMacroMenuAutoHunt::AddPtMemberBtn)
                    ONG_COMMAND(72, &CIFMacroMenuAutoHunt::RemovePtMemberBtn)
                    ONG_COMMAND(99999, &CIFMacroMenuAutoHunt::LoadPartyMembers)

                    ONG_COMMAND(84, &CIFMacroMenuAutoHunt::PartyExpItemSetting)
                    ONG_COMMAND(87, &CIFMacroMenuAutoHunt::PartyExpItemSetting)
                    ONG_COMMAND(90, &CIFMacroMenuAutoHunt::PartyExpItemSetting)
                    ONG_COMMAND(93, &CIFMacroMenuAutoHunt::PartyExpItemSetting)



GFX_END_MESSAGE_MAP()

void CIFMacroMenuAutoHunt::LoadPartyMembers()
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
void CIFMacroMenuAutoHunt::AddPtMemberBtn(){

    std::n_wstring charname = m_popup->m_text->GetNText();
    if(charname.empty())
    {
        g_pCGInterface->ShowMessage_Notice(KmtGetText(L"UIIT_KMT_ENTER_THE_CHARACTER_NAME"));
        //g_pCGInterface->ShowMessage_Notice(TSM_GETTEXTPTR(L"UIIT_AUTO_HUNT_SLOT_0"));
        return;
    }
    if(AutoPartyMemberList.size() < 7)
    {
        if(AutoPartyMemberList.find(charname) == AutoPartyMemberList.end())
        {
            AutoPartyMemberList.insert(std::make_pair(charname, charname));
            ClearPartySlotDDJ();
        }
        else
        {
            g_pCGInterface->ShowMessage_Notice(KmtGetText(L"UIIT_KMT_THIS_PARTY_MEMBER_IS_ALREADY_ADDED_TO_LIST"));
           // g_pCGInterface->ShowMessage_Notice(TSM_GETTEXTPTR(L"UIIT_AUTO_HUNT_SLOT_1"));
        }
    }
    else
    {
        g_pCGInterface->ShowMessage_Notice(KmtGetText(L"UIIT_KMT_MAXIMUM_CHARACTER_MEMBER_CAN_BE_ADD_TO_LIST"));
        //g_pCGInterface->ShowMessage_Notice(TSM_GETTEXTPTR(L"UIIT_AUTO_HUNT_SLOT_2"));
    }
    int i = 0;
    for(std::map<std::n_wstring, std::n_wstring>::iterator  it = AutoPartyMemberList.begin(); it != AutoPartyMemberList.end(); it++)
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
void CIFMacroMenuAutoHunt::RemovePtMemberBtn(){
    if(!SelectedPartyMemberName.empty())
    {
        if(AutoPartyMemberList.find(SelectedPartyMemberName) != AutoPartyMemberList.end())
        {
            AutoPartyMemberList.erase(SelectedPartyMemberName);
            ClearPartySlotDDJ();

            for(int i = 0; i < 7; i++)
            {
                m_PartySlot[i]->LoadItems(L"");
            }

            int i = 0;
            for(std::map<std::n_wstring, std::n_wstring>::iterator  it = AutoPartyMemberList.begin(); it != AutoPartyMemberList.end(); it++)
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

int CIFMacroMenuAutoHunt::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 17)
            return -1;
    }

    return 100;
}


<<<<<<< ours
CIFMacroMenuAutoHunt::CIFMacroMenuAutoHunt(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
    m_pTabsSecond = 0;
    AutoPartyInviteCheckBox = GDR_AUTO_REFORM_PARTY = DONT_ATTACK_MONSTERCB = 0;
    AutoResPtMembersCB = AutoReturnHPMPLessCheckbox = AutoUseNasrun = AutoUseNasrunIgnore = 0;
=======
CIFMacroMenuAutoHunt::CIFMacroMenuAutoHunt(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
    m_pTabsSecond = 0;
    AutoPartyInviteCheckBox = GDR_AUTO_REFORM_PARTY = DONT_ATTACK_MONSTERCB = 0;
    AutoResPtMembersCB = AutoReturnHPMPLessCheckbox = AutoUseNasrun = AutoUseNasrunIgnore = 0;
>>>>>>> theirs
    AutoPartyMemberList = std::map<std::n_wstring, std::n_wstring>();
    SelectedPartyMemberName = std::n_wstring();
    Macro_AutoHunt = false;
    AutoHuntSetting = std::map<eAutoHuntSetting, int>();


<<<<<<< ours
    MacroAutoTownTimerRunning = false;
    AutoHuntTimerRunning = false;
    MacroAutoInviteRunning = false;
    m_lastDeathRecoveryTick = 0;
    m_deathRecoveryPending = false;
    UniqueTargetCheckBox = 0;
}
=======
    MacroAutoTownTimerRunning = false;
    AutoHuntTimerRunning = false;
    MacroAutoInviteRunning = false;
    m_lastDeathRecoveryTick = 0;
    m_deathRecoveryPending = false;
    UniqueTargetCheckBox = 0;
}
>>>>>>> theirs
CIFMacroMenuAutoHunt::~CIFMacroMenuAutoHunt(void) {
    if (m_pTabsSecond) {
        delete[] m_pTabsSecond;
        m_pTabsSecond = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFMacroMenuAutoHunt::OnCreate(long ln) {

    // Populate inherited members
    CIFWnd::OnCreate(ln);

    wnd_rect sz;
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacromenuautohunt.txt");
    m_IRM.CreateInterfaceSection("Create", this);


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
                m_pTabsSecond[0]->SetText(KmtGetText(L"UIIT_KMT_GENERAL"));
                //m_pTabsSecond[0]->SetText(TSM_GETTEXTPTR(L"UIIT_AUTO_HUNT_SLOT_3"));
            }
                break;
            case 1:
            {
                m_pTabsSecond[1]->MoveGWnd(m_pTabsSecond[0]->GetPos().x + tabWidth + 10, m_pTabsSecond[0]->GetPos().y);
                m_pTabsSecond[1]->SetText(KmtGetText(L"UIIT_KMT_PARTY"));
                //m_pTabsSecond[1]->SetText(TSM_GETTEXTPTR(L"UIIT_AUTO_HUNT_SLOT_4"));
                m_pTabsSecond[1]->SetClickable(true);
            }
                break;
        }
        m_pTabsSecond[i]->sub_64CC30(0);

    }
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->m_scrollButton->SetClickable(false);
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->m_arrowDownButton->SetClickable(false);
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->m_arrowUpButton->SetClickable(false);
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->SetClickable(false);

    m_IRM.GetResObj(43, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_ATTACK_THE_MONSTERS"));
    m_IRM.GetResObj(45, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_HUNTING_RANGE"));
    m_IRM.GetResObj(47, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_BERSERK_MODE"));
    m_IRM.GetResObj(49, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_RESURRECT"));
    m_IRM.GetResObj(51, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_RETURN"));
    m_IRM.GetResObj(53, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIGURE_AUTO_REPAIR"));
    m_IRM.GetResObj(55, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_RETURN_WHEN_HP_MP_POTION_10"));
    m_IRM.GetResObj(57, 1)->SetText(KmtGetText(L"UIIT_KMT_USE_DEVIL_ANGEL_SPIRIT_S"));
    m_IRM.GetResObj(59, 1)->SetText(KmtGetText(L"UIIT_KMT_USE_DEVIL_ANGEL_SPIRIT_S_IGNORE_SKILL"));
    CIFWnd* uniqueTargetLabel = m_IRM.GetResObj(109, 1);
    if (uniqueTargetLabel)
        uniqueTargetLabel->SetText(KmtGetText(L"UIIT_KMT_UNIQUE_TARGET"));
    m_IRM.GetResObj(29, 1)->SetText(KmtGetText(L"UIIT_KMT_SAVE"));

    m_IRM.GetResObj(107, 1)->SetText(KmtGetText(L"UIIT_KMT_RESURRECT_PARTY_MEMBERS"));
    m_IRM.GetResObj(105, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_REFORM_PARTY"));
    m_IRM.GetResObj(95, 1)->SetText(KmtGetText(L"UIIT_KMT_ACCEPT_JOIN_PARTY_REQUESTS"));
    m_IRM.GetResObj(92, 1)->SetText(KmtGetText(L"UIIT_KMT_EXP_FREE_ITEM_FREE"));
    m_IRM.GetResObj(89, 1)->SetText(KmtGetText(L"UIIT_KMT_EXP_SHARE_ITEM_SHARE"));
    m_IRM.GetResObj(86, 1)->SetText(KmtGetText(L"UIIT_KMT_EXP_FREE_ITEM_SHARE"));
    m_IRM.GetResObj(83, 1)->SetText(KmtGetText(L"UIIT_KMT_EXP_SHARE_ITEM_FREE"));
    m_IRM.GetResObj(80, 1)->SetText(KmtGetText(L"UIIT_KMT_ONLY_FROM_LIST"));
    m_IRM.GetResObj(77, 1)->SetText(KmtGetText(L"UIIT_KMT_ACCEPT_PARTY_INVITE"));
    m_IRM.GetResObj(74, 1)->SetText(KmtGetText(L"UIIT_KMT_INVITE_TO_PARTY"));
    m_IRM.GetResObj(72, 1)->SetText(KmtGetText(L"UIIT_KMT_REMOVE"));
    m_IRM.GetResObj(71, 1)->SetText(KmtGetText(L"UIIT_KMT_ADD"));
    m_IRM.GetResObj(30, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    AutoReturnHPMPLessCheckbox = m_IRM.GetResObj<CIFCheckBox>(56, 1);
    AutoUseNasrun = m_IRM.GetResObj<CIFCheckBox>(58, 1);
    AutoUseNasrunIgnore = m_IRM.GetResObj<CIFCheckBox>(60, 1);


    std::n_wstring area1 = L"30 m";
    std::n_wstring area2 = L"50 m";
    std::n_wstring area3 = L"70 m";
    std::n_wstring area4 = L"100 m";
    std::n_wstring area5 = L"150 m";
    std::n_wstring area6 = L"200 m";
    std::n_wstring area7 = L"300 m";

    RadiusList = m_IRM.GetResObj<CIFEdit>(46, 1);
    RadiusList->SetText(L"10");
    RadiusList->AddValue_404(4);
    RadiusList->SetValue_404(1);
    RadiusList->SetMaxLength(3);

    AutoHuntSetting.insert(std::make_pair(RADUIS_SETTING, 10));


    std::n_wstring hwan1 = KmtGetText(L"UIIT_KMT_ALL");
    std::n_wstring hwan2 = KmtGetText(L"UIIT_KMT_GIANT");
    std::n_wstring hwan3 = TSM_GETTEXTPTR(L"UIIT_STT_OFF");

    ZerkList = m_IRM.GetResObj<CIFPopupList>(48, 1);
    ZerkList->m_list->sub_64F8A0(hwan3, 0, 0xffffff, 0xffffff, -1, 0, 0);
    ZerkList->m_list->sub_64F8A0(hwan1, 0, 0xffffff, 0xffffff, -1, 0, 0);
    ZerkList->m_list->sub_64F8A0(hwan2, 0, 0xffffff, 0xffffff, -1, 0, 0);
    //m_IRM.GetResObj<CIFPopupList>(48, 1)->m_list->sub_64F8A0(hwan3, 0, 0xffffff, 0xffffff, -1, 0, 0);

    ZerkList->m_text->SetText(hwan1.c_str());

    AutoHuntSetting.insert(std::make_pair(ZERK_SETTING, eZerkSetting::All));


    std::n_wstring sec1 = KmtGetText(L"UIIT_KMT_USE_RES_SCROLL");
    std::n_wstring sec2 = KmtGetText(L"UIIT_KMT_RESURRECT_IN_TOWN");
    std::n_wstring sec3 = KmtGetText(L"UIIT_KMT_WAIT_FOR_RESURRECTION");


    TownList = m_IRM.GetResObj<CIFPopupList>(50, 1);
    TownList->m_list->sub_64F8A0(sec1, 0, 0xffffff, 0xffffff, -1, 0, 0);
    TownList->m_list->sub_64F8A0(sec2, 0, 0xffffff, 0xffffff, -1, 0, 0);
    TownList->m_list->sub_64F8A0(sec3, 0, 0xffffff, 0xffffff, -1, 0, 0);
    TownList->m_text->SetText(sec1.c_str());

    AutoHuntSetting.insert(std::make_pair(RETURN_TOWN_SETTING, eTownSetting::RES_SCROLL));



    std::n_wstring ret1 = KmtGetText(L"UIIT_KMT_IN_1_HOUR");
    std::n_wstring ret2 = KmtGetText(L"UIIT_KMT_IN_3_HOURS");
    std::n_wstring ret3 = KmtGetText(L"UIIT_KMT_IN_5_HOURS");
    std::n_wstring ret4 = KmtGetText(L"UIIT_KMT_IN_10_HOURS");
    std::n_wstring ret5 = KmtGetText(L"UIIT_KMT_IN_24_HOURS");







    HourList = m_IRM.GetResObj<CIFPopupList>(52, 1);
    HourList->m_list->sub_64F8A0(ret1, 0, 0xffffff, 0xffffff, -1, 0, 0);
    HourList->m_list->sub_64F8A0(ret2, 0, 0xffffff, 0xffffff, -1, 0, 0);
    HourList->m_list->sub_64F8A0(ret3, 0, 0xffffff, 0xffffff, -1, 0, 0);
    HourList->m_list->sub_64F8A0(ret4, 0, 0xffffff, 0xffffff, -1, 0, 0);
    HourList->m_list->sub_64F8A0(ret5, 0, 0xffffff, 0xffffff, -1, 0, 0);

    HourList->m_text->SetText(ret1.c_str());


    AutoHuntSetting.insert(std::make_pair(BACK_HOUR_SETTING, eBackTownHour::HOUR_1));


    std::n_wstring rep1 = KmtGetText(L"UIIT_KMT_USE_REPAIR_HAMMER");
    std::n_wstring rep2 = KmtGetText(L"UIIT_KMT_RETURN_TO_TOWN");


    RepairList = m_IRM.GetResObj<CIFPopupList>(54, 1);
    RepairList->m_list->sub_64F8A0(rep1, 0, 0xffffff, 0xffffff, -1, 0, 0);
    RepairList->m_list->sub_64F8A0(rep2, 0, 0xffffff, 0xffffff, -1, 0, 0);
    RepairList->m_text->SetText(rep1.c_str());
    AutoHuntSetting.insert(std::make_pair(REPAIR_SETTING, eRepairSetting::USE_REPAIR_HAMMER));


    m_IRM.GetResObj<CIFPopupList>(54, 1)->BringToFront();
    m_IRM.GetResObj<CIFPopupList>(52, 1)->BringToFront();
    m_IRM.GetResObj<CIFPopupList>(50, 1)->BringToFront();
    m_IRM.GetResObj<CIFPopupList>(48, 1)->BringToFront();
    m_IRM.GetResObj<CIFEdit>(46, 1)->BringToFront();
    //m_IRM.GetResObj<CIFPopupList>(44, 1)->BringToFront();


    m_IRM.GetResObj<CIFCheckBox>(56, 1)->SetCheckBoxState(true);
    m_IRM.GetResObj<CIFCheckBox>(58, 1)->SetCheckBoxState(false);
    m_IRM.GetResObj<CIFCheckBox>(60, 1)->SetCheckBoxState(false);
    DONT_ATTACK_MONSTERCB = m_IRM.GetResObj<CIFCheckBox>(44, 1);
    DONT_ATTACK_MONSTERCB->SetCheckBoxState(false);
    UniqueTargetCheckBox = m_IRM.GetResObj<CIFCheckBox>(110, 1);
    if (UniqueTargetCheckBox)
        UniqueTargetCheckBox->SetCheckBoxState(false);




    GDR_AUTO_REFORM_TITLE = m_IRM.GetResObj<CIFEdit>(98, 1);


    GDR_AUTO_REFORM_TITLE->SetMaxLength(50);
    wnd_size tt =  GDR_AUTO_REFORM_TITLE->GetSize();

    GDR_AUTO_REFORM_TITLE->SetText(KmtGetText(L"UIIT_KMT_FOR_OPEN_HUNTING_ON_THE_SILKROAD"));
    GDR_AUTO_REFORM_TITLE->SetTextmode(tt.width);

    GDR_AUTO_REFORM_PT_LVL_MIN = m_IRM.GetResObj<CIFEdit>(100, 1);


    GDR_AUTO_REFORM_PT_LVL_MIN->SetMaxLength(3);
    wnd_size ttx =  GDR_AUTO_REFORM_PT_LVL_MIN->GetSize();

    GDR_AUTO_REFORM_PT_LVL_MIN->SetText(L"1");
    GDR_AUTO_REFORM_PT_LVL_MIN->SetTextmode(ttx.width);
    GDR_AUTO_REFORM_PT_LVL_MIN->AddValue_404(4);
    GDR_AUTO_REFORM_PT_LVL_MIN->SetValue_404(1);


    GDR_AUTO_REFORM_PT_LVL_MAX = m_IRM.GetResObj<CIFEdit>(102, 1);



    GDR_AUTO_REFORM_PT_LVL_MAX->SetMaxLength(3);
    wnd_size ttxx =  GDR_AUTO_REFORM_PT_LVL_MAX->GetSize();

    std::wstringstream ss;
    ss << m_Settings->ServerMaxLevel;
    std::wstring serverMaxLevelStr = ss.str();

    GDR_AUTO_REFORM_PT_LVL_MAX->SetText(serverMaxLevelStr.c_str());
    GDR_AUTO_REFORM_PT_LVL_MAX->SetTextmode(ttxx.width);
    GDR_AUTO_REFORM_PT_LVL_MAX->AddValue_404(4);
    GDR_AUTO_REFORM_PT_LVL_MAX->SetValue_404(1);

    m_IRM.GetResObj(103, 1)->SetText(L"~");
    sz.pos.x = 27;
    sz.pos.y = 167;
    sz.size.width = 238;
    sz.size.height = 24;



    AutoPartyInviteCheckBox = m_IRM.GetResObj<CIFCheckBox>(75, 1);
    AutoPartyInviteCheckBox->SetCheckBoxState(false);

    AutoPartyAcceptCheckBox =  m_IRM.GetResObj<CIFCheckBox>(78, 1);
    AutoPartyAcceptCheckBox->SetCheckBoxState(false);


    AutoPartyOnlyFromList = m_IRM.GetResObj<CIFCheckBox>(81, 1);
    AutoPartyOnlyFromList->SetCheckBoxState(true);

    GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(87, 1);
    GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
    GDR_PT_EXP_AUTO_ITEM_NOCheckBox = m_IRM.GetResObj<CIFCheckBox>(84, 1);

    GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox = m_IRM.GetResObj<CIFCheckBox>(90, 1);
    GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);


    GDR_PT_EXP_FREE_ITEM_FREECheckBox = m_IRM.GetResObj<CIFCheckBox>(93, 1);
    GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);


    GDR_ACCEPT_JOIN_PT_REQ = m_IRM.GetResObj<CIFCheckBox>(96, 1);
    GDR_ACCEPT_JOIN_PT_REQ->SetCheckBoxState(false);

    GDR_AUTO_REFORM_PARTY = m_IRM.GetResObj<CIFCheckBox>(106, 1);
    GDR_AUTO_REFORM_PARTY->SetCheckBoxState(false);

    AutoResPtMembersCB = m_IRM.GetResObj<CIFCheckBox>(108, 1);
    AutoResPtMembersCB->SetCheckBoxState(false);


    sz.pos.x = 353;
    sz.pos.y = 348;
    sz.size.width = 180;
    sz.size.height = 24;
    m_popup = m_IRM.GetResObj<CIFPopupList2>(70, 1);


    m_popup->m_text->SetGWndSize(230, 24);
    m_popup->m_bg->SetGWndSize(230, 24);


    m_popup->m_btn->SetUniqueID(99999);

    for (int i = 0; i < 7; ++i)
    {
        m_PartySlot[i] = m_IRM.GetResObj<CIFMacroMenuAutoHuntPartySlot>(62+i, 1);
        m_PartySlot[i]->SetBarSize(238, 24);
    }

    //LoadSetting();
    return true;
}

void CIFMacroMenuAutoHunt::OnUpdate() {
    /* std::n_wstring gettext = m_IRM.GetResObj<CIFTextBox>(100, 1)->GetNText();


     std::string str(gettext.begin(), gettext.end());

     // std::string'i int'e dÃ¶nÃ¼ÅŸtÃ¼r
     std::istringstream iss(str);
     int result;
     if((iss >> result))
     {
         if(result > m_Settings->ServerMaxLevel)
         {
             std::wstringstream ss;
             ss << m_Settings->ServerMaxLevel;
             std::wstring serverMaxLevelStr = ss.str();

            m_IRM.GetResObj<CIFTextBox>(100, 1)->SetText(serverMaxLevelStr.c_str());
         }
         if(result == 0)
         {
             m_IRM.GetResObj<CIFTextBox>(100, 1)->SetText(L"1");
         }
     }
 */
    std::n_wstring gettext2 = GDR_AUTO_REFORM_PT_LVL_MAX->GetNText();


    std::string str2(gettext2.begin(), gettext2.end());

    // std::string'i int'e dÃ¶nÃ¼ÅŸtÃ¼r
    std::istringstream isss(str2);
    int result2;
    if((isss >> result2))
    {
        if(result2 > m_Settings->ServerMaxLevel)
        {
            std::wstringstream ss;
            ss << m_Settings->ServerMaxLevel;
            std::wstring serverMaxLevelStr = ss.str();

            GDR_AUTO_REFORM_PT_LVL_MAX->SetText(serverMaxLevelStr.c_str());
        }
        if(result2 == 0)
        {
            GDR_AUTO_REFORM_PT_LVL_MAX->SetText(L"1");
        }
    }
}

void CIFMacroMenuAutoHunt::OnUnknownStuff() {
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
void CIFMacroMenuAutoHunt::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabsSecond[i]->sub_64CC30(0);
        m_pTabsSecond[i]->m_FontTexture.sub_8B4750(2);
    }


    m_pTabsSecond[page]->sub_64CC30(1);
    switch (page)
    {
        case 0://title
        {
            m_IRM.GetResObj(107, 1)->ShowGWnd(true);
            AutoResPtMembersCB->ShowGWnd(true);

            for(int is = 31; is < 61; is++)
            {
                this->m_IRM.GetResObj(is, 1)->ShowGWnd(true);
            }
            CIFWnd* uniqueTargetLabel = this->m_IRM.GetResObj(109, 1);
            if (uniqueTargetLabel)
                uniqueTargetLabel->ShowGWnd(true);
            if (UniqueTargetCheckBox)
                UniqueTargetCheckBox->ShowGWnd(true);

            for(int is = 61; is < 107; is++)
            {
                this->m_IRM.GetResObj(is, 1)->ShowGWnd(false);
            }
            for (int i = 0; i < 7; ++i)
            {
                //m_PartySlot[i]->ShowGWnd(false);
            }
            ClearPartySlotDDJ();
            this->m_IRM.GetResObj(69, 1)->ShowGWnd(false);
        }
            break;
        case 1://name tag
        {
            m_IRM.GetResObj(107, 1)->ShowGWnd(false);
            AutoResPtMembersCB->ShowGWnd(false);

            for(int is = 31; is < 61; is++)
            {
                m_IRM.GetResObj(is, 1)->ShowGWnd(false);
            }
            CIFWnd* uniqueTargetLabel = m_IRM.GetResObj(109, 1);
            if (uniqueTargetLabel)
                uniqueTargetLabel->ShowGWnd(false);
            if (UniqueTargetCheckBox)
                UniqueTargetCheckBox->ShowGWnd(false);

            for(int is = 61; is < 107; is++)
            {
                this->m_IRM.GetResObj(is, 1)->ShowGWnd(true);
            }
            for (int i = 0; i < 7; ++i)
            {
                //m_PartySlot[i]->ShowGWnd(true);
            }
            ClearPartySlotDDJ();
            this->m_IRM.GetResObj(69, 1)->ShowGWnd(false);
        }
            break;
    }
}
void CIFMacroMenuAutoHunt::ClearPartySlotDDJ()
{
    for (int i = 0; i < 7; ++i)
    {
        m_PartySlot[i]->ClearDDJ();
    }
    SelectedPartyMemberName = L"";
}

void CIFMacroMenuAutoHunt::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2 - 50) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
void CIFMacroMenuAutoHunt::BackTown()
{
    if (!g_pMyPlayerObj || !g_pCGInterface || !g_pCGInterface->GetMainPopup() ||
        !g_pCGInterface->GetMainPopup()->GetInventory())
        return;

    if(MacroAutoInviteRunning)
    {
        MacroAutoInviteRunning = false;
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
    }
    if(MacroAutoTownTimerRunning)
    {
        MacroAutoTownTimerRunning = false;
        g_pCGInterface->KillTimer(START_BACK_TOWN);
    }

    bool returnRequested = false;
    if(g_pMyPlayerObj->CHARACTER_STATUS == Dead)
    {
        CMsgStreamBuffer buf(0x3053);
        buf << (byte)1;
        SendMsg(buf);
        returnRequested = true;
    }
    else
    {
        for(std::n_vector<CIFSlotWithHelp*>::iterator it =
                g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.begin();
            it != g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.end(); ++it)
        {
            CIFSlotWithHelp* slot = *it;
            if(slot && slot->ItemInfo && slot->ItemInfo->GetItemData() &&
               slot->ItemInfo->GetItemData()->IsReturnScroll())
            {
                slot->UseItem();
                returnRequested = true;
                break;
            }
        }
    }

    if (!returnRequested)
    {
        // Do not keep hunting after a safety return was requested but no
        // usable return item exists.
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_MSG_NOT_ENOUGH_ITEM"));
    }

    Macro_AutoHunt = false;
    if(AutoHuntTimerRunning)
    {
        AutoHuntTimerRunning = false;
        g_pCGInterface->KillTimer(START_AUTO_HUNT);
    }

    CIFMacro* macro = g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1);
    if (macro)
    {
        macro->GDR_MACRO_AUTO_HUNT_CHECKBOX_STATE(false);
        if (macro->autohuntonoffbtn)
            macro->autohuntonoffbtn->TB_Func_13("clientlibrary\\macro\\macro_icon_hunting_off.ddj", 0, 0);
        macro->SendPacket();
    }
}
void CIFMacroMenuAutoHunt::AutoReformParty()
{
    if (!g_pMyPlayerObj)
        return;
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead ||g_pMyPlayerObj->CHARACTER_STATUS == Stall)
        return;
    if(g_CCharacterDependentData.GetRegisteredPartyID() == 0)
    {
        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        if (partyData.NumberOfMembers < 8)
        {

            byte Action = 0;
            if(GDR_PT_EXP_FREE_ITEM_FREECheckBox->GetCheckedState_MAYBE())
            {
                Action = 0x4; // can ivite true // cant invite 0
            }
            else  if(GDR_PT_EXP_AUTO_ITEM_NOCheckBox->GetCheckedState_MAYBE())
            {
                Action = 0x5; // can invite // cant invite 1
            }
            else  if(GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->GetCheckedState_MAYBE())
            {
                Action = 0x6; // can invite cant invite // 2
            }
            else  if(GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->GetCheckedState_MAYBE())
            {
                Action = 0x7; // can invite cant invite // 3
            }
            if(Action != 0)
            {
                CMsgStreamBuffer buf(0x7069);
                buf << (unsigned int)0;
                buf << (unsigned int)0;
                buf << Action;
                byte Purpose = 0;

                if(g_pMyPlayerObj->GetJobType() == 4)
                {
                    Purpose = 0;
                }
                else if(g_pMyPlayerObj->GetJobType() == 3 || g_pMyPlayerObj->GetJobType() == 1)// hunter trader
                {
                    Purpose = 2;
                }
                else if(g_pMyPlayerObj->GetJobType() == 2) // thief
                {
                    Purpose = 3;
                }
                buf << Purpose;



                std::wstring test1 = GDR_AUTO_REFORM_PT_LVL_MIN->GetNText().c_str();
                std::string test = TO_STRING(test1).c_str();

                byte ConvertedInteger = atoi(test.c_str());

                buf << ConvertedInteger;

                std::wstring test12 = GDR_AUTO_REFORM_PT_LVL_MAX->GetNText().c_str();
                std::string test2 = TO_STRING(test12).c_str();

                byte MaxLevel = atoi(test2.c_str());

                buf << MaxLevel;

                std::n_wstring Title = GDR_AUTO_REFORM_TITLE->GetNText();

                std::string x = TO_STRING(Title);

                buf << x;

                SendMsg(buf);

            }
        }
    }
}
void CIFMacroMenuAutoHunt::InviteNearPartyMembers()
{
    if (!g_pMyPlayerObj || !g_pGfxEttManager)
        return;
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead ||g_pMyPlayerObj->CHARACTER_STATUS == Stall)
        return;

    byte inviteAction = 0;
    if(GDR_PT_EXP_FREE_ITEM_FREECheckBox->GetCheckedState_MAYBE())
        inviteAction = 0x4;
    else if(GDR_PT_EXP_AUTO_ITEM_NOCheckBox->GetCheckedState_MAYBE())
        inviteAction = 0x5;
    else if(GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->GetCheckedState_MAYBE())
        inviteAction = 0x6;
    else if(GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->GetCheckedState_MAYBE())
        inviteAction = 0x7;
    if (inviteAction == 0)
        return;

    const SPartyData& currentParty = g_CCharacterDependentData.GetPartyData();
    D3DVECTOR playerGlobal = g_pMyPlayerObj->GetLocation();
    GetSilkPos(g_pMyPlayerObj->GetRegion(), playerGlobal);
    const float inviteRadius = (float)AutoHuntSetting[RADUIS_SETTING] + 10.0f;

    for (std::map<int, CIObject*>::iterator candidateIt =
             g_pGfxEttManager->entities.begin();
         candidateIt != g_pGfxEttManager->entities.end(); ++candidateIt)
    {
        CIObject* object = candidateIt->second;
        if (!object || strcmp(object->GetRuntimeClass()->m_lpszClassName, "CICUser"))
            continue;

        CICUser* candidate = (CICUser*)object;
        if (!candidate || candidate->GetUniqueId() == 0 ||
            candidate->GetUniqueId() == g_pMyPlayerObj->GetUniqueId() ||
            candidate->CHARACTER_STATUS == Dead ||
            candidate->GetJobType() != g_pMyPlayerObj->GetJobType())
            continue;

        if (AutoPartyOnlyFromList->GetCheckedState_MAYBE() &&
            AutoPartyMemberList.find(candidate->GetName()) == AutoPartyMemberList.end())
            continue;

        bool alreadyInParty = false;
        for (int partyIndex = 0; partyIndex < currentParty.NumberOfMembers; ++partyIndex)
        {
            if (g_CCharacterDependentData.GetPartyMemberData(partyIndex).m_charactername ==
                candidate->GetName())
            {
                alreadyInParty = true;
                break;
            }
        }
        if (alreadyInParty)
            continue;

        D3DVECTOR candidateGlobal = candidate->GetLocation();
        GetSilkPos(candidate->GetRegion(), candidateGlobal);
        if (MacroHuntDistance(playerGlobal, candidateGlobal) > inviteRadius * 10.0f)
            continue;

        CMsgStreamBuffer invite(0x7060);
        invite << candidate->GetUniqueId();
        invite << inviteAction;
        SendMsg(invite);
        return;
    }
    return;

    unsigned int FoundedUserUniqueID = 0;
    std::n_wstring FoundedCharName;
    std::map<int, CIObject*> entities = g_pGfxEttManager->entities;
    for (std::map<int, CIObject*>::iterator it = entities.begin(); it != entities.end(); ++it) {
        if (!strcmp(it->second->GetRuntimeClass()->m_lpszClassName, "CICUser")) {
            CICUser* nearmember = (CICUser*)it->second;
            if (nearmember) {
                if(nearmember->GetJobType() == g_pMyPlayerObj->GetJobType())
                {
                    if(AutoPartyOnlyFromList->GetCheckedState_MAYBE())
                    {
                        if(AutoPartyMemberList.find(nearmember->GetName()) != AutoPartyMemberList.end())
                        {
                            FoundedUserUniqueID = nearmember->GetUniqueId();
                            FoundedCharName = nearmember->GetName();
                            break;
                        }
                    }
                    else
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
        }
    }
    bool isinparty = false;
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

    if(FoundedUserUniqueID != 0 && !isinparty)
    {

        byte Action = 0;
        if(GDR_PT_EXP_FREE_ITEM_FREECheckBox->GetCheckedState_MAYBE())
        {
            Action = 0x4;
        }
        else  if(GDR_PT_EXP_AUTO_ITEM_NOCheckBox->GetCheckedState_MAYBE())
        {
            Action = 0x5;
        }
        else  if(GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->GetCheckedState_MAYBE())
        {
            Action = 0x6;
        }
        else  if(GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->GetCheckedState_MAYBE())
        {
            Action = 0x7;
        }
        if(Action != 0)
        {
            CMsgStreamBuffer buf(0x7060);
            buf << FoundedUserUniqueID;
            buf << Action;
            SendMsg(buf);
        }
        // disable exp and item 0x5 == disable item active exp

    }
}
void CIFMacroMenuAutoHunt::CheckNasrun()
{
    if(g_pCGInterface->GetMainPopup()->GetEquipment()->DEVILSLOT != NULL)
    {
        if(g_pCGInterface->GetMainPopup()->GetEquipment()->DEVILSLOT->ItemInfo != NULL)
        {
            if(g_pCGInterface->GetMainPopup()->GetEquipment()->DEVILSLOT->ItemInfo->GetItemData() != NULL)
            {
                if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd != NULL)
                {
                    if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot1 != NULL)
                    {
                        if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot1->m_pSlot != NULL)
                        {
                            if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot1->m_pSlot->GetSkillSlotInDex() != 0)
                            {
                                int GetSkillID = g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot1->m_pSlot->GetSkillSlotInDex();
                                if(GetSkillID != 0)
                                {
                                    float delay = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(GetSkillID);
                                    // printf("%f delay\n", delay);
                                    if (delay > 0.0)
                                        return;
                                    else
                                    {

//                                               g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot1->m_pSlot->UseItemSlot();
                                        CMsgStreamBuffer buf(0x7074);
                                        byte i = 4;
                                        buf << (byte)1;
                                        buf << i;
                                        buf << GetSkillID;
                                        buf << (byte)0;
                                        SendMsg(buf);

                                    }
                                }
                            }
                        }
                    }
                }

            }
        }
    }
}
void CIFMacroMenuAutoHunt::CheckNasrunIgnore()
{
    if(g_pCGInterface->GetMainPopup()->GetEquipment()->DEVILSLOT != NULL)
    {
        if(g_pCGInterface->GetMainPopup()->GetEquipment()->DEVILSLOT->ItemInfo != NULL)
        {
            if(g_pCGInterface->GetMainPopup()->GetEquipment()->DEVILSLOT->ItemInfo->GetItemData() != NULL)
            {
                if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd != NULL)
                {
                    if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot2 != NULL)
                    {
                        if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot2->m_pSlot != NULL)
                        {
                            if(g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot2->m_pSlot->GetSkillSlotInDex() != 0)
                            {
                                int GetSkillID = g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot2->m_pSlot->GetSkillSlotInDex();
                                if(GetSkillID != 0)
                                {
                                    float delay = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(GetSkillID);
                                    // printf("%f delay\n", delay);
                                    if (delay > 0.0)
                                        return;
                                    else
                                    {

//                                               g_pCGInterface->GetMainPopup()->GetEquipment()->pSkillSlotWnd->CIFSkillSlot1->m_pSlot->UseItemSlot();
                                        CMsgStreamBuffer buf(0x7074);
                                        byte i = 4;
                                        buf << (byte)1;
                                        buf << i;
                                        buf << GetSkillID;
                                        buf << (byte)0;
                                        SendMsg(buf);

                                    }
                                }
                            }
                        }
                    }
                }

            }
        }
    }
}
#include "NavMesh/RTNavMesh.h"
std::map<short,CRTNavMeshTerrain*>  CIFMacroMenuAutoHunt::CRTNavMeshTerrainAll;
<<<<<<< ours
void CIFMacroMenuAutoHunt::StartAutoHunt()
{
    if (!g_pCGInterface)
        return;

    if (!IsRuntimeReady())
    {
        Macro_AutoHunt = false;
        g_pCGInterface->KillTimer(START_AUTO_HUNT);
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        AutoHuntTimerRunning = MacroAutoTownTimerRunning = MacroAutoInviteRunning = false;
        return;
    }

    if (!Macro_AutoHunt)
    {
        g_pCGInterface->KillTimer(START_AUTO_HUNT);
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        AutoHuntTimerRunning = false;
        MacroAutoTownTimerRunning = false;
        MacroAutoInviteRunning = false;
        return;
    }

    if(!AutoHuntTimerRunning)
    {
        AutoHuntTimerRunning = true;
        g_pCGInterface->StartTimer(START_AUTO_HUNT, 500);
    }

    const bool settingsReady =
        AutoHuntSetting.find(RADUIS_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(ZERK_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(RETURN_TOWN_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(BACK_HOUR_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(REPAIR_SETTING) != AutoHuntSetting.end();

    if (!m_Player ||
        !m_Player->FirstSpawn ||
        !g_pMyPlayerObj ||
        g_pMyPlayerObj->CHARACTER_STATUS == 0 ||
        g_pMyPlayerObj->GetMaxHp() == 0 ||
        g_pMyPlayerObj->GetMaxMp() == 0 ||
        (g_pMyPlayerObj->CHARACTER_STATUS != Dead &&
         g_pMyPlayerObj->GetCurrentHp() == 0) ||
        !settingsReady ||
        !g_pCGInterface->GetMainPopup() ||
        !g_pCGInterface->GetMainPopup()->GetInventory() ||
        !g_pCGInterface->GetMainPopup()->GetEquipment())
    {
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        MacroAutoTownTimerRunning = false;
        MacroAutoInviteRunning = false;
        StartRegion.r = 0;
        StartPosition.x = 0.0f;
        StartPosition.y = 0.0f;
        StartPosition.z = 0.0f;
        return;
    }

    if (StartRegion.r == 0)
    {
        StartRegion = g_pMyPlayerObj->GetRegion();
        StartPosition = g_pMyPlayerObj->GetLocation();
    }

    if (g_CurrentNavMesh)
        CIFMacroMenuAutoHunt::CRTNavMeshTerrainAll[g_CurrentNavMesh->m_Region] = g_CurrentNavMesh;

    if(!MacroAutoTownTimerRunning)
    {
        MacroAutoTownTimerRunning = true;
        const int configuredHours = AutoHuntSetting[BACK_HOUR_SETTING];
        const int safeHours = (configuredHours == 1 || configuredHours == 3 ||
            configuredHours == 5 || configuredHours == 10 || configuredHours == 24)
            ? configuredHours : 1;
        AutoHuntSetting[BACK_HOUR_SETTING] = safeHours;
        g_pCGInterface->StartTimer(START_BACK_TOWN, (unsigned long)safeHours * 3600000UL);
    }
    if(g_pMyPlayerObj->CHARACTER_STATUS == Dead)
    {
        const unsigned long now = GetTickCount();
        if (m_deathRecoveryPending && now - m_lastDeathRecoveryTick < 5000UL)
            return;
        m_deathRecoveryPending = true;
        m_lastDeathRecoveryTick = now;
=======
void CIFMacroMenuAutoHunt::StartAutoHunt()
{
    if (!g_pCGInterface)
        return;

    if (!IsRuntimeReady())
    {
        Macro_AutoHunt = false;
        g_pCGInterface->KillTimer(START_AUTO_HUNT);
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        AutoHuntTimerRunning = MacroAutoTownTimerRunning = MacroAutoInviteRunning = false;
        return;
    }

    if (!Macro_AutoHunt)
    {
        g_pCGInterface->KillTimer(START_AUTO_HUNT);
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        AutoHuntTimerRunning = false;
        MacroAutoTownTimerRunning = false;
        MacroAutoInviteRunning = false;
        return;
    }

    if(!AutoHuntTimerRunning)
    {
        AutoHuntTimerRunning = true;
        g_pCGInterface->StartTimer(START_AUTO_HUNT, 500);
    }

    const bool settingsReady =
        AutoHuntSetting.find(RADUIS_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(ZERK_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(RETURN_TOWN_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(BACK_HOUR_SETTING) != AutoHuntSetting.end() &&
        AutoHuntSetting.find(REPAIR_SETTING) != AutoHuntSetting.end();

    if (!m_Player ||
        !m_Player->FirstSpawn ||
        !g_pMyPlayerObj ||
        g_pMyPlayerObj->CHARACTER_STATUS == 0 ||
        g_pMyPlayerObj->GetMaxHp() == 0 ||
        g_pMyPlayerObj->GetMaxMp() == 0 ||
        (g_pMyPlayerObj->CHARACTER_STATUS != Dead &&
         g_pMyPlayerObj->GetCurrentHp() == 0) ||
        !settingsReady ||
        !g_pCGInterface->GetMainPopup() ||
        !g_pCGInterface->GetMainPopup()->GetInventory() ||
        !g_pCGInterface->GetMainPopup()->GetEquipment())
    {
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        g_pCGInterface->KillTimer(STARTED_INVITE_PLAYER_PARTY);
        MacroAutoTownTimerRunning = false;
        MacroAutoInviteRunning = false;
        StartRegion.r = 0;
        StartPosition.x = 0.0f;
        StartPosition.y = 0.0f;
        StartPosition.z = 0.0f;
        return;
    }

    if (StartRegion.r == 0)
    {
        StartRegion = g_pMyPlayerObj->GetRegion();
        StartPosition = g_pMyPlayerObj->GetLocation();
    }

    if (g_CurrentNavMesh)
        CIFMacroMenuAutoHunt::CRTNavMeshTerrainAll[g_CurrentNavMesh->m_Region] = g_CurrentNavMesh;

    if(!MacroAutoTownTimerRunning)
    {
        MacroAutoTownTimerRunning = true;
        const int configuredHours = AutoHuntSetting[BACK_HOUR_SETTING];
        const int safeHours = (configuredHours == 1 || configuredHours == 3 ||
            configuredHours == 5 || configuredHours == 10 || configuredHours == 24)
            ? configuredHours : 1;
        AutoHuntSetting[BACK_HOUR_SETTING] = safeHours;
        g_pCGInterface->StartTimer(START_BACK_TOWN, (unsigned long)safeHours * 3600000UL);
    }
    if(g_pMyPlayerObj->CHARACTER_STATUS == Dead)
    {
        const unsigned long now = GetTickCount();
        if (m_deathRecoveryPending && now - m_lastDeathRecoveryTick < 5000UL)
            return;
        m_deathRecoveryPending = true;
        m_lastDeathRecoveryTick = now;
>>>>>>> theirs
        if(AutoHuntSetting[RETURN_TOWN_SETTING] == eTownSetting::RES_SCROLL) // Use Res Scroll
        {

            for(std::n_vector<CIFSlotWithHelp*>::iterator it = g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.begin(); it != g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.end(); it++)
            {
                if((*it)->ItemInfo != NULL)
                {
                    if((*it)->ItemInfo->GetItemData()->IsResurrectScroll())
                    {
                        (*it)->UseItem();
                        break;
                    }
                }
            }
        }
        else if(AutoHuntSetting[RETURN_TOWN_SETTING] == eTownSetting::GO_TOWN) // go town
        {
            CMsgStreamBuffer buf(0x3053);
            buf << (byte)1;
            SendMsg(buf);
        }
        else if(AutoHuntSetting[RETURN_TOWN_SETTING] == eTownSetting::WAIT_FOR_RES) // wait for ress
        {

        }
    }

<<<<<<< ours
    m_deathRecoveryPending = false;

    if(AutoPartyInviteCheckBox->GetCheckedState_MAYBE())
=======
    m_deathRecoveryPending = false;

    if(AutoPartyInviteCheckBox->GetCheckedState_MAYBE())
>>>>>>> theirs
    {
        if(!MacroAutoInviteRunning)
        {
            MacroAutoInviteRunning = true;
            g_pCGInterface->StartTimer(STARTED_INVITE_PLAYER_PARTY, 5000);;
        }
    }

    if(GDR_AUTO_REFORM_PARTY->GetCheckedState_MAYBE())
    {
        AutoReformParty();
    }

    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead ||g_pMyPlayerObj->CHARACTER_STATUS == Stall)
        return;


    if(AutoReturnHPMPLessCheckbox->GetCheckedState_MAYBE())
    {

        int TotalHPPotion = 0;
        for(std::n_vector<CIFSlotWithHelp*>::iterator it = g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.begin(); it != g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.end(); it++)
        {
            if((*it)->ItemInfo != NULL)
            {
                if((*it)->ItemInfo->GetItemData() != NULL)
                {
                    if((*it)->ItemInfo->GetItemData()->IsHPPotion())
                    {
                        TotalHPPotion += (*it)->ItemInfo->GetQuantity();
                    }
                }
            }
        }



        int TotalMPPotion = 0;
        for(std::n_vector<CIFSlotWithHelp*>::iterator it = g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.begin(); it != g_pCGInterface->GetMainPopup()->GetInventory()->pSlots.end(); it++)
        {
            if((*it)->ItemInfo != NULL)
            {
                if((*it)->ItemInfo->GetItemData() != NULL)
                {
                    if((*it)->ItemInfo->GetItemData()->IsMPPotion())
                    {
                        TotalMPPotion += (*it)->ItemInfo->GetQuantity();
                    }
                }

            }
        }

        if(TotalHPPotion < 10)
        {
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_MACRO_TOTAL_HP_LESS"));
            BackTown();
            return;
        }
        if(TotalMPPotion < 10)
        {
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_MACRO_TOTAL_MP_LESS"));
            BackTown();
            return;
        }
    }

    if(AutoHuntSetting[REPAIR_SETTING] == eRepairSetting::USE_REPAIR_HAMMER)
    {
        bool itemfound = false;
        for(int i = 0; i < 7; i++)
        {

            CSOItem * p = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(i);
            if(p)
            {
                if(p->GetItemData() != NULL)
                {
                    if(p->m_CurrDurability < 5)
                    {
                        itemfound = true;
                        break;
                    }
                }
            }
        }
        CSOItem * p = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(7);
        if(p != NULL)
        {
            if(p->GetItemData() != NULL)
            {
                if(p->GetItemData()->m_typeId.getTypeID1() == 3 && p->GetItemData()->m_typeId.getTypeID2() == 1 && p->GetItemData()->m_typeId.getTypeID3() == 4)
                {
                    if(p->m_CurrDurability < 5)
                    {
                        itemfound = true;
                    }
                }
            }
        }
        if(itemfound)
        {
            CIFInventory *Inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            int slotcnt = Inventory->InventorySlotCount();
            for(int i = 0; i < slotcnt && i < (int)Inventory->pSlots.size(); i++)
            {
                if(Inventory->pSlots[i] != NULL && Inventory->pSlots[i]->ItemInfo != NULL)
                {
                    if(Inventory->pSlots[i]->ItemInfo->GetItemData() != NULL)
                    {
                        if(Inventory->pSlots[i]->ItemInfo->GetItemData()->IsRepairHammer())
                        {
                            static unsigned long lastRepairRequest = 0;
                            const unsigned long now = GetTickCount();
                            if (now - lastRepairRequest < 5000)
                                break;

                            CMsgStreamBuffer buf(0x704C);
                            UINT16 type = Inventory->pSlots[i]->ItemInfo->GetItemData()->m_typeId.m_type_id_value;
                            byte slot = i + 13;
                            buf << slot;
                            buf << type;
                            //printf("%d \n", type);
                            SendMsg(buf);
                            lastRepairRequest = now;
                            break;
                        }
                    }
                }
            }

        }
    }
    else if(AutoHuntSetting[REPAIR_SETTING] == eRepairSetting::RETURN_TOWN)
    {
        for(int i = 0; i < 7; i++)
        {
            CSOItem * p = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(i);
            if(p  != NULL)
            {
                if(p->GetItemData() != NULL)
                {
                    if(p->m_CurrDurability < 5)
                    {
                        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_MACRO_TOTAL_DURABILITY_LESS"));

                        BackTown();
                        break;
                    }
                }
            }
        }
        CSOItem * p = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(7);
        if(p  != NULL)
        {
            if(p->GetItemData() != NULL)
            {
                if(p->GetItemData()->m_typeId.getTypeID1() == 3 && p->GetItemData()->m_typeId.getTypeID2() == 1 && p->GetItemData()->m_typeId.getTypeID3() == 4)
                {
                    if(p->m_CurrDurability < 5)
                    {
                        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_MACRO_TOTAL_DURABILITY_LESS"));
                        BackTown();
                    }
                }
            }
        }
    }
    if(AutoUseNasrun->GetCheckedState_MAYBE())
    {
        CheckNasrun();
    }
    if(AutoUseNasrunIgnore->GetCheckedState_MAYBE())
    {
        CheckNasrunIgnore();
    }
}
uregion CIFMacroMenuAutoHunt::GetRegion() const {
    return StartRegion;
}

<<<<<<< ours
D3DVECTOR CIFMacroMenuAutoHunt::GetLocation() const {
    return StartPosition;
}
bool CIFMacroMenuAutoHunt::IsUniqueTargetEnabled() const {
    return UniqueTargetCheckBox && UniqueTargetCheckBox->GetCheckedState_MAYBE();
}
bool CIFMacroMenuAutoHunt::IsRuntimeReady() const {
    return AutoPartyInviteCheckBox && GDR_AUTO_REFORM_PARTY &&
           DONT_ATTACK_MONSTERCB && AutoResPtMembersCB &&
           AutoReturnHPMPLessCheckbox && AutoUseNasrun && AutoUseNasrunIgnore;
}
#include <sstream>
void CIFMacroMenuAutoHunt::SaveButton(){
=======
D3DVECTOR CIFMacroMenuAutoHunt::GetLocation() const {
    return StartPosition;
}
bool CIFMacroMenuAutoHunt::IsUniqueTargetEnabled() const {
    return UniqueTargetCheckBox && UniqueTargetCheckBox->GetCheckedState_MAYBE();
}
bool CIFMacroMenuAutoHunt::IsRuntimeReady() const {
    return AutoPartyInviteCheckBox && GDR_AUTO_REFORM_PARTY &&
           DONT_ATTACK_MONSTERCB && AutoResPtMembersCB &&
           AutoReturnHPMPLessCheckbox && AutoUseNasrun && AutoUseNasrunIgnore;
}
#include <sstream>
void CIFMacroMenuAutoHunt::SaveButton(){
>>>>>>> theirs



    std::n_wstring RangeW = m_IRM.GetResObj<CIFEdit>(46, 1)->GetNText().c_str();

    if (RangeW.length() > 0)
    {
        std::wstring ws(RangeW.c_str());
        std::wstringstream wss(ws);

        int selectedRange = 10;
        wss >> selectedRange; // wstringstream ile dÃ¶nÃ¼ÅŸÃ¼m

        AutoHuntSetting[RADUIS_SETTING] = selectedRange;
    }

    std::n_wstring hwan = m_IRM.GetResObj<CIFPopupList>(48, 1)->m_text->GetNText().c_str();
    int selectedhwan = 0;
    if(hwan.length() > 0)
    {

        std::n_wstring hwan1 = KmtGetText(L"UIIT_KMT_ALL");
        std::n_wstring hwan2 = KmtGetText(L"UIIT_KMT_GIANT");
        std::n_wstring hwan3 = (TSM_GETTEXTPTR(L"UIIT_STT_OFF"));

        if(hwan == hwan1)
        {
            selectedhwan = 1;
        }
        else if(hwan == hwan2)
        {
            selectedhwan = 2;
        }
        else if(hwan == hwan3)
        {
            selectedhwan = 3;
        }
        AutoHuntSetting[ZERK_SETTING] = selectedhwan;
    }

    std::n_wstring res = m_IRM.GetResObj<CIFPopupList>(50, 1)->m_text->GetNText().c_str();
    int selectedres = 0;
    if(res.length() > 0)
    {
        std::n_wstring sec1 = KmtGetText(L"UIIT_KMT_USE_RES_SCROLL");
        std::n_wstring sec2 = KmtGetText(L"UIIT_KMT_RESURRECT_IN_TOWN");
        std::n_wstring sec3 = KmtGetText(L"UIIT_KMT_WAIT_FOR_RESURRECTION");

        if(res == sec1)
        {
            selectedres = 1;
        }
        else if(res == sec2)
        {
            selectedres = 2;
        }
        else if(res == sec3)
        {
            selectedres = 3;
        }
        AutoHuntSetting[RETURN_TOWN_SETTING] = selectedres;
    }

    std::n_wstring returne = m_IRM.GetResObj<CIFPopupList>(52, 1)->m_text->GetNText().c_str();
    int selectedreturn = 0;
    if(returne.length() > 0)
    {
        std::n_wstring ret1 = KmtGetText(L"UIIT_KMT_IN_1_HOUR");
        std::n_wstring ret2 = KmtGetText(L"UIIT_KMT_IN_3_HOURS");
        std::n_wstring ret3 = KmtGetText(L"UIIT_KMT_IN_5_HOURS");
        std::n_wstring ret4 = KmtGetText(L"UIIT_KMT_IN_10_HOURS");
        std::n_wstring ret5 = KmtGetText(L"UIIT_KMT_IN_24_HOURS");
        if(returne == ret1)
        {
            selectedreturn = 1;
        }
        else if(returne == ret2)
        {
            selectedreturn = 3;
        }
        else if(returne == ret3)
        {
            selectedreturn = 5;
        }
        else if(returne == ret4)
        {
            selectedreturn = 10;
        }
        else if(returne == ret5)
        {
            selectedreturn = 24;
        }

        AutoHuntSetting[BACK_HOUR_SETTING] = selectedreturn;
    }

    std::n_wstring repair = m_IRM.GetResObj<CIFPopupList>(54, 1)->m_text->GetNText().c_str();
    int selectedrepair = 0;
    if(repair.length() > 0)
    {

        std::n_wstring rep1 = KmtGetText(L"UIIT_KMT_USE_REPAIR_HAMMER");
        std::n_wstring rep2 = KmtGetText(L"UIIT_KMT_RETURN_TO_TOWN");

        if(repair == rep1)
        {
            selectedrepair = 1;
        }
        else if(repair == rep2)
        {
            selectedrepair = 2;
        }
        AutoHuntSetting[REPAIR_SETTING] = selectedrepair;
    }

<<<<<<< ours
    char settingDirectory[MAX_PATH];
    if (!KmtFormatPath(settingDirectory, sizeof(settingDirectory), "%s\\Setting", theApp.GetWorkingDir()))
        return;
    CreateDirectoryA(settingDirectory, NULL);

    char buffer3[0x200];
    const std::n_wstring characterName = KmtSanitizeMacroCharacterName(
        g_pMyPlayerObj ? g_pMyPlayerObj->GetCharName().c_str() : L"");
    if (characterName.empty() ||
        !KmtFormatPath(buffer3, sizeof(buffer3), "%s\\Setting\\%ls_Macro.txt", theApp.GetWorkingDir(), characterName.c_str()))
        return;

    char temporaryPath[0x240];
    FILE *file3 = KmtOpenAtomicTextFile(buffer3, temporaryPath, sizeof(temporaryPath));
=======
    char settingDirectory[MAX_PATH];
    if (!KmtFormatPath(settingDirectory, sizeof(settingDirectory), "%s\\Setting", theApp.GetWorkingDir()))
        return;
    CreateDirectoryA(settingDirectory, NULL);

    char buffer3[0x200];
    const std::n_wstring characterName = KmtSanitizeMacroCharacterName(
        g_pMyPlayerObj ? g_pMyPlayerObj->GetCharName().c_str() : L"");
    if (characterName.empty() ||
        !KmtFormatPath(buffer3, sizeof(buffer3), "%s\\Setting\\%ls_Macro.txt", theApp.GetWorkingDir(), characterName.c_str()))
        return;

    char temporaryPath[0x240];
    FILE *file3 = KmtOpenAtomicTextFile(buffer3, temporaryPath, sizeof(temporaryPath));
>>>>>>> theirs
    if (file3 != NULL) {
        // Veri kontrolÃ¼ ve dosyaya yazma

        fprintf(file3, "Dont attack the monsters: %d\n", DONT_ATTACK_MONSTERCB->GetCheckedState_MAYBE());
        fprintf(file3, "Unique target: %d\n", IsUniqueTargetEnabled());
        fprintf(file3, "Radius: %d\n", AutoHuntSetting[RADUIS_SETTING]);
        fprintf(file3, "Hwan setting: %d\n", AutoHuntSetting[ZERK_SETTING]);
        fprintf(file3, "Return town setting: %d\n", AutoHuntSetting[RETURN_TOWN_SETTING]);
        fprintf(file3, "Back town setting: %d\n", AutoHuntSetting[BACK_HOUR_SETTING]);
        fprintf(file3, "Repair setting: %d\n", AutoHuntSetting[REPAIR_SETTING]);
        fprintf(file3, "Back town when potion less: %d\n", AutoReturnHPMPLessCheckbox->GetCheckedState_MAYBE());
        fprintf(file3, "Nasrun setting: %d\n", AutoUseNasrun->GetCheckedState_MAYBE());
        fprintf(file3, "Ignore skill setting: %d\n", AutoUseNasrunIgnore->GetCheckedState_MAYBE());

        fprintf(file3, "Invite to Party: %d\n", AutoPartyInviteCheckBox->GetCheckedState_MAYBE());
        fprintf(file3, "Accept party invite: %d\n", AutoPartyAcceptCheckBox->GetCheckedState_MAYBE());
        fprintf(file3, "Only from list: %d\n", AutoPartyOnlyFromList->GetCheckedState_MAYBE());
        fprintf(file3, "Accept join party requests: %d\n", GDR_ACCEPT_JOIN_PT_REQ->GetCheckedState_MAYBE());
        fprintf(file3, "Auto reform party: %d\n",GDR_AUTO_REFORM_PARTY->GetCheckedState_MAYBE());

        fprintf(file3, "Resurrect party members: %d\n", AutoResPtMembersCB->GetCheckedState_MAYBE());


        std::n_wstring Title = GDR_AUTO_REFORM_TITLE->GetNText();
        fprintf(file3, "Auto reform party title: %ls\n", Title.c_str());


        fprintf(file3, "Auto reform min level: %ls\n",GDR_AUTO_REFORM_PT_LVL_MIN->GetNText().c_str());
        fprintf(file3, "Auto reform max level: %ls\n", GDR_AUTO_REFORM_PT_LVL_MAX->GetNText().c_str());

        if(GDR_PT_EXP_AUTO_ITEM_NOCheckBox->GetCheckedState_MAYBE())
        {
            fprintf(file3, "Auto party setting: %d\n", 84);
        }
        else if(GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->GetCheckedState_MAYBE())
        {
            fprintf(file3, "Auto party setting: %d\n", 87);
        }
        else if(GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->GetCheckedState_MAYBE())
        {
            fprintf(file3, "Auto party setting: %d\n", 90);
        }
        else if(GDR_PT_EXP_FREE_ITEM_FREECheckBox->GetCheckedState_MAYBE())
        {
            fprintf(file3, "Auto party setting: %d\n", 93);
        }
        int i = 0;
        if(AutoPartyMemberList.size() > 0)
        {
            for(std::map<std::n_wstring, std::n_wstring>::iterator it = AutoPartyMemberList.begin();
                it != AutoPartyMemberList.end(); it++)
            {
                i++;
                fprintf(file3, "Auto party member %d: %ls\n",i, it->first.c_str());
            }
        }
<<<<<<< ours
        KmtCommitAtomicTextFile(file3, temporaryPath, buffer3);
=======
        KmtCommitAtomicTextFile(file3, temporaryPath, buffer3);
>>>>>>> theirs
        //printf("Veri dosyaya baÅŸarÄ±yla yazÄ±ldÄ±: %s", buffer3);
    }

}
undefined1 CIFMacroMenuAutoHunt::OnCloseWnd(){
    return CIFWnd::OnCloseWnd();
}

void CIFMacroMenuAutoHunt::CancelBtn(){
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->ShowGWnd(false);
}
void CIFMacroMenuAutoHunt::LoadSetting(){
    wchar_t radius[16] = {0};
    swprintf_s(radius, L"%d", AutoHuntSetting[RADUIS_SETTING]);
    RadiusList->SetText(radius);

    switch (AutoHuntSetting[ZERK_SETTING])
    {
        case eZerkSetting::Giant: ZerkList->m_text->SetText(KmtGetText(L"UIIT_KMT_GIANT")); break;
        case eZerkSetting::OFF: ZerkList->m_text->SetText(TSM_GETTEXTPTR(L"UIIT_STT_OFF")); break;
        case eZerkSetting::All:
        default: ZerkList->m_text->SetText(KmtGetText(L"UIIT_KMT_ALL")); break;
    }

    switch (AutoHuntSetting[RETURN_TOWN_SETTING])
    {
        case eTownSetting::GO_TOWN: TownList->m_text->SetText(KmtGetText(L"UIIT_KMT_RESURRECT_IN_TOWN")); break;
        case eTownSetting::WAIT_FOR_RES: TownList->m_text->SetText(KmtGetText(L"UIIT_KMT_WAIT_FOR_RESURRECTION")); break;
        case eTownSetting::RES_SCROLL:
        default: TownList->m_text->SetText(KmtGetText(L"UIIT_KMT_USE_RES_SCROLL")); break;
    }

    switch (AutoHuntSetting[BACK_HOUR_SETTING])
    {
        case eBackTownHour::HOUR_3: HourList->m_text->SetText(KmtGetText(L"UIIT_KMT_IN_3_HOURS")); break;
        case eBackTownHour::HOUR_5: HourList->m_text->SetText(KmtGetText(L"UIIT_KMT_IN_5_HOURS")); break;
        case eBackTownHour::HOUR_10: HourList->m_text->SetText(KmtGetText(L"UIIT_KMT_IN_10_HOURS")); break;
        case eBackTownHour::HOUR_24: HourList->m_text->SetText(KmtGetText(L"UIIT_KMT_IN_24_HOURS")); break;
        case eBackTownHour::HOUR_1:
        default: HourList->m_text->SetText(KmtGetText(L"UIIT_KMT_IN_1_HOUR")); break;
    }

    RepairList->m_text->SetText(
        AutoHuntSetting[REPAIR_SETTING] == eRepairSetting::RETURN_TOWN
            ? KmtGetText(L"UIIT_KMT_RETURN_TO_TOWN")
            : KmtGetText(L"UIIT_KMT_USE_REPAIR_HAMMER"));

    if (Macro_AutoHunt && MacroAutoTownTimerRunning)
    {
        MacroAutoTownTimerRunning = false;
        g_pCGInterface->KillTimer(START_BACK_TOWN);
        StartAutoHunt();
    }
}

void CIFMacroMenuAutoHunt::PartyExpItemSetting()
{
    int id = GetCurrentEventMsgCtrlId();
    if(id == 84)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(true);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);

    }
    else if(id == 87)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(true);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);
    }
    else if(id == 90)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(true);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);
    }
    else if(id == 93)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(true);
    }
}
void CIFMacroMenuAutoHunt::LoadPartyExpItemSetting(int id)
{
    if(id == 84)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(true);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);

    }
    else if(id == 87)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(true);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);
    }
    else if(id == 90)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(true);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(false);
    }
    else if(id == 93)
    {
        GDR_PT_EXP_AUTO_ITEM_NOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->SetCheckBoxState(false);
        GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->SetCheckBoxState(false);
        GDR_PT_EXP_FREE_ITEM_FREECheckBox->SetCheckBoxState(true);
    }
}
