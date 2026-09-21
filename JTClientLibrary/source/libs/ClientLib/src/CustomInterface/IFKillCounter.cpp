#include <GInterface.h>
#include "IFKillCounter.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <TextStringManager.h>

#define GDR_BAR6 9
#define GDR_BAR7 10
#define GDR_BAR8 11
#define GDR_FTW_TITLE_LABEL 12
GFX_IMPLEMENT_DYNCREATE(CIFKillCounter, CIFFrame)



CIFKillCounter::CIFKillCounter(void) {

    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFKillCounter::~CIFKillCounter(void) {


    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFKillCounter::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    wnd_rect sz;

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifkillcounter.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_IRM.GetResObj(11, 1)->SetText(L"1.");
    m_IRM.GetResObj(12, 1)->SetText(L"2.");
    m_IRM.GetResObj(13, 1)->SetText(L"3.");
    m_IRM.GetResObj(14, 1)->SetText(L"4.");
    m_IRM.GetResObj(15, 1)->SetText(L"5.");


    this->m_IRM.GetResObj(9, 1)->SetText(KmtGetText(L"UIIT_KMT_KILL"));
    this->m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_NAME"));
    this->m_IRM.GetResObj(7, 1)->SetText(KmtGetText(L"UIIT_KMT_NO"));


    m_pTitleText->MoveGWnd(15, 6);

    m_pCloseBtn->MoveGWnd(m_pCloseBtn->GetPos().x-2, m_pCloseBtn->GetPos().y - 4);
    this->ShowGWnd(false);
    Clear();
    UpdateMenuSize();

    return true;
}

void CIFKillCounter::OnUpdate()
{

}

void CIFKillCounter::WriteLine(BYTE Num, const wchar_t* Charname, const wchar_t* DMG)
{
    switch (Num)
    {
        case 1:
            this->m_IRM.GetResObj(16, 1)->SetText(Charname);
            this->m_IRM.GetResObj(21, 1)->SetText(DMG);
            break;
        case 2:
            this->m_IRM.GetResObj(17, 1)->SetText(Charname);
            this->m_IRM.GetResObj(22, 1)->SetText(DMG);
            break;
        case 3:
            this->m_IRM.GetResObj(18, 1)->SetText(Charname);
            this->m_IRM.GetResObj(23, 1)->SetText(DMG);
            break;
        case 4:
            this->m_IRM.GetResObj(19, 1)->SetText(Charname);
            this->m_IRM.GetResObj(24, 1)->SetText(DMG);
            break;
        case 5:
            this->m_IRM.GetResObj(20, 1)->SetText(Charname);
            this->m_IRM.GetResObj(25, 1)->SetText(DMG);
            break;
        default:
            break;
    }
}

void CIFKillCounter::Clear()
{
    for(int i = 0; i < 5; i++)
    {
        this->m_IRM.GetResObj(16+i, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }
    for(int i = 0; i < 5; i++)
    {
        this->m_IRM.GetResObj(21+i, 1)->SetText(L"-");
    }
}

void CIFKillCounter::UpdateMenuSize()
{
    CIFWnd *minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
    wnd_pos x = minimap->GetPos();
    MoveGWnd(g_CGame->GetRes().res->width - (110 * 2), x.y + 400);
    BringToFront();

}
