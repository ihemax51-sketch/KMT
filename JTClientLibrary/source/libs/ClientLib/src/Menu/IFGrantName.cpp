#include <Game.h>
#include <ICPlayer.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <BSLib/multibyte.h>
#include <GlobalDataManager.h>
#include <Macro/IFMacroMenuAutoHunt.h>
#include <EntityManagerClient.h>
#include <ICMonster.h>
#include <NavMesh/LocationInfo.h>
#include <NavMesh/IRegionManager.h>
#include <Macro/IFMacroMenu.h>
#include <sstream>
#include <ExtraUI/IFCounterWnd.h>
#include <CustomData/CustomDataManager.h>
#include "IFGrantName.h"



#define GDR_GRANTNAME_BTN_APPLY 9
#define GDR_GRANTNAME_EDIT 7
#define GDR_GRANTNAME_BG3 52
#define GDR_GRANTNAME_BG2 51
#define GDR_GRANTNAME_BG1 50
#define GDR_GRANTNAME_LABEL 53


GFX_IMPLEMENT_DYNCREATE(CIFGrantName, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFGrantName, CIFMainFrame)
                    ONG_COMMAND(GDR_GRANTNAME_BTN_APPLY, &On_BtnClick)
                    ONG_COMMAND(1000, &On_BtnClick)

GFX_END_MESSAGE_MAP()

CIFGrantName::CIFGrantName(void){
}
CIFGrantName::~CIFGrantName(void){

}

bool CIFGrantName::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifgrantname.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    this->SetText(KmtGetText(L"UIIT_KMT_CHANGE_THE_GRANT_NAME"));

    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->JustifyHorizontal(JUSTIFY_CENTER);
    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->JustifyVertical(JUSTIFY_BOTTOM);

    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetTooltip(KmtGetText(L"UIIT_KMT_ENTER_THE_GRANT_NAME"));
    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetStyleThingy(TOOLTIP);


    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->AddValue_404(4);
    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetValue_404(2);
    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetMaxLength(12);
    wnd_size t =  m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->GetSize();

    this->m_IRM.GetResObj(9, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIRM"));
    this->m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_GRANT_NAME_MUST_CONSIST_2_12_ENGLISH_LETTERS"));

    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetTextmode(t.width);

  /*  RECT m_pSlotRect = { this->GetPos().x , this->GetPos().y, 32, 32 };
    CIFSlotWithHelp*m_pMySlot = (CIFSlotWithHelp*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelp), m_pSlotRect, this->UniqueID()+70, 0);

    m_pMySlot->SetSlot(700);
    m_pMySlot->SetSlotType(0xC);*/
    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}

void CIFGrantName::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFGrantName::OnUpdate()
{

}
void CIFGrantName::Clear()
{
    m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetText(L"");
}

#define DAT_00eedf08 (*(void **) 0x00eedf08)        /// charID ?
int getDecimalFromLast4HexDigits(int value) {
    std::stringstream ss;
    ss << std::hex << value;
    std::string hexStr = ss.str();

    // Son 4 basamağı al
    std::string last4HexDigits = hexStr.substr(hexStr.length() - 4);

    // Tekrar decimal formata çevir
    int decimalValue;
    std::stringstream ss2;
    ss2 << std::hex << last4HexDigits;
    ss2 >> decimalValue;

    return decimalValue;
}

void CIFGrantName::On_BtnClick() {
  g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleRegionStartTime();
    g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleStage_2_Time(1);
    m_CustomDataManager->CircleGreenShow = true;
  //  g_sNewInterfaceMgr.InstantiateDimensional("clientlibrary\\nifsafetydevice.2dt", this, false);
  //  g_sNewInterfaceMgr.InstantiateDimensional("clientlibrary\\auto_login.2dt", this, false);

 /*   g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->ShowGWnd(true);
    g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->SetTimer(40, 0);
    g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->StartTimerWnd2();
    g_pCGInterface->m_IRM.GetResObj<CIFCounterWnd>(CounterWndNew, 1)->UpdateMenuSize();
*/

    std::n_wstring guildname2 = g_pMyPlayerObj->GetGuildName().c_str();
    if (guildname2.empty()) {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_MUST_ENTER_THE_GUILD"));
        return;
    }
    if ( m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->GetNText().length() >= 2) {
        CMsgStreamBuffer buf(0x207B);
        std::n_string grant = TO_NSTRING(this-> m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->GetNText());
        buf << grant;
        SendMsg(buf);
        m_IRM.GetResObj<CIFEdit>(GDR_GRANTNAME_EDIT, 1)->SetText(L"");

    } else {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_GRANT_NAME_MUST_BE_CONSIST_MIN_2_CHARACTERS"));
        return;
    }

}

