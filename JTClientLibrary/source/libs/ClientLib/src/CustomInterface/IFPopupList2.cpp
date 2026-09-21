#include "IFPopupList2.h"

GFX_IMPLEMENT_DYNCREATE(CIFPopupList2, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFPopupList2, CIFWnd)
                    ONG_COMMAND(3, &CIFPopupList2::OnShowList)
                    ONG_BSSig_u38(2, 0x806D, 5, &CIFPopupList2::OnSelect)
GFX_END_MESSAGE_MAP()

CIFPopupList2::CIFPopupList2(void)
{
    m_bg = 0;
    m_btn = 0;
    m_text = 0;
    m_list = 0;
    m_listbg = 0;  //printf("> " __FUNCTION__ "\n");
    BS_DEBUG_LOW(">" __FUNCTION__);
}


CIFPopupList2::~CIFPopupList2(void)
{
    // printf("> " __FUNCTION__ "\n");
}

bool CIFPopupList2::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);

    RECT m_bgRect = { 0,0,this->GetSize().width - 20,this->GetSize().height };
    m_bg = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), m_bgRect, 1, 0);
    m_bg->TB_Func_13("interface\\ifcommon\\com_grad_gage_form.ddj", 0, 0);

    RECT m_textRect = { 0,0,this->GetSize().width - 20,this->GetSize().height };
    m_text = (CIFEdit*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFEdit), m_textRect, 2, 0);

    RECT m_btnRect = { this->GetSize().width - 22,0,20,this->GetSize().height };
    m_btn = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), m_btnRect, 3, 0);
    m_btn->TB_Func_13("interface\\ifcommon\\com_qst_downarrow_button.ddj", 0, 0);

    RECT m_listbgRect = { 0,this->GetSize().height,this->GetSize().width - 20,this->GetSize().height };
    m_listbg = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), m_listbgRect, 4, 0);
    m_listbg->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_m.ddj", 0, 0);
    m_listbg->ShowGWnd(false);

    RECT m_listRect = { 0,this->GetSize().height + 5,this->GetSize().width - 20,this->GetSize().height };
    m_list = (CIFTextBox*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFTextBox), m_listRect, 5, 0);
    m_list->N00000608 = this;
    m_list->m_HeightInLines = 14;
    m_list->SetLineHeight(14);
    m_list->sub_638B50(0);
    m_list->sub_64E380(0);
    m_list->sub_638C70(14);
    m_list->SetHightlineLine(true);
    m_list->sub_638D50(1);
    m_list->SetHighlightColor(D3DCOLOR_RGBA(255, 255, 255, 102));
    m_list->sub_638D40(1);
    m_list->ShowGWnd(false);
    m_list->m_FontTexture.sub_8B4400(0, 0);
    m_list->m_FontTexture.sub_8B4750(0);
    m_list->JustifyVertical(JUSTIFY_MIDDLE);
    m_list->JustifyHorizontal(JUSTIFY_LEFT);
    m_list->m_LinesOfHistory = 8;
    return true;
}
void CIFPopupList2::OnShowList()
{
    m_listbg->ShowGWnd(!m_listbg->IsVisible());
    m_list->ShowGWnd(!m_list->IsVisible());

    m_listbg->SetGWndSize(m_listbg->GetSize().width, m_list->m_CurrentLines * 14 + 10);
    m_list->SetGWndSize(m_listbg->GetSize().width, m_list->m_CurrentLines * 14 + 10);

    m_listbg->BringToFront();
    m_list->BringToFront();
}
void CIFPopupList2::OnSelect(int a1, int a2)
{
    CIFListCtrl* pList;
    pList = m_list;

    CIFListCtrl::SLineOfText* line = pList->sub_63A940();

    std::n_wstring str;
    line->m_font->GetText(&str);

    m_text->SetText(str.c_str());
    m_listbg->ShowGWnd(false);
    m_list->ShowGWnd(false);



}
void CIFPopupList2::OnUpdate() {

}