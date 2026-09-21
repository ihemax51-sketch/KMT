#include <GInterface.h>
#include "IFTeamCounter.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <TextStringManager.h>

#define GDR_BAR6 9
#define GDR_BAR7 10
#define GDR_BAR8 11
#define GDR_FTW_TITLE_LABEL 12
GFX_IMPLEMENT_DYNCREATE(CIFTeamCounter, CIFFrame)

CIFTeamCounter::CIFTeamCounter(void) {
    bluekill = 0;
    redkill = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFTeamCounter::~CIFTeamCounter(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFTeamCounter::OnCreate(long ln) {

    // Populate inherited members
    CIFFrame::OnCreate(ln);
    wnd_rect sz;
   // this->TB_Func_13("interface\\frame\\mall_sub_wnd02_", 1, 1);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifteamcounter.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    sz.pos.x= 0;
    sz.pos.y = -40;
    sz.size.width = 84;
    sz.size.height = 32;
    bluekill = m_IRM.GetResObj<CIFStatic>(3, 1);
    bluekill->SetFont(theApp.GetFont(4));
    bluekill->m_FontTexture.SetColor(0xFFFF00);
    bluekill->JustifyHorizontal(JUSTIFY_RIGHT);
    bluekill->JustifyVertical(JUSTIFY_MIDDLE);

    bluekill->SetText(L"0");

    sz.pos.x= 128;
    sz.pos.y = -40;
    sz.size.width = 1;
    sz.size.height = 26;

    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(L":");
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetFont(theApp.GetFont(4));
    m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(0xFFFF00);
    m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.sub_8B4750(7);


    sz.pos.x= 174;
    sz.pos.y = -40;
    sz.size.width = 84;
    sz.size.height = 32;
    redkill =  m_IRM.GetResObj<CIFStatic>(6, 1);
    redkill->SetFont(theApp.GetFont(4));
    redkill->m_FontTexture.SetColor(0xFFFF00);
    redkill->JustifyHorizontal(JUSTIFY_LEFT);
    redkill->JustifyVertical(JUSTIFY_MIDDLE);
    redkill->SetText(L"0");


    m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(L"1.");
    m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(L"2.");
    m_IRM.GetResObj<CIFStatic>(9, 1)->SetText(L"3.");
    m_IRM.GetResObj<CIFStatic>(10, 1)->SetText(L"4.");
    m_IRM.GetResObj<CIFStatic>(11, 1)->SetText(L"5.");

    IconSlot1 =  m_IRM.GetResObj<CIFStatic>(22, 1);
    IconSlot2 =  m_IRM.GetResObj<CIFStatic>(23, 1);
    IconSlot3 =  m_IRM.GetResObj<CIFStatic>(24, 1);
    IconSlot4 =  m_IRM.GetResObj<CIFStatic>(25, 1);
    IconSlot5 =  m_IRM.GetResObj<CIFStatic>(26, 1);


    NameSlot1 =  m_IRM.GetResObj<CIFStatic>(12, 1);
    NameSlot1->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    NameSlot2 =  m_IRM.GetResObj<CIFStatic>(13, 1);
    NameSlot2->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    NameSlot3 =  m_IRM.GetResObj<CIFStatic>(14, 1);
    NameSlot3->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    NameSlot4 =  m_IRM.GetResObj<CIFStatic>(15, 1);
    NameSlot4->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    NameSlot5 =  m_IRM.GetResObj<CIFStatic>(16, 1);
    NameSlot5->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));


    KillSlot1 =  m_IRM.GetResObj<CIFStatic>(17, 1);
    KillSlot1->SetText(L"-");
    KillSlot2 =  m_IRM.GetResObj<CIFStatic>(18, 1);
    KillSlot2->SetText(L"-");
    KillSlot3 =  m_IRM.GetResObj<CIFStatic>(19, 1);
    KillSlot3->SetText(L"-");
    KillSlot4 =  m_IRM.GetResObj<CIFStatic>(20, 1);
    KillSlot4->SetText(L"-");
    KillSlot5 =  m_IRM.GetResObj<CIFStatic>(21, 1);
    KillSlot5->SetText(L"-");



    this->ShowGWnd(false);

    UpdateMenuSize();

    return true;
}

