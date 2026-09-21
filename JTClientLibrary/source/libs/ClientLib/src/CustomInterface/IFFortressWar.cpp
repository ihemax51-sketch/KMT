#include <GInterface.h>
#include "IFFortressWar.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <TextStringManager.h>

#define GDR_GUILD_BTN 4
#define GDR_PLAYER_BTN 5
#define GDR_FTW_RANK1 6
#define GDR_FTW_RANK2 7
#define GDR_FTW_RANK3 8
#define GDR_FTW_RANK4 9
#define GDR_FTW_RANK5 10

GFX_IMPLEMENT_DYNCREATE(CIFFortressWar, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFFortressWar, CIFMainFrame)
                    ONG_COMMAND(100, &CIFFortressWar::OnUnknownStuff)
GFX_END_MESSAGE_MAP()



CIFFortressWar::CIFFortressWar(void) {

    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFFortressWar::~CIFFortressWar(void) {
    if (m_pTabs) {
        free(m_pTabs);
        m_pTabs = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}
bool CIFFortressWar::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\iffortresswar.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    wnd_rect sz;
    sz.pos.x= 45;
    sz.pos.y = 42;
    m_pTabs = new CIFSelectableArea *[numberOfTabs];

    for (int i = 0; i < numberOfTabs; i++) {

        RECT selectable_area_size;
        selectable_area_size.top = -22;
        selectable_area_size.left = 6;
        // selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth + 1;
        selectable_area_size.bottom = tabHeight;

        m_pTabs[i] = (CIFSelectableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                               selectable_area_size, tabFirstId + i, 0);

        m_pTabs[i]->SetFont(this->N00009C2F);


        m_pTabs[i]->sub_64CE30("interface\\mall\\mall_sub_tab_on.ddj",
                               "interface\\mall\\mall_sub_tab_off.ddj", "interface\\mall\\mall_sub_tab_off.ddj");

        switch (i) {
            case 0:
            {
                m_pTabs[0]->sub_64CC30(1);
                m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_PLAYER"));
            }
                break;
            case 1:
            {
                m_pTabs[1]->MoveGWnd(m_pTabs[0]->GetPos().x + tabWidth + 4, m_pTabs[0]->GetPos().y);
                m_pTabs[1]->SetText(KmtGetText(L"UIIT_KMT_GUILD"));
                // m_pTabs[1]->SetClickable(false);
            }
                break;
            case 2:
            {
                m_pTabs[2]->MoveGWnd(m_pTabs[1]->GetPos().x + tabWidth + 4, m_pTabs[1]->GetPos().y);
                m_pTabs[2]->SetText(KmtGetText(L"UIIT_KMT_UNION"));
                //m_pTabs[2]->SetClickable(false);
            }
                break;
        }
        m_pTabs[i]->sub_64CC30(0);

    }

    m_IRM.GetResObj(7, 1)->SetText(L"1.");
    m_IRM.GetResObj(8, 1)->SetText(L"2.");
    m_IRM.GetResObj(9, 1)->SetText(L"3.");
    m_IRM.GetResObj(10, 1)->SetText(L"4.");
    m_IRM.GetResObj(11, 1)->SetText(L"5.");

    for(int i = 0; i < 5; i++)
    {
        m_PlayerNameSlots[i] = m_IRM.GetResObj<CIFStatic>(12+i, 1);
        m_PlayerNameSlots[i]->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }
    for(int i = 0; i < 5; i++)
    {
        m_PlayerKillSlots[i] = m_IRM.GetResObj<CIFStatic>(17+i, 1);
        m_PlayerKillSlots[i]->SetText(L"-");
    }

    for(int i = 0; i < 5; i++)
    {
        m_GuildNameSlots[i] = m_IRM.GetResObj<CIFStatic>(22+i, 1);
        m_GuildNameSlots[i]->SetText(KmtGetText(L"UIIT_KMT_NO_GUILD"));
    }

    for(int i = 0; i < 5; i++)
    {
        m_GuildKillSlots[i] = m_IRM.GetResObj<CIFStatic>(27+i, 1);
        m_GuildKillSlots[i]->SetText(L"-");
    }

    for(int i = 0; i < 5; i++)
    {
        m_UnionNameSlots[i] = m_IRM.GetResObj<CIFStatic>(32+i, 1);
        m_UnionNameSlots[i]->SetText(KmtGetText(L"UIIT_KMT_NO_UNION"));

        m_UnionKillSlots[i] = m_IRM.GetResObj<CIFStatic>(37+i, 1);
        m_UnionKillSlots[i]->SetText(L"-");

    }

    m_pCloseBtn->ShowGWnd(false);
    ActivateTabPage(0);
    this->ShowGWnd(false);
    UpdateMenuSize();
    return true;
}
void CIFFortressWar::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFFortressWar::ActivateTabPage(BYTE page) {
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
            for(int i = 0; i < 5; i++)
            {
                m_PlayerNameSlots[i]->ShowGWnd(true);
                m_PlayerKillSlots[i]->ShowGWnd(true);

                m_GuildNameSlots[i]->ShowGWnd(false);
                m_GuildKillSlots[i]->ShowGWnd(false);

                m_UnionNameSlots[i]->ShowGWnd(false);
                m_UnionKillSlots[i]->ShowGWnd(false);
            }
        }
            break;
        case 1:
        {
            for(int i = 0; i < 5; i++)
            {
                m_PlayerNameSlots[i]->ShowGWnd(false);
                m_PlayerKillSlots[i]->ShowGWnd(false);


                m_GuildNameSlots[i]->ShowGWnd(true);
                m_GuildKillSlots[i]->ShowGWnd(true);

                m_UnionNameSlots[i]->ShowGWnd(false);
                m_UnionKillSlots[i]->ShowGWnd(false);
            }
        }
            break;

        case 2:
        {
            for(int i = 0; i < 5; i++)
            {
                m_PlayerNameSlots[i]->ShowGWnd(false);
                m_PlayerKillSlots[i]->ShowGWnd(false);


                m_GuildNameSlots[i]->ShowGWnd(false);
                m_GuildKillSlots[i]->ShowGWnd(false);

                m_UnionNameSlots[i]->ShowGWnd(true);
                m_UnionKillSlots[i]->ShowGWnd(true);
            }
        }
            break;
    }
}
int CIFFortressWar::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 7)
            return -1;
    }

    return 100;
}
void CIFFortressWar::OnUpdate()
{

}

