//
// Created by Kurama on 2/25/2023.
//

#include "NIFMainFrame.h"

#include "GNFontChar.h"

#define GDR_STA_TITLE 65535 // 0xffff, -1
#define GDR_STA_DRAG 65534 // 0xfffe, -2
#define GDR_BTN_CLOSE 65533 // 0xfffd, -3

GFX_IMPLEMENT_DYNAMIC_EXISTING(CNIFMainFrame, 0x0115dfa8)

GFX_BEGIN_MESSAGE_MAP(CNIFMainFrame, CNIFWnd)
                    ONG_COMMAND(GDR_BTN_CLOSE, &CNIFMainFrame::OnCloseBtnClick)
GFX_END_MESSAGE_MAP()

CNIFMainFrame::CNIFMainFrame() : m_pTitleCtrl(NULL),
                                 m_pDragAreaCtrl(NULL),
                                 m_pCloseBtnCtrl(NULL) {
}

bool CNIFMainFrame::OnCreate(long ln) {
    // called as inline
    CNIFFrame::OnCreate(ln);

    wnd_rect rcCreateBounds;
    wnd_rect rcBounds = GetBounds();

    rcCreateBounds.size.width = rcBounds.size.width - 21;
    rcCreateBounds.pos.x = 10;
    rcCreateBounds.pos.y = 12;
    rcCreateBounds.size.height = 12;

    m_pTitleCtrl = (CNIFStatic *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFStatic), rcCreateBounds, GDR_STA_TITLE, 0);

    rcCreateBounds.pos.y = 0;
    rcCreateBounds.size.height = 34;

    m_pDragAreaCtrl = (CNIFDragableArea *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFDragableArea), rcCreateBounds,
                                                          GDR_STA_DRAG, 0);

    rcCreateBounds.pos.x = rcBounds.size.width - 26;
    rcCreateBounds.size.width = 16;
    rcCreateBounds.pos.y = 10;
    rcCreateBounds.size.height = 16;

    m_pCloseBtnCtrl = (CNIFButton *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFButton), rcCreateBounds, GDR_BTN_CLOSE,
                                                    0);
    m_pCloseBtnCtrl->NTB_Func_13("interface\\ifcommon\\com_windowclose.ddj", 0, 0);

    RECT rect = {0, 0, 0, 0};

    m_pTitleCtrl->SetTextPosBounds(rect);
    m_pTitleCtrl->SetFontData(g_sNFontDataVec[0]);
    m_pTitleCtrl->SetHAlign(HALIGN_CENTER);
    m_pTitleCtrl->SetVAlign(VALIGN_TOP);

    m_pDragAreaCtrl->SetTextPosBounds(rect);
    m_pDragAreaCtrl->SetFontData(g_sNFontDataVec[0]);
    m_pDragAreaCtrl->SetHAlign(HALIGN_CENTER);
    m_pDragAreaCtrl->SetVAlign(VALIGN_TOP);

    m_pCloseBtnCtrl->SetTextPosBounds(rect);
    m_pCloseBtnCtrl->SetFontData(g_sNFontDataVec[0]);
    m_pCloseBtnCtrl->SetHAlign(HALIGN_CENTER);
    m_pCloseBtnCtrl->SetVAlign(VALIGN_TOP);

    return true;
}

void CNIFMainFrame::UpdateElementsDimension() {
    wnd_rect rcBounds = GetBounds();
    wnd_rect rcNewBounds;

    rcNewBounds.pos.y = rcBounds.pos.y + 12;
    rcNewBounds.pos.x = rcBounds.pos.x + 10;
    rcNewBounds.size.width = rcBounds.size.width - 21;

    m_pTitleCtrl->MoveGWnd(rcNewBounds.pos.x, rcNewBounds.pos.y);
    m_pTitleCtrl->SetGWndSize(rcNewBounds.size.width, 12);

    rcNewBounds.pos.x += 10;

    m_pDragAreaCtrl->MoveGWnd(rcNewBounds.pos.x, rcNewBounds.pos.y);
    m_pDragAreaCtrl->SetGWndSize(rcNewBounds.size.width, 34);

    m_pCloseBtnCtrl->MoveGWnd(rcNewBounds.pos.x - 26 + rcNewBounds.size.width, rcNewBounds.pos.y + 10);
}

void CNIFMainFrame::SetGWndSize(int nWidth, int nHeight) {
    // inline
    CNIFWnd::SetGWndSize(nWidth, nHeight);

    UpdateElementsDimension();
}

bool CNIFMainFrame::SetText(const wchar_t *pSrc) {
    return m_pTitleCtrl->SetText(pSrc);
}

void CNIFMainFrame::OnCloseBtnClick() {
    this->OnCloseWnd();
}

