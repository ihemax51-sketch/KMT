
#include "IFDynamicRankingSlot.h"


GFX_IMPLEMENT_DYNCREATE(CIFDynamicRankingSlot, CIFWnd)

CIFDynamicRankingSlot::CIFDynamicRankingSlot(void)
{
}
CIFDynamicRankingSlot::~CIFDynamicRankingSlot(void)
{
}
bool CIFDynamicRankingSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdynamicrankingslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    return true;
}
void CIFDynamicRankingSlot::OnUpdate() {

}

void CIFDynamicRankingSlot::Clear()
{
    this->m_IRM.GetResObj<CIFStatic>(3, 1)->m_FontTexture.SetColor(0xffffff);
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->m_FontTexture.SetColor(0xffffff);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->m_FontTexture.SetColor(0xffffff);
    this->m_IRM.GetResObj<CIFStatic>(7, 1)->m_FontTexture.SetColor(0xffffff);

    this->m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(L"");
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(L"");
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(L"");
    this->m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(L"");
}
void CIFDynamicRankingSlot::NewUpdate()
{
    this->m_IRM.GetResObj<CIFStatic>(3, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(7, 1)->m_FontTexture.SetColor(0x00FF00);
}

void CIFDynamicRankingSlot::SetName(int Num, const wchar_t* Charname, const wchar_t* Guild, const wchar_t* Points) {
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d", Num);
    this->m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(buffer1);
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(Charname);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(Guild);
    this->m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(Points);

}
