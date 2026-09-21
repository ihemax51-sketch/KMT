#include "IFDps.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <GInterface.h>


#define GDR_STATIC2 5
#define GDR_STATIC3 6
GFX_IMPLEMENT_DYNCREATE(CIFDps, CIFMainFrame)


CIFDps::CIFDps(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFDps::~CIFDps(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFDps::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_pCloseBtn->ShowGWnd(false);
    m_pHandleBar->ShowGWnd(false);
    // SetText(L"Dps Meter");
    // TB_Func_13("interface\\frame\\frame_make_", 1, 1);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdps.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_IRM.GetResObj<CIFStatic>(2, 1)->SetText(KmtGetText(L"UIIT_KMT_1_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(2, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(3, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_2_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(5, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(KmtGetText(L"UIIT_KMT_3_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(6, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(7, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(KmtGetText(L"UIIT_KMT_4_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(8, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(9, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(9, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(10, 1)->SetText(KmtGetText(L"UIIT_KMT_5_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(10, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(11, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(11, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(12, 1)->SetText(KmtGetText(L"UIIT_KMT_6_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(12, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(13, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(13, 1)->BringToFront();


    m_IRM.GetResObj<CIFStatic>(14, 1)->SetText(KmtGetText(L"UIIT_KMT_7_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(14, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(15, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(15, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(16, 1)->SetText(KmtGetText(L"UIIT_KMT_8_KMTGUARD"));
    m_IRM.GetResObj<CIFStatic>(16, 1)->BringToFront();

    m_IRM.GetResObj<CIFStatic>(17, 1)->SetText(L"100.k");
    m_IRM.GetResObj<CIFStatic>(17, 1)->BringToFront();

    UpdateMenuSize();
    this->ShowGWnd(false);


    return true;
}

void CIFDps::OnUpdate()
{

}

void CIFDps::WriteLine(BYTE Num, std::n_wstring Charname, const wchar_t* DMG)
{
    switch (Num)
    {
        case 1:
        {
            std::n_wstring Charnamex = L"1." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(2, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(2, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(3, 1)->BringToFront();
        }
            break;
        case 2:
        {
            std::n_wstring Charnamex = L"2." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(5, 1)->BringToFront();
        }
            break;

        case 3:
        {
            std::n_wstring Charnamex = L"3." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(6, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(7, 1)->BringToFront();
        }
            break;
        case 4:
        {
            std::n_wstring Charnamex = L"4." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(8, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(9, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(9, 1)->BringToFront();
        }
            break;
        case 5:
        {
            std::n_wstring Charnamex = L"5." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(10, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(10, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(11, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(11, 1)->BringToFront();
        }
            break;
        case 6:
        {
            std::n_wstring Charnamex = L"6." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(12, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(12, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(13, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(13, 1)->BringToFront();
        }
            break;
        case 7:
        {
            std::n_wstring Charnamex = L"7." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(14, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(14, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(15, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(15, 1)->BringToFront();
        }
            break;
        case 8:
        {
            std::n_wstring Charnamex = L"8." + Charname;
            this->m_IRM.GetResObj<CIFStatic>(16, 1)->SetText(Charnamex.c_str());
            this->m_IRM.GetResObj<CIFStatic>(16, 1)->BringToFront();

            this->m_IRM.GetResObj<CIFStatic>(17, 1)->SetText(DMG);
            this->m_IRM.GetResObj<CIFStatic>(17, 1)->BringToFront();
        }
            break;

        default:
            break;
    }
}

void CIFDps::Clear()
{
    for(int i = 0; i < 16; i++)
    {
        this->m_IRM.GetResObj(2 + i, 1)->SetText(L"-");
    }
    this->m_IRM.GetResObj(109, 1)->SetText(L"-");
}
void CIFDps::SetUniqueName(std::n_wstring text)
{
    this->m_IRM.GetResObj(109, 1)->SetText(text.c_str());
}
void CIFDps::UpdateMenuSize()
{
    MoveGWnd(g_pCGInterface->m_IRM.GetResObj(157, 1)->GetPos().x - 154,   g_pCGInterface->GetPos().y + 30);

    BringToFront();
}
#define TIMERSHOWDPS 10002
void CIFDps::OnTimer(int timerId) {
    if (timerId == TIMERSHOWDPS) {
        this->KillTimer(TIMERSHOWDPS);

        this->ShowGWnd(false);
    }
}

void CIFDps::OpenDPS(int timeoutSeconds) {

    UpdateMenuSize();
    this->ShowGWnd(true);
    this->StartTimer(TIMERSHOWDPS, timeoutSeconds);
}