void CIFFortressWar::On_BtnClick() {

}


void CIFFortressWar::On_BtnClickPlayer() {

}



void CIFFortressWar::WriteGuildKill(BYTE Num, const wchar_t* Charname, const wchar_t* DMG) const
{
    m_GuildNameSlots[Num - 1]->SetText(Charname);
    m_GuildKillSlots[Num - 1]->SetText(DMG);
}

void CIFFortressWar::WritePlayerKill(BYTE Num, const wchar_t* Charname, const wchar_t* DMG) const
{
    m_PlayerNameSlots[Num - 1]->SetText(Charname);
    m_PlayerKillSlots[Num - 1]->SetText(DMG);
}


void CIFFortressWar::ClearGuild() const {
    for(int i = 0; i < 5; i++)
    {
        m_GuildNameSlots[i]->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
        m_GuildKillSlots[i]->SetText(L"-");
    }
}

void CIFFortressWar::ClearPlayer() const
{
    for(int i = 0; i < 5; i++)
    {
        m_PlayerNameSlots[i]->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
        m_PlayerKillSlots[i]->SetText(L"-");
    }
}
void CIFFortressWar::WriteUnionKill(BYTE Num, const wchar_t* Charname, const wchar_t* DMG) const
{
    m_UnionNameSlots[Num - 1]->SetText(Charname);
    m_UnionKillSlots[Num - 1]->SetText(DMG);
}


void CIFFortressWar::ClearUnion() const {
    for(int i = 0; i < 5; i++)
    {
        m_UnionNameSlots[i]->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
        m_UnionKillSlots[i]->SetText(L"-");
    }
}
void CIFFortressWar::UpdateMenuSize()
{
    CIFWnd *minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
    wnd_pos x = minimap->GetPos();
    MoveGWnd(g_CGame->GetRes().res->width - (130 * 2), x.y + 380);
    BringToFront();

}