#define TIMER_BATTLE_ROYALE_MAP_STAGE_1 610
#define TIMER_BATTLE_ROYALE_MAP_STAGE_2 611
#define TIMER_BATTLE_ROYALE_MAP_STAGE_3 612
#define TIMER_BATTLE_ROYALE_MAP_STAGE_4 613
#define TIMER_BATTLE_ROYALE_MAP_STAGE_5 614
#define TIMER_BATTLE_ROYALE_MAP_STAGE_6 615
void CIFGrantName::OnTimer(int timerId)
{
    if (timerId == TIMER_BATTLE_ROYALE_MAP_STAGE_1) {

        m_CustomDataManager->x1 = m_battleroyale_stage1_SecondsLeft;
        m_CustomDataManager->x2 = - m_battleroyale_stage1_SecondsLeft;
        if (m_battleroyale_stage1_SecondsLeft < 119) {
            m_battleroyale_stage1_SecondsLeft++;
        } else {
            m_CustomDataManager->cercleX = 275;
            m_CustomDataManager->cercleY = 202;
            m_CustomDataManager->cercleRadius = 134;
            this->KillTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_1);
        }
    }
    if (timerId == TIMER_BATTLE_ROYALE_MAP_STAGE_2) {

        m_CustomDataManager->x1 = 119 - m_battleroyale_stage2_SecondsLeft;
        m_CustomDataManager->x2 = -119 - m_battleroyale_stage2_SecondsLeft;
        m_CustomDataManager->cercle = -47 + m_battleroyale_stage2_SecondsLeft;
        if (m_battleroyale_stage2_SecondsLeft < 48) {
            m_battleroyale_stage2_SecondsLeft++;
        } else {
           m_CustomDataManager->cercleX = 233;
           m_CustomDataManager->cercleY = 202;
           m_CustomDataManager->cercleRadius = 114;
            this->KillTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_2);
        }
    }
    //41s
    if (timerId == TIMER_BATTLE_ROYALE_MAP_STAGE_3) {

        m_CustomDataManager->x1 =  71 - m_battleroyale_stage3_SecondsLeft;
        m_CustomDataManager->x2 = -171 - m_battleroyale_stage3_SecondsLeft;
        m_CustomDataManager->cercle =  m_battleroyale_stage3_SecondsLeft;
        if (m_battleroyale_stage3_SecondsLeft < 41) {
            m_battleroyale_stage3_SecondsLeft++;
        } else {
            m_CustomDataManager->cercleX = 233;
            m_CustomDataManager->cercleY = 155;
            m_CustomDataManager->cercleRadius = 72.5;
            this->KillTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_3);
        }
    }
    //47s
    if (timerId == TIMER_BATTLE_ROYALE_MAP_STAGE_4) {

       m_CustomDataManager->x1 =  31;
       m_CustomDataManager->x2 = - 212;
       m_CustomDataManager->y1 = -m_battleroyale_stage4_SecondsLeft;
       m_CustomDataManager->y2 = -m_battleroyale_stage4_SecondsLeft;
       m_CustomDataManager->cercle =  41 + m_battleroyale_stage4_SecondsLeft;
        if (m_battleroyale_stage4_SecondsLeft < 48) {
            m_battleroyale_stage4_SecondsLeft++;
        } else {
            m_CustomDataManager->cercleX = 235;
            m_CustomDataManager->cercleY = 155;
            m_CustomDataManager->cercleRadius = 36;
            this->KillTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_4);
        }
    }
    //51s
    if (timerId == TIMER_BATTLE_ROYALE_MAP_STAGE_5) {

       m_CustomDataManager->x1 =  31;
       m_CustomDataManager->x2 = - 212;
       m_CustomDataManager->y1 = -47;
       m_CustomDataManager->y2 = -47;
       m_CustomDataManager->cercle = 94 + m_battleroyale_stage5_SecondsLeft;
        if (m_battleroyale_stage5_SecondsLeft < 52) {
            m_battleroyale_stage5_SecondsLeft++;
        } else {
           m_CustomDataManager->cercleX = 235;
           m_CustomDataManager->cercleY = 155;
           m_CustomDataManager->cercleRadius = 15;
            this->KillTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_5);
        }
    }
}
void CIFGrantName::SetBattleRoyaleStage_1_Time(int timeoutSeconds) {

    this->m_battleroyale_stage1_SecondsLeft = timeoutSeconds;
    StartTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_1, 1000);
}
void CIFGrantName::SetBattleRoyaleStage_2_Time(int timeoutSeconds) {

    this->m_battleroyale_stage2_SecondsLeft = timeoutSeconds;
    StartTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_2, 2500);
}
void CIFGrantName::SetBattleRoyaleStage_3_Time(int timeoutSeconds) {

    this->m_battleroyale_stage3_SecondsLeft = timeoutSeconds;
    StartTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_3, 3000);
}
void CIFGrantName::SetBattleRoyaleStage_4_Time(int timeoutSeconds) {

    this->m_battleroyale_stage4_SecondsLeft = timeoutSeconds;
    StartTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_4, 2500);
}
void CIFGrantName::SetBattleRoyaleStage_5_Time(int timeoutSeconds) {

    this->m_battleroyale_stage5_SecondsLeft = timeoutSeconds;
    StartTimer(TIMER_BATTLE_ROYALE_MAP_STAGE_5, 2400);
}
#define TIMER_BATTLE_REGION_HANDLE 711
void CIFGrantName::SetBattleRoyaleRegionStartTime() {
    StartTimer(TIMER_BATTLE_REGION_HANDLE, 5000);
}
void CIFGrantName::SetBattleRoyaleRegionKillTime() {
    KillTimer(TIMER_BATTLE_REGION_HANDLE);
}