void CIFTeamCounter::OnUpdate()
{

}

void CIFTeamCounter::WriteLine(BYTE Sira, byte team, const wchar_t *Charname, int Kills, int RedTeam, int BlueTeam) const {
    wchar_t Blue[250];
    swprintf(Blue, sizeof(Blue), L"%d", BlueTeam);
    bluekill->SetText(Blue);


    wchar_t Red[250];
    swprintf(Red, sizeof(Red), L"%d", RedTeam);
    redkill->SetText(Red);


    wchar_t DMG[250];
    swprintf(DMG, sizeof(DMG), L"%d", Kills);
    if (Sira == 1) {
        NameSlot1->SetText(Charname);
        KillSlot1->SetText(DMG);
        if (team == 1)
        {
            IconSlot1->TB_Func_13("interface\\guild\\gil_arena_tiger_team.ddj", 1, 1);
        }
        else if (team == 3)
        {

            IconSlot1->TB_Func_13("interface\\guild\\gil_arena_dragon_team.ddj", 1, 1);
        }
    } else if (Sira == 2) {
        NameSlot2->SetText(Charname);
        KillSlot2->SetText(DMG);
        if (team == 1) {
            IconSlot2->TB_Func_13("interface\\guild\\gil_arena_tiger_team.ddj", 1, 1);
        } else if (team == 3) {

            IconSlot2->TB_Func_13("interface\\guild\\gil_arena_dragon_team.ddj", 1, 1);
        }
    } else if (Sira == 3) {
        NameSlot3->SetText(Charname);
        KillSlot3->SetText(DMG);
        if (team == 1) {
            IconSlot3->TB_Func_13("interface\\guild\\gil_arena_tiger_team.ddj", 1, 1);
        } else if (team == 3) {

            IconSlot3->TB_Func_13("interface\\guild\\gil_arena_dragon_team.ddj", 1, 1);
        }
    } else if (Sira == 4) {
        NameSlot4->SetText(Charname);
        KillSlot4->SetText(DMG);
        if (team == 1) {
            IconSlot4->TB_Func_13("interface\\guild\\gil_arena_tiger_team.ddj", 1, 1);
        } else if (team == 3) {

            IconSlot4->TB_Func_13("interface\\guild\\gil_arena_dragon_team.ddj", 1, 1);
        }
    } else if (Sira == 5) {
        NameSlot5->SetText(Charname);
        KillSlot5->SetText(DMG);
        if (team == 1) {
            IconSlot5->TB_Func_13("interface\\guild\\gil_arena_tiger_team.ddj", 1, 1);
        } else if (team == 3) {

            IconSlot5->TB_Func_13("interface\\guild\\gil_arena_dragon_team.ddj", 1, 1);
        }
    }
}

void CIFTeamCounter::Clear() const
{
    NameSlot1->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    KillSlot1->SetText(L"-");

    NameSlot2->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    KillSlot2->SetText(L"-");

    NameSlot3->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    KillSlot3->SetText(L"-");

    NameSlot4->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    KillSlot4->SetText(L"-");

    NameSlot5->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    KillSlot5->SetText(L"-");


    IconSlot1->TB_Func_13("", 1, 1);
    IconSlot2->TB_Func_13("", 1, 1);
    IconSlot3->TB_Func_13("", 1, 1);
    IconSlot4->TB_Func_13("", 1, 1);
    IconSlot5->TB_Func_13("", 1, 1);

    bluekill->SetText(L"0");
    redkill->SetText(L"0");


}

void CIFTeamCounter::UpdateMenuSize()
{
    CIFWnd *minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
    wnd_pos x = minimap->GetPos();
    MoveGWnd(g_CGame->GetRes().res->width - (130 * 2), x.y + 380);
    BringToFront();

}
